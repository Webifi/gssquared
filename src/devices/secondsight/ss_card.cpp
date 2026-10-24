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

#include "ss_card.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <cstdio>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

// The worker owns card state while processing a batch. The main thread
// drains preceding batches before reading that state, presenting video or
// applying Apple IRQ transitions. SDL and IIgs state stay on the main thread.
struct SSCard::Worker {
    // One host submission: replay these Apple transfers in order, then run
    // through ticks. access, when present, is a final synchronous read/write.
    // Its pointers refer to dispatch()'s caller and stack, so dispatch must
    // wait for this sequence before returning. The deque owns queued jobs;
    // the worker moves one out and owns it until completed is published.
    struct Job {
        std::vector<megaii_transfer_t> transfers;
        uint64_t ticks;
        megaii_transfer_t *access;
        uint64_t *held;
        uint64_t sequence;
    };
    // The mutex protects queue ownership and submitted/completed counters.
    // Publishing completed under the lock makes all chip writes visible to
    // synchronize(). Only the worker writes error and irq during a job.
    std::mutex mutex;
    std::condition_variable ready, finished;
    std::deque<Job> jobs;
    std::vector<bool> irq;
    std::thread thread;
    const std::thread::id host_thread = std::this_thread::get_id();
    std::exception_ptr error;
    uint64_t submitted = 0, completed = 0;
    bool stop = false;
};

// Connect the processor pins to the board decode and VGA chip. The serial RTS
// output selects the ROM overlay, so its callback must update direct-read pages.
SSCard::SSCard(const uint8_t *image, uint64_t rate, SSCapture::LatticeMap lattice_map) : capture(lattice_map), ticks_per_second(rate) {
    // image supplies exactly ROM_SIZE bytes; rate is host ticks per second.
    // Keep a truncated integer tick period, matching Z180Time::from_ticks.
    // Zero rate is handled as an unreachable deadline by transfer_deadline().
    if (uint32_t(rate))
        transfer_period = Z180Time::units_per_second / uint32_t(rate);
    std::memcpy(rom.data(), image, rom.size());
    transfers.reserve(QUEUE_SIZE);
    // The CPU has no knowledge of Apple addresses or video chips. These
    // callbacks translate its physical memory and I/O cycles into card actions.
    // An interrupt acknowledge reads whatever byte the card bus last held.
    Z180::Bus bus;
    bus.memory_read = [this](uint32_t a) { return memory_read(a); };
    bus.memory_write = [this](uint32_t a, uint8_t d) { memory_write(a, d); };
    bus.io_read = [this](uint16_t p) { return io_read(p); };
    bus.io_write = [this](uint16_t p, uint8_t d) { io_write(p, d); };
    bus.irq_ack = [this] { return interrupt_bus_byte(); };
    bus.clock_changed = [this](uint32_t hz) { clock_changed(hz); };
    cpu = std::make_unique<Z180>(std::move(bus), EXTAL_HZ, Z180::Silicon::K);
    cpu->serial(0).rts_handler = [this](bool level) {
        rom_overlay = level;
        update_direct_reads();
    };
    cpu->set_asci_idle_clock_gating(true);
    cpu->set_idle_skipping(true);
    // Idle-loop skipping may inspect only stable ROM/SRAM. Return -1 for
    // addresses with device effects, so the CPU executes those reads normally.
    cpu->set_idle_memory([this](uint32_t address, bool drive) -> int {
        if (address == 0x100030)
            return bus_data; // DMA status (DSTAT) is internal; it leaves the card data bus unchanged.
        address = cpu->a18_tout_address(address);
        if (address >= 0x40000 && (address < 0x60000 || address >= 0x80000))
            return -1;
        uint8_t data = ((address < 0x20000 && rom_overlay) || address >= 0x60000) ? rom[address & 0x1ffff]
                                                                                  : sram[address & 0x1ffff];
        if (drive) {
            bus_data = data;
            bus_valid = true;
        }
        return data;
    });
    cpu->set_bus_wait([this](uint32_t a, bool write, bool io) {
        return io ? 0U : extra_wait(write ? Z180::Access::MemoryWrite : Z180::Access::MemoryRead, a);
    });
    // IORQ is the Z180 I/O-request signal. Its select and release edges
    // matter separately from the data read/write (UM0050 pp. 16-17).
    cpu->set_io_strobe([this](uint32_t port, bool write, uint8_t data, bool asserted) {
        if ((port & 0xf8) != 0xa0)
            return;
        ax_selected = asserted;
        unsigned reg = port & 7;
        if (reg == 0) {
            // BCLK, the U4 Z180 I/O decoder latch clock, rises at the final
            // write-data edge. This also applies
            // when relocated internal I/O owns the CPU's data transfer.
            if (!asserted && write)
                host_output = data;
            ack_selected = asserted;
            if (!asserted)
                cpu->request_yield();
        }
        if (!asserted)
            return;
        if (reg >= 4)
            ax_nibble = capture.status(reg, 0) & 15;
        else
            strobe(reg, write ? data : bus_data);
    });
    // Do not release a copy grant before the CPU has actually surrendered
    // the bus. Leave at least ACK_WRITE_PHI for the SRAM write after BUSACK.
    cpu->set_busack([this](bool asserted) {
        if (!asserted || !bus_request)
            return;
        catch_until = std::max(catch_until, card_time() + cpu->phi_time(ACK_WRITE_PHI));
        if (!schedule_copy_release())
            cpu->request_yield();
    });
    cpu->set_bus_release_complete([this] { complete_copy(); });
    vga.cint_callback = [this](bool asserted) { cpu->set_input(Z180::Input::INT0, asserted); };
    reset(0, true);
}

