/*
 *   Second Sight is a video card with its own Z8S180 processor and memory.
 *   The U7 Lattice chip watches Apple bus accesses. It captures video writes
 *   and asks the U17 copy controller to put them in the card's static RAM
 *   (SRAM). Other accesses become events: requests for the firmware to handle.
 *   The Oak OTI-087X Video Graphics Array (VGA) controller turns video
 *   memory written by the firmware into pixels.
 *
 *   This file joins those parts. The IIgs memory mapper calls observe(); the
 *   card runs its processor and video clocks up to each Apple access. Reads
 *   can return the byte held in the U3 Apple data-out latch. A latch holds a
 *   value until its next load. A pending event can hold RDY (Ready) low to
 *   make the Apple wait before finishing another access.
 *
 *   Mega II is the IIgs chip providing the Apple II memory and expansion bus.
 *   Host ticks count the IIgs 14.31818 MHz clock, about 70 ns per tick.
 *   A slow Apple bus cycle normally takes fourteen ticks. PHI is the Z180
 *   processor clock; one
 *   T-state is one PHI period and a half-PHI is one rising or falling edge step.
 *   BUSREQ asks the Z180 to give up its bus; BUSACK says it has done so. DMA
 *   (direct memory access) copies bytes without executing a CPU instruction.
 *   DREQ is a request input to one of the Z180's two DMA channels.
 *
 *   IRQ is an interrupt request. RTS is the
 *   serial request-to-send output, wired here to select read-only memory (ROM).
 *   EPROM is the erasable read-only chip containing the firmware. EEPROM
 *   is electrically erasable memory holding the video configuration.
 *
 *   The application owns SSCard. The worker alone runs chip state while a job
 *   is outstanding. Host reads, reset and presentation first synchronize().
 *   Sources: Second Sight manual, card operation; Jawaid
 *   Bazyar's ROM_BEEF.asm (Reset, ClearScreen and host-command routines), at
 *   https://github.com/jawaidbazyar2/secondsight . Zilog UM005004-0918 pp. 16-30
 *   and 95-113 describe processor bus ownership and DMA. Board grant lengths
 *   below are scheduling choices; those Zilog pages do not specify card delays.
 */

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <vector>
#include "ss_capture.hpp"
#include "z180/Z180.hpp"
#include "oti087.hpp"

// Owns the physical card chips and their shared bus schedule. Host accessors
// synchronize before exposing worker-owned state.
class SSCard {
public:
    // Board EPROM and SRAM are each 128 KiB; address decode mirrors them.
    static constexpr size_t ROM_SIZE = 0x20000;
    static constexpr size_t SRAM_SIZE = 0x20000;

    // Copy ROM_SIZE bytes from rom; ticks_per_second is the host clock rate.
    // lattice_map selects the U7 Lattice chip equations used for capture.
    explicit SSCard(const uint8_t *rom, uint64_t ticks_per_second, SSCapture::LatticeMap lattice_map = SSCapture::LatticeMap::BE8C);
    // Finish outstanding worker jobs before destroying the chips and callbacks.
    ~SSCard();
    // Start native asynchronous execution; browser execution stays synchronous.
    void start_worker();
    // Wait for all submitted jobs and deliver their Apple IRQ edges on the host.
    void synchronize();
    // Reset at an absolute host tick; power_on also clears RAM and card latches.
    void reset(uint64_t ticks, bool power_on = false);
    // Install a ROM_SIZE-byte image and capture map at ticks as a power cycle.
    void change_hardware(uint64_t ticks, const uint8_t *image, SSCapture::LatticeMap lattice_map);
    // Observe one Apple access; update read data and return extra RDY wait ticks.
    uint64_t observe(megaii_transfer_t &transfer);
    // Drain captured accesses and run the card through this absolute host tick.
    void catch_up(uint64_t ticks);
#ifdef GS2_SS_TESTING
    // Fixture switch: execute each transfer separately when disabled.
    void set_batching(bool enabled) { batch_execution = enabled; }
#endif
    // CPU bus callbacks: physical 20-bit byte addresses, with device side effects.
    uint8_t memory_read(uint32_t address);
    // Drive data and store through the same physical address decoder as reads.
    void memory_write(uint32_t address, uint8_t data);
    // Read a Z180 I/O port, including board acknowledgement and status effects.
    uint8_t io_read(uint16_t port);
    // Write a byte to a Z180 I/O port; board ports decode their low address byte.
    void io_write(uint16_t port, uint8_t data);