// Join the worker before its bus callbacks or card storage can be destroyed.
SSCard::~SSCard() {
    if (worker) {
        {
            std::lock_guard<std::mutex> lock(worker->mutex);
            worker->stop = true;
        }
        worker->ready.notify_one();
        if (worker->thread.joinable())
            worker->thread.join();
    }
}

// Start the optional card execution thread once. It consumes jobs submitted
// by dispatch(); no parameters or result are needed. Browser builds execute
// the same work on the main thread because waiting there would spin.
void SSCard::start_worker() {
#ifdef __EMSCRIPTEN__
    // Browser-main-thread condition waits spin. The same batch scheduler can
    // execute locally with the same event deadlines and synchronous host reads.
    return;
#endif
    if (worker)
        return;
    worker = std::make_unique<Worker>();
    host_pending = capture.pending;
    host_irq = apple_irq;
    worker->thread = std::thread([this] {
        // Sleep until a job or shutdown arrives. During shutdown, finish
        // queued jobs in order; exit only when the queue has become empty.
        for (;;) {
            Worker::Job job;
            {
                std::unique_lock<std::mutex> lock(worker->mutex);
                worker->ready.wait(lock, [this] { return worker->stop || !worker->jobs.empty(); });
                if (worker->jobs.empty())
                    return;
                // Take ownership under the lock, then release it before
                // running the CPU. The main thread can enqueue the next job.
                job = std::move(worker->jobs.front());
                worker->jobs.pop_front();
            }
            if (!worker->error) {
                try {
                    // Replay earlier Apple accesses, then advance the card to
                    // the job's host time. process() handles the final access
                    // whose read byte or RDY wait the main thread needs back.
                    run_transfers(job.transfers, job.ticks);
                    if (job.access)
                        *job.held = process(*job.access);
                } catch (...) {
                    // An exception escaping std::thread would terminate the
                    // application. Keep the first failure for synchronize()
                    // to rethrow on the host; later jobs only retire their
                    // sequence numbers so a waiting host cannot deadlock.
                    worker->error = std::current_exception();
                }
            }
            {
                std::lock_guard<std::mutex> lock(worker->mutex);
                // Publish the result before waking the host. Its predicate
                // waits for all submitted jobs, not merely one notification.
                worker->completed = job.sequence;
            }
            worker->finished.notify_one();
        }
    });
}

// Drain all submitted transfers before exposing card state to the host.
// Publish Apple IRQ edges here because the worker cannot modify the IIgs CPU.
void SSCard::synchronize() {
    if (!work_pending)
        return;
    {
        std::unique_lock<std::mutex> lock(worker->mutex);
        worker->finished.wait(lock, [this] { return worker->completed == worker->submitted; });
    }
    work_pending = false;
    // The completed/submitted wait above is also the memory hand-off for
    // error, capture and IRQ (interrupt request) changes made by the worker.
    if (worker->error)
        std::rethrow_exception(worker->error);
    host_pending = capture.pending;
    host_irq = apple_irq;
    for (bool asserted : worker->irq)
        if (irq_handler)
            irq_handler(asserted);
    worker->irq.clear();
}

// Keep IRQ edges in order while the worker owns the card; the host applies them
// at synchronization rather than racing the IIgs interrupt controller.
void SSCard::publish_irq(bool asserted) {
    if (worker && worker->host_thread != std::this_thread::get_id())
        worker->irq.push_back(asserted);
    else if (irq_handler)
        irq_handler(asserted);
}

// Submit captured bus transfers through the same scheduler in either mode.
// An access that returns data or stretches RDY must wait for its result.
uint64_t SSCard::dispatch(uint64_t ticks, bool wait, megaii_transfer_t *access) {
    uint64_t held = 0;
    if (worker) {
        // ticks is an absolute host-clock count. wait requests a full drain;
        // access must be paired with wait=true because its result pointers
        // remain valid only until this call returns. held is in host ticks.
        Worker::Job job{{}, ticks, access, access ? &held : nullptr, 0};
        job.transfers.swap(transfers);
        transfers.reserve(QUEUE_SIZE);
        {
            std::lock_guard<std::mutex> lock(worker->mutex);
            job.sequence = ++worker->submitted;
            worker->jobs.push_back(std::move(job));
        }
        work_pending = true;
        worker->ready.notify_one();
        if (wait)
            synchronize();
    } else {
        run_transfers(transfers, ticks);
        transfers.clear();
        if (access)
            held = process(*access);
    }
    return held;
}

// Replace the EPROM and Lattice map as a power cycle; keep the display and
// nonvolatile EEPROM objects that the application already owns.
void SSCard::change_hardware(uint64_t ticks, const uint8_t *image, SSCapture::LatticeMap lattice_map) {
    synchronize();
    std::memcpy(rom.data(), image, rom.size());
    capture.lattice_map = lattice_map;
    reset(ticks, true);
}

// Apple reset restarts the Z180, but does not clear the Lattice chip or host
// latches. Only power-on clears SRAM and those board-level retained outputs.
void SSCard::reset(uint64_t ticks, bool power_on) {
    synchronize();
    if (!power_on)
        catch_up(ticks);
    transfers.clear();
    queued_event = false;
    observed_ticks = ticks;
    next_transfer_tick = ticks;
    // ticks anchors the reset to host time. A power cycle initializes RAM
    // and retained card latches; an Apple reset only restarts the processor.
    // Zero-filled SRAM is a deterministic emulator choice, not power-up data.
    if (power_on) {
        sram.fill(0);
        capture.power_on();
        vga_half = linear = rgb = apple_irq = false;
        bus_data = 0xff;
        bus_valid = false;
        host_output = 0;
        copies = events = stalls = stall_ticks = 0;
    }
    // U7 Lattice chip and the U4 Z180 I/O decoder/U15 Z180 memory decoder outputs have no Apple reset input.
    rom_overlay = true;
    bus_request = ax_selected = ack_selected = false;
    cpu->reset();
    // The card holds DMA request 0 active. Fixed-source memory fills need
    // that request (UM0050 pp. 105-109; ROM_BEEF.asm ClearScreen).
    cpu->set_input(Z180::Input::DREQ0, true);
    update_direct_reads();
    host_epoch = Z180Time::from_ticks(ticks, ticks_per_second) - cpu->time();
    cpu_hz = cpu->clock_hz();
    vga.reset();
    vga.output_divider_w(rgb);
    vga.advance(double(cpu_time()));
    drive_event();
    host_pending = capture.pending;
    host_irq = apple_irq;
    if (irq_handler)
        irq_handler(apple_irq);
}