    // Host inspection: wait before borrowing the capture state; do not retain
    // this reference across a later asynchronous submission.
    const SSCapture &capture_state() {
        synchronize();
        return capture;
    }
    // Borrow the stopped processor for debugger/fixture access after draining jobs.
    Z180 &processor() {
        synchronize();
        return *cpu;
    }
    // Borrow video state after draining jobs; display use is on the main thread.
    OTI087 &video() {
        synchronize();
        return vga;
    }
    // Borrow the stopped card RAM for inspection, indexed by physical SRAM byte.
    const std::array<uint8_t, SRAM_SIZE> &sram_data() {
        synchronize();
        return sram;
    }
    // The following observations require a stopped/synchronized card. This byte
    // is held by the U3 Apple data-out latch for Apple reads of $C0B0-$C0BF.
    uint8_t output_latch() const { return host_output; }
    // Interrupt acknowledge data: use all ones until any device drives the bus.
    uint8_t interrupt_bus_byte() const { return bus_valid ? bus_data : 0xff; }
    // Whether the retained card bus byte has a known driver since power-on.
    bool data_bus_driven() const { return bus_valid; }
    // True maps ROM over the low 128 KiB while firmware starts up.
    bool overlay_enabled() const { return rom_overlay; }
    // Last observed host tick, including any returned RDY wait.
    uint64_t host_ticks() const { return observed_ticks; }
    // Host ticks per second; used to convert Apple time to processor time.
    uint64_t host_tick_rate() const { return ticks_per_second; }
    // Diagnostic totals: copied bytes, firmware events, held accesses, held ticks.
    uint64_t copies = 0, events = 0, stalls = 0, stall_ticks = 0;
    // Main-thread callback: true asserts the card interrupt on the Apple CPU.
    std::function<void(bool)> irq_handler;

private:
    // 36 MHz board oscillator; Z8S180 reset divides EXTAL by two.
    static constexpr uint32_t EXTAL_HZ = 36'000'000;
    // The scheduler uses fourteen host ticks for a nominal slow Mega II cycle.
    static constexpr uint64_t MEGAII_CYCLE_TICKS = 14;
    // Modelled U17 copy controller grant duration in PHI, including the write.
    static constexpr unsigned COPY_PHI = 7;
    // Host acknowledgement write timing, in processor PHI periods.
    static constexpr unsigned ACK_WRITE_PHI = 2;
    // Scheduler upper bound only; the acknowledge edge can end a slice sooner.
    static constexpr unsigned HANDSHAKE_SLICE_PHI = 128;
    std::array<uint8_t, ROM_SIZE> rom{};
    std::array<uint8_t, SRAM_SIZE> sram{};
    SSCapture capture;
    OTI087 vga;
    std::unique_ptr<Z180> cpu;
    std::vector<megaii_transfer_t> transfers;
    // Host allocation/batch capacity, not a FIFO present on the card.
    static constexpr size_t QUEUE_SIZE = 512;
    struct Worker;
    std::unique_ptr<Worker> worker;
    bool work_pending = false, host_pending = false, host_irq = false;
    bool queued_event = false;
#ifdef GS2_SS_TESTING
    bool batch_execution = true;
#else
    static constexpr bool batch_execution = true;
#endif
    bool rom_overlay = true, vga_half = false, linear = false, rgb = false, apple_irq = false;
    uint8_t bus_data = 0xff, host_output = 0;
    bool bus_valid = false;
    uint64_t ticks_per_second;
    uint64_t observed_ticks = 0;
    uint64_t next_transfer_tick = 0;
    bool bus_request = false, ax_selected = false, ack_selected = false;
    uint8_t ax_nibble = 0, copy_data = 0;
    uint32_t copy_address = 0;
    Z180Time catch_until, host_epoch;
    uint32_t cpu_hz = EXTAL_HZ / 2;
    double wait_seconds[2] = {-1, -1};
    unsigned wait_phi[2]{};
    uint32_t wait_rate[2]{};
    double event_seconds = -1;
    Z180Time event_time;
    uint64_t transfer_epoch_tick = UINT64_MAX;
    uint64_t transfer_period = 0;
    Z180Time transfer_epoch_time;