// hz is the new PHI frequency in cycles per second. Discard cached wait
// lengths and yield so the card can convert deadlines at that rate.
// Zilog UM0050 pp. 34-41 describe the clock multiplier and divider.
void SSCard::clock_changed(uint32_t hz) {
    cpu_hz = hz;
    wait_seconds[0] = wait_seconds[1] = -1;
    if (cpu)
        cpu->request_yield();
}

// Run only to the next host, VGA or copy-release boundary. Acknowledgement
// waits stop at the port strobe so the Apple can resume at the proper PH0 edge.
void SSCard::run_to(Z180Time deadline, bool awaiting_ack) {
    // deadline is absolute emulated time. awaiting_ack allows an earlier
    // return as soon as the firmware releases its acknowledge port. Each
    // iteration stops at the earliest video edge, copy completion or deadline;
    // no chip may run past a change that another chip must observe.
    while (card_time() < deadline && (!awaiting_ack || capture.pending || ack_selected)) {
        double const vga_event = vga.next_event();
        Z180Time const event_deadline = vga_deadline(vga_event);
        Z180Time end = std::min(deadline, event_deadline);
        if (bus_request && cpu->bus_acknowledged())
            end = std::min(end, catch_until);
        if (end > card_time())
            cpu->run_half_until(cpu->half_cycles() + (end - card_time()).ceil_ticks(cpu_hz * 2));
        if (bus_request && cpu->bus_acknowledged() && card_time() >= catch_until)
            complete_copy();
        // If the exact integer deadline has passed, retain the original VGA
        // double for dispatch; converting card time back must not round below it.
        vga.advance(std::max(double(cpu_time()), card_time() >= event_deadline ? vga_event : 0.0));
    }
}

// Expose only ROM/SRAM pages whose reads have no board-side decode action;
// the CPU must still update the retained data-bus byte on each direct read.
void SSCard::update_direct_reads() {
    for (unsigned page = 0; page < 4; ++page)
        cpu->set_direct_read_page(page, (rom_overlay && page < 2 ? rom.data() : sram.data()) + ((page & 1) << 16),
                                  bus_data, &bus_valid);
    for (unsigned page = 6; page < 8; ++page)
        cpu->set_direct_read_page(page, rom.data() + ((page & 1) << 16), bus_data, &bus_valid);
}

// Convert the copy controller deadline to a CPU half-PHI edge without rounding
// earlier; BUSREQ releases only after the SRAM write has completed.
bool SSCard::schedule_copy_release() {
    auto const now = card_time();
    return catch_until > now &&
           cpu->schedule_bus_release(cpu->half_cycles() + (catch_until - now).ceil_ticks(cpu_hz * 2));
}

// Bridge the VGA raster clock to integer CPU time. Cache by the exact double
// value so unchanged raster deadlines have identical rounding.
Z180Time SSCard::vga_deadline(double event) {
    // Timing-register writes and raster events can change the deadline. Its
    // binary64 value is the complete conversion input, including after reset.
    if (event_seconds != event) {
        event_seconds = event;
        event_time = Z180Time::from_seconds(event);
    }
    return event_time;
}

// Translate Mega II ticks to card time using the same truncated oscillator
// period as from_ticks; batching must not change when a bus access occurs.
Z180Time SSCard::transfer_deadline(uint64_t ticks) {
    // Retain from_ticks' whole-second boundary and truncated tick period.
    // Consecutive captured bytes normally share this host-clock second.
    const uint32_t rate = ticks_per_second;
    if (!rate)
        return Z180Time::never;
    if (ticks < transfer_epoch_tick || ticks - transfer_epoch_tick >= rate) {
        const uint64_t seconds = ticks / rate;
        transfer_epoch_tick = seconds * rate;
        transfer_epoch_time = Z180Time(seconds);
    }
    return transfer_epoch_time + Z180Time(0, (ticks - transfer_epoch_tick) * transfer_period);
}

// Commit the Apple data-in latch to SRAM at the end of the copy-controller
// grant, then release the Z180 bus. See UM0050 pp. 28-30 for BUSREQ/BUSACK.
void SSCard::complete_copy() {
    sram[copy_address] = copy_data;
    bus_data = copy_data;
    bus_valid = true;
    ++copies;
    bus_request = false;
    cpu->bus_release_w(false);
}

// The U7 Lattice chip pending-event output feeds both Z180 INT1 and DMA request 1;
// acknowledging the host port must drop both signals together.
void SSCard::drive_event() {
    cpu->set_input(Z180::Input::INT1, capture.pending);
    cpu->set_input(Z180::Input::DREQ1, capture.pending);
}

// Translate a Z180 byte address to the address presented to Oak. An aperture
// is the memory range through which the CPU reaches video RAM. Board latches
// provide its upper address bits; Oak's own bank registers act afterwards.
uint32_t SSCard::oak_address(uint32_t address) const {
    return (linear ? 0x100000 : 0) | (vga_half ? 0x80000 : 0) | (address & 0x7ffff);
}

// Convert Oak readiness to external WAIT duration. The memory decoder
// wait overlaps programmed CPU waits rather than adding a second wait train.
unsigned SSCard::extra_wait(Z180::Access access, uint32_t address) {
    if (access != Z180::Access::MemoryRead && access != Z180::Access::MemoryWrite)
        return 0;
    address = cpu->a18_tout_address(address);
    if (address < 0x80000)
        return 0;
    // U15 Z180 memory decoder waits overlap DCNTL waits; UM0050 p. 30 samples WAIT at T2.
    // Return full PHI periods of external wait. Round the half-PHI count up
    // to avoid allowing a CPU access before the video chip is ready.
    const unsigned write = access == Z180::Access::MemoryWrite;
    const double seconds = vga.access_wait(write);
    const uint32_t rate = cpu->clock_hz();
    if (wait_seconds[write] != seconds || wait_rate[write] != rate) {
        const auto ticks = cpu->time_to_cycles(Z180Time::from_seconds(seconds));
        wait_phi[write] = unsigned((ticks + 1) / 2);
        wait_seconds[write] = seconds;
        wait_rate[write] = rate;
    }
    return wait_phi[write];
}

// Read a 20-bit Z180 physical byte address. Return the byte driven by ROM,
// SRAM or Oak; an unselected device leaves the previous bus byte in place.
// ROM_BEEF.asm Reset uses the overlay to copy ROM into RAM, then removes it.
// Map: 00000-1FFFF ROM or SRAM, 20000-3FFFF SRAM mirror, 40000-5FFFF
// half-select latch, 60000-7FFFF ROM mirror, 80000-FFFFF Oak aperture.
uint8_t SSCard::memory_read(uint32_t address) {
    address = cpu->a18_tout_address(address & 0xfffff);
    if ((address < 0x20000 && rom_overlay) || (address >= 0x60000 && address < 0x80000)) {
        bus_data = rom[address & 0x1ffff];
        bus_valid = true;
    } else if (address < 0x40000) {
        bus_data = sram[address & 0x1ffff];
        bus_valid = true;
    } else if (address < 0x60000) {
        vga_half = address & 0x10000;
    } else {
        vga.advance(double(cpu_time()));
        if (vga.memory_read_selected(oak_address(address))) {
            bus_data = vga.memory_read(oak_address(address)).data;
            bus_valid = true;
        }
    }
    return bus_data;
}

// Write data to a 20-bit physical byte address, using the map above. A CPU
// write drives the data bus even if ROM is selected and discards the store.
// Address bit 18 can instead be the timer output pin (UM0050 pp. 157-164).
void SSCard::memory_write(uint32_t address, uint8_t data) {
    address = cpu->a18_tout_address(address & 0xfffff);
    bus_data = data;
    bus_valid = true;
    if ((address < 0x20000 && rom_overlay) || (address >= 0x60000 && address < 0x80000))
        return;
    if (address < 0x40000)
        sram[address & 0x1ffff] = data;
    else if (address < 0x60000)
        vga_half = address & 0x10000;
    else {
        vga.advance(double(cpu_time()));
        vga.memory_write(oak_address(address), data);
    }
}