    // Absolute board time = host reset epoch + elapsed processor time.
    Z180Time card_time() const { return host_epoch + cpu->time(); }
    // Same time in seconds for the video model, whose deadlines use doubles.
    double cpu_time() const { return card_time().as_seconds(); }
    // Publish stable ROM/SRAM read pages to the CPU after overlay/address changes.
    // Pages selecting a device retain callbacks so their side effects are not skipped.
    void update_direct_reads();
    // Finish the pending U17 copy controller SRAM write and release BUSREQ.
    // The CPU must already have granted the bus before this deadline can complete.
    void complete_copy();
    // Try to schedule the copy deadline inside the current CPU grant; return
    // false when run_to must service it explicitly instead.
    bool schedule_copy_release();
    // Convert a video event in absolute seconds to integer card time, rounding
    // forward. Cache unchanged deadlines to avoid repeated floating-point conversion.
    Z180Time vga_deadline(double event);
    // Convert absolute host ticks to card time using a retained whole-second epoch.
    Z180Time transfer_deadline(uint64_t ticks);
    // Accept a new processor PHI rate hz and invalidate cached wait conversions.
    void clock_changed(uint32_t hz);
    // Advance CPU and video up to an absolute card deadline; awaiting_ack stops
    // once the selected host acknowledgement permits the Apple to continue.
    void run_to(Z180Time deadline, bool awaiting_ack = false);
    // Run to this host access and apply it synchronously; return extra Apple
    // RDY wait ticks and update transfer.data/driven for a card-driven read.
    uint64_t process(megaii_transfer_t &transfer);
    // Decode and accept transfer at absolute card timestamp; return host wait
    // ticks and retain any event or SRAM copy until its hardware hand-off completes.
    uint64_t accept_transfer(megaii_transfer_t &transfer, Z180Time timestamp);
    // Process queue in order and run through absolute host ticks. Queued
    // accesses cannot return an unreported RDY delay; such a delay is an invariant failure.
    void run_transfers(std::vector<megaii_transfer_t> &queue, uint64_t ticks);
    // Submit queued accesses through host ticks. If wait is true, return only
    // after completion; access is an optional final synchronous access. Return its
    // RDY wait ticks. Caller storage must survive the submitted job.
    uint64_t dispatch(uint64_t ticks, bool wait, megaii_transfer_t *access = nullptr);
    // Record an asserted/deasserted Apple interrupt request on the worker;
    // synchronize delivers it to the host CPU in order.
    void publish_irq(bool asserted);
    // Translate the Z180 video window address plus board half/linear selections
    // into an Oak address. This changes address wiring, not memory contents.
    uint32_t oak_address(uint32_t address) const;
    // Present the retained event to Z180 INT1 and DREQ1 until firmware acknowledges it.
    void drive_event();
    // Apply low-three-bit board port selection with data on the card bus.
    // The IORQ select edge can acknowledge an event before the data sample.
    void strobe(unsigned port, uint8_t data);
    // Return full PHI wait clocks for physical address/access, combining the
    // board access delay with current CPU clock conversion (UM0050 pp. 28-30).
    unsigned extra_wait(Z180::Access access, uint32_t address);
};