// Decode the low nibble of Z180 ports $A0-$AF: $A0 acknowledges the host,
// $A1 bit 2 inhibits copies and bit 3 enables IIgs bank selection. $A2/$A3
// have no action here. $A4/$A5 read and clear dirty-band bits; $A6/$A7 read
// the event address. $A8/$A9 select VGA/RGB timing; $AA/$AB disable/enable
// linear addressing; $AC/$AD assert/release Apple IRQ; $AE/$AF do nothing.
// port is the low nibble, data is the driven byte. Port selection itself
// acts, on reads too. See ROM_BEEF.asm Reset and its IN0/OUT0 $A0-$AD uses.
void SSCard::strobe(unsigned port, uint8_t data) {
    switch (port) {
        case 0: {
            bool const was_pending = capture.pending;
            capture.acknowledge();
            drive_event();
            if (was_pending)
                cpu->request_yield();
            break;
        }
        case 1:
            capture.configure(data);
            break;
        case 8:
        case 9:
            rgb = port & 1;
            vga.output_divider_w(rgb);
            cpu->request_yield();
            break;
        case 10:
        case 11:
            linear = port & 1;
            break;
        case 12:
        case 13:
            apple_irq = !(port & 1);
            publish_irq(apple_irq);
            break;
    }
}

// Return the Apple data-in latch at $A0 or capture status at $A4-$A7.
// Other VGA-selected ports go to Oak; unselected bits retain the data bus.
uint8_t SSCard::io_read(uint16_t port) {
    vga.advance(double(cpu_time()));
    if ((port & 0xf0) == 0xa0) {
        const unsigned reg = port & 15;
        const uint8_t data = reg == 0 ? capture.host_input
                             : reg >= 4 && reg <= 7
                                 ? (ax_selected ? (bus_data & 0xf0) | ax_nibble : capture.status(reg, bus_data))
                                 : bus_data;
        if (reg != 0 || !ack_selected)
            strobe(reg, bus_data);
        bus_data = data;
        if (reg == 0 || (reg >= 4 && reg <= 7))
            bus_valid = true;
    } else if (vga.io_read_selected(port)) {
        bus_data = vga.io_r(port);
        bus_valid = true;
    }
    return bus_data;
}

// Apply board port strobes or forward VGA/DAC register writes to Oak.
// The $A0 output latch uses the IORQ release edge during real CPU cycles.
void SSCard::io_write(uint16_t port, uint8_t data) {
    vga.advance(double(cpu_time()));
    bus_data = data;
    bus_valid = true;
    if ((port & 0xf0) == 0xa0) {
        const unsigned reg = port & 15;
        // Direct fixture/debug accesses complete here. Machine cycles load
        // U3 Apple data-out latch and release ACK through the IORQ edge callback.
        if (reg == 0 && !ack_selected)
            host_output = data;
        if (reg >= 4 && reg <= 7)
            bus_data = ax_selected ? (data & 0xf0) | ax_nibble : capture.status(reg, data);
        else if (reg != 0 || !ack_selected)
            strobe(reg, data);
    } else {
        vga.io_w(port, data);
        cpu->request_yield();
    }
}

// Bring the card to the host transfer time before evaluating its bus pins.
uint64_t SSCard::process(megaii_transfer_t &transfer) {
    const auto timestamp = Z180Time::from_ticks(transfer.ticks, ticks_per_second);
    run_to(timestamp);
    return accept_transfer(transfer, timestamp);
}

// Finish any selected acknowledgement, hold the Apple if an event is pending,
// then latch this transfer. A video write starts a copy-controller BUSREQ grant.
uint64_t SSCard::accept_transfer(megaii_transfer_t &transfer, Z180Time timestamp) {
    // transfer carries the Apple address, data and select signals at timestamp.
    // The result says whether to copy a byte, notify firmware or drive a read.
    // Return the extra host ticks spent holding RDY; the memory mapper adds
    // them before it completes the Apple access.
    const auto result = capture.transfer(transfer);
    // The MMU calls before advancing the host bus cycle. A0 select clears
    // pending before its data edge samples U2 Apple data-in latch or loads U3 Apple data-out latch. Finish that selected
    // cycle before accepting another transfer; no host wait is added here.
    if (result.copy || result.event || result.drive)
        while (ack_selected)
            run_to(card_time() + cpu->phi_time(HANDSHAKE_SLICE_PHI), true);
    uint64_t held = 0;
    if (result.stall) {
        ++stalls;
        while (capture.pending || ack_selected) {
            // Video and bus-grant deadlines still apply while the Apple is held.
            // Stop when ACK deasserts, not at the end of this upper bound.
            run_to(card_time() + cpu->phi_time(HANDSHAKE_SLICE_PHI), true);
        }
        // ACK is the firmware acknowledgement; PH0 is the Apple bus clock.
        // Round up to a whole Apple cycle, then allow one complete low/high
        // cycle before releasing RDY. This is the card timing model, not a
        // Z180 timing requirement.
        const uint64_t ack = card_time().ceil_ticks(ticks_per_second);
        const uint64_t release = ((ack + MEGAII_CYCLE_TICKS - 1) / MEGAII_CYCLE_TICKS + 1) * MEGAII_CYCLE_TICKS;
        held = release > transfer.ticks ? release - transfer.ticks : 0;
        run_to(Z180Time::from_ticks(transfer.ticks + held, ticks_per_second));
        stall_ticks += held;
    }
    if (result.drive) {
        transfer.data = host_output;
        transfer.driven = true;
    }
    capture.complete(transfer, result);
    if (result.copy) {
        // The controller has only one captured byte. Overlapping grants mean
        // the scheduler would overwrite that byte, so stop instead of silently
        // corrupting the card's copy of Apple video memory.
        if (bus_request) {
            std::fprintf(
                stderr,
                "SS grant overlap host=%llu old=%05x new=%04x now=%.18f grant=%.18f ack=%d pc=%04x half=%llu\n",
                transfer.ticks, copy_address, transfer.address, cpu_time(), catch_until.as_seconds(),
                cpu->bus_acknowledged(), cpu->get_state().pc, cpu->half_cycles());
            throw std::runtime_error("Second Sight: overlapping SRAM grants");
        }
        copy_address = (result.bank ? 0x10000 : 0) | transfer.address;
        copy_data = capture.host_input;
        bus_request = true;
        catch_until = (held ? Z180Time::from_ticks(transfer.ticks + held, ticks_per_second) : timestamp) +
                      cpu->phi_time(COPY_PHI);
        cpu->bus_release_w(true);
    }
    if (result.event) {
        ++events;
        drive_event();
    }
    return held;
}

// Meet the host at a frame/reset/debug boundary before reading shared state.
void SSCard::catch_up(uint64_t ticks) {
    observed_ticks = ticks;
    dispatch(ticks, true);
    queued_event = false;
}

// Batch execution only between observable events: host transfers, copy
// release and raster events. A pending handshake returns to the synchronous
// path with the CPU machine-cycle phase intact.
void SSCard::run_transfers(std::vector<megaii_transfer_t> &queue, uint64_t ticks) {
    size_t next = 0;
    if (batch_execution && !queue.empty()) {
        const auto deadline = Z180Time::from_ticks(std::max(ticks, queue.back().ticks), ticks_per_second);
        auto transfer_time = transfer_deadline(queue[0].ticks);
        while (next < queue.size() || card_time() < deadline) {
            bool blocked = false;
            auto scheduler = [&]() -> uint64_t {
                auto now = card_time();
                if (bus_request && cpu->bus_acknowledged()) {
                    if (now >= catch_until)
                        complete_copy();
                    // Re-evaluate at board event deadlines: integer clock epochs
                    // can change the rounding of an absolute board deadline.
                    else
                        schedule_copy_release();
                }
                double const event = vga.next_event();
                if (now >= vga_deadline(event))
                    vga.advance(std::max(double(now.as_seconds()), event));
                if (next < queue.size() && now >= transfer_time) {
                    // RDY and an already selected A0 cycle need the ordinary
                    // synchronous handshake. Leave the CPU's exact phase intact.
                    if (capture.pending || ack_selected) {
                        blocked = true;
                        return cpu->half_cycles();
                    }
                    if (accept_transfer(queue[next++], transfer_time))
                        throw std::runtime_error("Second Sight: queued RDY access escaped arbitration");
                    if (next < queue.size())
                        transfer_time = transfer_deadline(queue[next].ticks);
                }
                auto end = std::min(deadline, vga_deadline(vga.next_event()));
                if (next < queue.size())
                    end = std::min(end, transfer_time);
                if (bus_request && cpu->bus_acknowledged() && !cpu->bus_release_scheduled())
                    end = std::min(end, catch_until);
                return cpu->half_cycles() + (end > now ? (end - now).ceil_ticks(cpu_hz * 2) : 0);
            };
            // The CPU calls this scheduler at each external deadline. Its closure
            // refers to this stack frame, so remove it on both success and error.
            cpu->set_scheduler([&scheduler] { return scheduler(); });
            try {
                // At a final transfer boundary the scheduler must still accept
                // that transfer, even though no further CPU edge is needed.
                auto const now = card_time();
                cpu->run_half_until(cpu->half_cycles() +
                                    (deadline > now ? (deadline - now).ceil_ticks(cpu_hz * 2) : 1));
            } catch (...) {
                cpu->set_scheduler({});
                throw;
            }
            cpu->set_scheduler({});
            if (bus_request && cpu->bus_acknowledged() && card_time() >= catch_until)
                complete_copy();
            double const event = vga.next_event();
            vga.advance(std::max(double(cpu_time()), card_time() >= vga_deadline(event) ? event : 0.0));
            if (blocked) {
                if (process(queue[next++]))
                    throw std::runtime_error("Second Sight: queued RDY access escaped arbitration");
                if (next < queue.size())
                    transfer_time = transfer_deadline(queue[next].ticks);
            }
        }
    }
    for (; next < queue.size(); ++next) {
        // The observer drains before a possible held access. Queued transfers
        // must not introduce an unreported RDY hold; virtual DMA spacing stays
        // solely in the card schedule.
        if (process(queue[next]))
            throw std::runtime_error("Second Sight: queued RDY access escaped arbitration");
    }
    run_to(Z180Time::from_ticks(ticks, ticks_per_second));
}

// Accept a Mega II transfer before the MMU completes it. Readback, IRQ and
// possible RDY holds drain the queue; independent writes may be batched.
uint64_t SSCard::observe(megaii_transfer_t &host_transfer) {
    observed_ticks = host_transfer.ticks;
    // Virtual disk DMA may submit a whole block at one host tick. Mega II
    // can carry one byte per cycle: assign those bytes successive card-side
    // event times. This does not advance the IIgs clock or drop a byte.
    megaii_transfer_t transfer = host_transfer;
    transfer.ticks = std::max(transfer.ticks, next_transfer_tick);
    next_transfer_tick = transfer.ticks + MEGAII_CYCLE_TICKS;
    const uint16_t a = transfer.address;
    // These broad address checks decide when batching is safe. The U7
    // Lattice chip still performs the exact decode in accept_transfer().
    // A queued event may become pending, so the next possible copy or event
    // must wait for the worker before the Apple is allowed to continue.
    const bool host = (a & 0xfff0) == 0xc0b0;
    const bool c0 = (a & 0xff00) == 0xc000;
    const bool possible_event = c0 && (host || (a >= 0xc050 && a <= 0xc05f) ||
                                       (!transfer.read && ((a >= 0xc00c && a <= 0xc00f) || a == 0xc022 || a == 0xc029 ||
                                                           a == 0xc02b || (capture.lattice_map == SSCapture::LatticeMap::V2 && a == 0xc034) ||
                                                           a == 0xc07e || a == 0xc07f)));
    const bool possible_copy = !transfer.read && ((a >= 0x400 && a < 0xc00) || (a >= 0x2000 && a < 0xa000));
    // Card execution may acknowledge an event before host state catches up. Resolve
    // that at the next possible RDY access, before the Apple completes it.
    if (host || (worker ? host_irq : apple_irq) ||
        ((queued_event || (worker ? host_pending : capture.pending)) && (possible_copy || possible_event))) {
        const uint64_t held = dispatch(transfer.ticks, true, &transfer);
        queued_event = false;
        next_transfer_tick += held;
        observed_ticks = host_transfer.ticks + held;
        host_transfer.data = transfer.data;
        host_transfer.driven = transfer.driven;
        return held;
    }
    if (transfers.size() == QUEUE_SIZE)
        dispatch(transfer.ticks, false);
    transfers.push_back(transfer);
    queued_event |= possible_event;
    observed_ticks = host_transfer.ticks;
    return 0;
}
