// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller

/*
 *   The Zilog Z8S180 is Second Sight's own processor. It runs the card ROM
 *   and copies Apple video bytes into the Oak controller's video memory.
 *   SSCard supplies memory, port and pin callbacks. This file connects the
 *   instruction handlers to the on-chip timers, serial ports and bus scheduler.
 *   SSCard runs the CPU on its worker while a job is outstanding. Without
 *   a worker, including browser builds, the caller executes it directly.
 *   Callbacks run on that same thread and must not enter execution recursively.
 *
 *   PHI is the processor clock. One T-state is one PHI period; a half-PHI is
 *   one rising-to-falling or falling-to-rising interval. A machine cycle is a
 *   bus operation or internal step made of T-states. T1, T2 and T3 name its
 *   successive clocks; TW is an inserted wait clock and Ti an internal clock.
 *   M1 identifies an instruction-byte fetch. IORQ selects an input/output port.
 *   WAIT lengthens a transfer. BUSREQ asks the processor to release its bus;
 *   BUSACK confirms that it has done so. These names denote asserted signals,
 *   regardless of whether the electrical pin is active low.
 *
 *   The MMU (memory management unit) maps a 16-bit program address to a
 *   20-bit physical address. DMA (direct memory access) has two channels,
 *   each a source, destination and byte counter that can copy without an
 *   instruction for each byte. DREQ is a channel's external request input.
 *   INT0-2 are maskable interrupt inputs; NMI is a non-maskable interrupt.
 *   PRT means programmable reload timer. FRC is the free-running counter
 *   used to divide PHI. ASCI is asynchronous serial communication; CSI/O
 *   is serial communication with a separate clock wire.
 *
 *   Sources: Zilog UM005004-0918 (UM0050), printed pp. 15-30 for bus timing,
 *   pp. 55-64 for the MMU, pp. 65-115 for interrupts, refresh and DMA,
 *   pp. 116-164 for serial ports and timers, and pp. 251-280 for instruction
 *   bus sequences: https://www.zilog.com/docs/z180/um0050.pdf.
 *   The continuation journal and fast paths are emulator mechanisms, not
 *   extra processor registers. K/N revision differences use Zilog TN0053
 *   and the Z8S180 data sheets cited at the affected operation.
 */

#pragma once
#include <algorithm>
#include <array>
#include <csetjmp>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <utility>
#include "Z180Types.hpp"
#include "Z180Time.hpp"
#include "Z180ASCI.hpp"
#include "Z180CSIO.hpp"
enum {
    Z180_TABLE_op,
    Z180_TABLE_cb,
    Z180_TABLE_ed,
    Z180_TABLE_xy,
    Z180_TABLE_xycb,
    Z180_TABLE_ex // cycle counts for taken jr/jp/call and interrupt latency (rst opcodes)
};

// Internal input indices. IRQ means interrupt request; DREQ requests a DMA
// byte. Public callers use Input below rather than these numeric values.
enum {
    Z180_INPUT_LINE_IRQ0,  // Execute IRQ0
    Z180_INPUT_LINE_IRQ1,  // Execute IRQ1
    Z180_INPUT_LINE_IRQ2,  // Execute IRQ2
    Z180_INPUT_LINE_DREQ0, // Start DMA0
    Z180_INPUT_LINE_DREQ1  // Start DMA1
};

constexpr int INPUT_LINE_NMI = 5;
// Processor state owned by one SSCard. Only its execution thread mutates it.
// The debugger reads it after SSCard has waited for the worker to finish.
class Z180 {
public:
    enum class Access { MemoryRead, MemoryWrite, IORead, IOWrite };
    enum class Input { INT0, INT1, INT2, NMI, DREQ0, DREQ1 };

    // Board wiring copied into Z180. Callbacks run on the CPU execution thread.
    // Memory addresses are physical 20-bit values; port addresses are 16-bit.
    // Reads return the driven byte; writes receive that byte. extra_wait returns
    // additional TW clocks. irq_ack returns the interrupt bus value; reti signals
    // return-from-interrupt. tend receives channel 0/1 and asserted transfer-end
    // state. clock_changed receives the new PHI frequency in Hz.
    struct Bus {
        std::function<uint8_t(uint32_t)> memory_read;
        std::function<void(uint32_t, uint8_t)> memory_write;
        std::function<uint8_t(uint16_t)> io_read;
        std::function<void(uint16_t, uint8_t)> io_write;
        std::function<unsigned(Access, uint32_t)> extra_wait;
        std::function<uint32_t()> irq_ack;
        std::function<void()> reti;
        std::function<void(unsigned, bool)> tend;
        std::function<void(uint32_t)> clock_changed;
    };

    // Debugger copy of programmer-visible state; the caller owns the copy.
    // PC is the next instruction address, SP the stack pointer, AF the
    // accumulator/flags pair, BC/DE/HL general pairs, IX/IY index registers.
    // Suffix 2 denotes the alternate register set. R counts refresh fetches;
    // I supplies the interrupt-vector high byte; IM is interrupt mode 0-2.
    // IFF1/2 hold interrupt enables. halt: 0 running, 1 HALT, 2 SLP.
    // tstates is elapsed whole PHI, not the number of retired instructions.
    // Register layout: UM0050 pp. 173-179.
    struct State {
        uint16_t pc, sp, af, bc, de, hl, ix, iy;
        uint16_t af2, bc2, de2, hl2;
        uint8_t r, i, im, iff1, iff2, halt;
        uint64_t tstates;
    };

#ifdef GS2_SS_TESTING
    // Select bus-cycle execution for fixtures; false uses the test-only instruction engine.
    void set_machine_cycle_engine(bool enabled) { cycle_engine = enabled; }
    // Force one journaled transaction at a time for timing comparison fixtures.
    void set_machine_cycle_reference(bool enabled) { cycle_reference = enabled; }
#endif
    // Return whether execution budgets count half-PHI bus edges.
    bool machine_cycle_engine() const { return cycle_engine; }
    // Return the asserted BUSACK output; the board may own the bus only while true (UM0050 pp. 26-27).
    bool bus_acknowledged() const { return bus_ack; }
    // Convert phi whole processor clocks into an emulated duration at the current divider.
    Z180Time phi_time(unsigned phi) const { return cycles_to_time(phi * (cycle_engine ? 2 : 1)); }
    // Invalidate direct execution when the external WAIT pin changes.
    void wait_w(bool asserted) {
        instruction_fast = false;
        wait_line = asserted;
    }
    // IORQ edges, separate from the T3 data sample/write. A board may
    // acknowledge DREQ at select assertion before the data transfer ends.
    // Observe external port-select edges. Callback arguments are port, write flag,
    // outgoing byte, and asserted select. Data transfer is a separate later edge
    // (UM0050 pp. 17, 23); Second Sight acknowledges its copy request here.
    void set_io_strobe(std::function<void(uint32_t, bool, uint8_t, bool)> callback) { io_strobe = std::move(callback); }
    // Bind the board wait query: physical address/port, write flag and I/O flag
    // in; additional TW clocks out. The core combines these with DCNTL waits.
    void set_bus_wait(std::function<unsigned(uint32_t, bool, bool)> callback) { bus_wait = std::move(callback); }
    // Observe half-PHI time, encoded owner/kind, edge number, address and byte.
    // Edges 0-5 mark cycle start, T1 falling, T2 rising, wait end, data, and end;
    // edge 6 marks instruction retirement. Installing it disables skipped edges.
    void set_cycle_observer(std::function<void(uint64_t, uint8_t, uint8_t, uint32_t, uint8_t)> callback) {
        cycle_observer = std::move(callback);
    }
    // Observe asserted/deasserted BUSACK so SSCard can start or stop its bus transfer.
    void set_busack(std::function<void(bool)> callback) { busack_cb = std::move(callback); }
    // Permit unchanged HALT and recognized polling spans to advance within a
    // scheduler deadline. Observers, pending inputs or active timers prevent skipping.
    void set_idle_skipping(bool enabled) { idle_skipping = enabled; }
    // Bind a board check for safe idle reads: address and write flag in,
    // nonnegative if safe, negative otherwise. Address 0x100030 identifies
    // the internal DMA-status port, outside the physical memory address range.
    void set_idle_memory(std::function<int(uint32_t, bool)> cb) { idle_memory = std::move(cb); }
#ifdef GS2_SS_TESTING
    // Select the test-only instruction timing comparison path.
    void set_instruction_boundary_timing(bool v) { instruction_boundary_timing = v; }
#endif
    // Return completed instruction/interrupt-handler entries for debugger stepping.
    uint64_t retired_instructions() const { return retired; }
#ifdef GS2_SS_TESTING
    // Enable the test fixture switch for scheduled bus release and cycle timelines.
    void set_scheduled_timeline(bool enabled) { scheduled_timeline = enabled; }
    // Enable the test fixture switch for batching eligible register instructions.
    void set_register_batching(bool enabled) { batch_registers = enabled; }
#endif
    // Certify a physical page for direct reads and retain its data-bus drive
    // side effect; changing the page invalidates logical read caches.
    // page is a physical 64-KiB page number 0-15; data must remain valid while
    // registered. bus and valid receive the last driven byte and its validity.
    // The board promises reads need no callback; null data removes a page.
    void set_direct_read_page(unsigned page, uint8_t const *data, uint8_t &bus, bool *valid = nullptr) {
        direct_read[page] = data;
        direct_read_bus = &bus;
        direct_read_valid = valid ? valid : &direct_read_valid_default;
        instruction_fast = false;
        cycle_update_direct_reads();
    }
    // Remove all host-pointer shortcuts before remapping or restoring bus state.
    void clear_direct_read_pages() {
        direct_read.fill(nullptr);
        direct_logical.fill(nullptr);
        direct_read_bus = nullptr;
        instruction_fast = false;
    }

    // K and N select documented silicon revisions; N adds clock multiplier
    // and serial features. Second Sight uses K. See Zilog TN0053.
    enum class Silicon { K, N };
    // Copy board callbacks, select the silicon revision, and reset the processor.
    // extal_hz is the external oscillator frequency, before the PHI divider.
    explicit Z180(Bus bus, uint32_t extal_hz = 36'000'000, Silicon silicon = Silicon::N);
    // Reset CPU and internal peripherals while preserving elapsed emulated time.
    // The board owns ROM, RAM and external latch reset (UM0050 pp. 25, 45-48).
    void reset();
    // Deadlines are exact half-PHI boundaries, including partial machine cycles.
    // Run no later than target, an absolute half-PHI counter. A yield may return
    // earlier; a partially executed instruction remains resumable.
    void run_half_until(uint64_t target);
    // Service due board events on the current CPU stack and return the next
    // absolute half-PHI deadline. Returning the current time yields normally.
    // The callback must not execute this CPU recursively.
    void set_scheduler(std::function<uint64_t()> callback) {
        scheduler = std::move(callback);
        if (!scheduler)
            bus_release_at = UINT64_MAX;
    }
    // Board-owned release time, valid only for the active scheduler call.
    // The board retains its absolute deadline across CPU checkpoints/exits.
    void set_bus_release_complete(std::function<void()> callback) { bus_release_complete = std::move(callback); }
    // Request BUSREQ release at an absolute half-PHI deadline. Return false if
    // this grant cannot schedule it; the board must retain and service the deadline.
    bool schedule_bus_release(uint64_t half_deadline);
    // Return whether this grant has a pending board-owned BUSREQ release deadline.
    bool bus_release_scheduled() const { return bus_release_at != UINT64_MAX; }
    // Run to target whole PHI since reset; converts to the half-PHI interface.
    void run_until(uint64_t target) { run_half_until(target * 2); }
    // Stop the current grant at the next safe return to the board scheduler.
    void request_yield() {
        yield_requested = true;
        if (executing)
            icount = 0;
    }
    // Advance toward one retired instruction; stop on HALT, BUSACK or the
    // bounded step budget so a debugger cannot hang on an unending external wait.
    void step();
    // Return elapsed whole PHI; an unfinished half clock is rounded down.
    uint64_t cycles() const { return half_phi / 2; }
    // Return elapsed half-PHI for scheduling a board event between clock edges.
    uint64_t half_cycles() const { return half_phi; }
    // Return current PHI clocks per second after CMR/CCR division (UM0050 pp. 49-52).
    uint32_t clock_hz() const;
    // Set the logical asserted state of an interrupt or DMA-request pin.
    // NMI assertions are remembered until serviced; INT0-2 are level requests.
    void set_input(Input input, bool asserted);
    // Apply board BUSREQ state and invalidate any scheduled release deadline
    // when the request ends (Zilog UM0050, BUSREQ/BUSACK).
    void bus_release_w(bool state) {
        instruction_fast = false;
        bus_released = state;
        if (!state)
            bus_release_at = UINT64_MAX;
        if (state && executing)
            request_yield();
    }
    // Allow serial channels to skip idle receive clocks only while their pin
    // level and sample phase remain equivalent.
    void set_asci_idle_clock_gating(bool state) {
        for (auto &a : asci)
            a.set_idle_receive_clock_gating(state);
    }
    // Return the timer output level that can replace address bit A18 (UM0050 p. 164).
    bool tout() const { return tout_level; }
    // Return true in either HALT or SLP; these states have different clock and wake rules.
    bool halted() const { return HALT != 0; }
    // Substitute timer output on the multiplexed A18/TOUT pin when selected
    // by TCR; the external board sees this pin rather than the MMU address bit.
    uint32_t a18_tout_address(uint32_t a) const {
        return (tcr & 0x0c) ? (a & ~uint32_t(0x40000)) | (uint32_t(tout_level) << 18) : a;
    }
    // Return a debugger register snapshot; it excludes peripherals and partial-cycle state.
    State get_state() const;
    // Apply debugger registers. The tstates member is informational and does
    // not rewind the scheduler. Use load for a complete execution checkpoint.
    void set_state(const State &state);
    // Translate logical address a through the cached MMU page map (UM0050 pp. 58-64).
    uint32_t physical_address(uint16_t a) const { return mmu[a >> 12] | (a & 4095); }
    // Read internal register number reg modulo 64, including hardware read side effects.
    uint8_t read_register(uint8_t reg) { return z180_internal_port_read(reg & 0x3f); }
    // Write internal register number reg modulo 64; value is the bus byte.
    // These calls bypass relocated port-address matching but retain register effects.
    void write_register(uint8_t reg, uint8_t value) { z180_internal_port_write(reg & 0x3f, value); }
    // Return the CPU-owned ASCI channel (ch low bit selects 0/1); use only on the execution thread.
    Z180ASCI &serial(unsigned ch) { return asci[ch & 1]; }
    // Return the CPU-owned clocked serial port; use only on the execution thread.
    Z180CSIO &clocked_serial() { return csio; }
    // Optional synchronous debugger hook before instruction execution. The
    // snapshot is temporary; a hook must copy it to retain it.
    std::function<void(const State &)> trace;
    // Derive absolute CPU time from the current oscillator epoch and half-PHI
    // counter, preserving period rounding across execution chunks.
    Z180Time time() const {
        uint64_t const delta = half_phi - time_half;
        if (delta < epoch_rate)
            return time_base + Z180Time(0, delta * half_period);
        return update_time_cache();
    }
    // Convert execution ticks to duration at the current divider (half-PHI in the cycle engine).
    Z180Time cycles_to_time(uint64_t n) const;
    // Return complete execution ticks in duration t, rounded down.
    uint64_t time_to_cycles(Z180Time t) const;
    // Write a same-build CPU/peripheral checkpoint; return 0 on success, 1 on stream failure.
    int save(std::ostream &out);
    // Restore a same-build checkpoint; return 0 on success, 1 on format/revision/read failure.
    int load(std::istream &in);

private:
    Bus bus;
    uint32_t extal_hz;
    const Silicon silicon;
    bool executing = false, yield_requested = false, side_effects_disabled = false;
    std::function<uint64_t()> scheduler;
    uint64_t run_target = 0;
#ifdef GS2_SS_TESTING
    bool scheduled_timeline = true;
#else
    static constexpr bool scheduled_timeline = true;
#endif
    uint64_t scheduler_limit = 0, bus_release_at = UINT64_MAX;
    std::function<void()> bus_release_complete;
    // Service the board and serial deadlines; return true only when another
    // positive half-PHI budget is available. Never recursively execute an instruction.
    bool refill_budget();
    Z180Time epoch_time;
    uint64_t epoch_half = 0;
    uint32_t epoch_rate = 0;
    uint64_t attoseconds_per_cycle = 0;
    // Derived interpolation within one whole second of the clock epoch.
    // Never serialized: observing time must not change a checkpoint.
    mutable Z180Time time_base;
    mutable uint64_t time_half = 0;
    uint64_t half_period = 0;
    // Return absolute emulated time and cache its whole-second epoch; no hardware state changes.
    Z180Time update_time_cache() const;
    // Rebuild derived clock interpolation after reset, restore or a divider change.
    void reset_time_cache();
    Z180ASCI asci[2] = {Z180ASCI(0), Z180ASCI(1)};
    Z180CSIO csio;
    uint8_t cmr = 0, ccr = 0, rtemp = 0;
    uint32_t ioltemp = 0;
    int icount = 0, extra_cycles = 0;
    const uint8_t *cc[6]{};
#ifdef GS2_SS_TESTING
    bool instruction_boundary_timing = false;
#endif
    std::function<int(uint32_t, bool)> idle_memory;
    bool idle_skipping = false;
    // Return true after consuming safe HALT/SLP cycles, false if individual edges are needed.
    bool skip_machine_idle();
    // Advance stopped-clock SLP or its wake delay. Return true if this call
    // consumed the interval without ordinary CPU execution (UM0050 pp. 37-41).
    bool run_low_power();
    uint8_t power_mode = 0;
    uint32_t wake_remaining = 0;
    // Return true after advancing unchanged iterations of the firmware DMA1
    // wait loop. Recheck its instruction bytes and inputs before each admission.
    bool skip_machine_poll();
#ifdef GS2_SS_TESTING
    // Test-only instruction-engine equivalent of the firmware DMA polling shortcut.
    bool skip_dma_poll();
    // Test-only duration, in whole PHI, of one halted instruction-engine step.
    int halt_cycles() const;
#endif
    // The N multiplier is valid only for 10-16 MHz EXTAL with CMR.X2 set
    // (Z8S180 data sheet DS006002, clock multiplier section).
    bool clock_multiplier_enabled() const {
        return silicon == Silicon::N && (cmr & 0x80) && extal_hz >= 10'000'000 && extal_hz <= 16'000'000;
    }
    // Convert oscillator clocks to execution ticks, using the active engine units.
    uint64_t clocks_to_cycles(uint64_t n) const;
    // Convert execution ticks to enough oscillator clocks, rounding upward.
    uint64_t cycles_to_clocks(uint64_t n) const;
    // Record elapsed time at the old clock rate before changing its divider.
    void prepare_deferred_clock_change();
    // Rebase serial clocks and notify the board so its next deadline uses the new PHI rate.
    void notify_clock_changed();
    // Apply documented CPU/internal-register reset values (UM0050 pp. 45-48).
    void reset_core();
    // Consume icount using the selected engine; production always uses machine cycles.
    void execute_run();
    // Set an internal input index to logical state 0/1 and retain new NMI/DREQ edges.
    void execute_set_input(int input, int state);
    // Read one physical 20-bit byte through the board callback; an unwired bus returns 0xff.
    uint8_t read_memory(uint32_t a) { return bus.memory_read ? bus.memory_read(a & 0xfffff) : 0xff; }
    // Route a translated CPU write to the board while retaining bus timing effects.
    // Write byte d at physical address a modulo 1 MiB if the board has a write callback.
    void write_memory(uint32_t a, uint8_t d) {
        if (bus.memory_write)
            bus.memory_write(a & 0xfffff, d);
    }
    // Read an external 16-bit port; return 0xff if the board has no read callback.
    uint8_t read_io(uint16_t a) { return bus.io_read ? bus.io_read(a) : 0xff; }
    // Write byte d to an external 16-bit port. Internal registers are decoded before this call.
    void write_io(uint16_t a, uint8_t d) {
        if (bus.io_write)
            bus.io_write(a, d);
    }
    // Instruction-helper alias for a physical board read; it adds no clocks itself.
    uint8_t z180_read_memory(uint32_t a) { return read_memory(a); }
    // Instruction-helper alias for a physical board write; timing belongs to the cycle engine.
    void z180_write_memory(uint32_t a, uint8_t d) { write_memory(a, d); }
    // Notify the board of DMA terminal-count output transitions.
    void tend(unsigned ch, bool state) {
        if (bus.tend)
            bus.tend(ch, state);
    }
    // Return the internal register byte at offset port, including latches and flag clearing
    // (UM0050 pp. 293-302). The caller has already decoded the register window.
    uint8_t z180_internal_port_read(uint8_t port);
    // Apply data to internal offset port with that register's reserved-bit and side-effect rules.
    void z180_internal_port_write(uint8_t port, uint8_t data);
    // Return DCNTL memory waits, 0-3 TW clocks (UM0050 p. 29).
    int memory_wait_states() const { return (dcntl & 0xc0) >> 6; }
    // Return DCNTL external port waits, 1-4 TW clocks (UM0050 p. 30).
    int io_wait_states() const { return ((dcntl & 0x30) >> 4) + 1; }
    // Test the relocated 64-byte window; only port bits 7:6 may vary (UM0050 pp. 42-43).
    bool is_internal_io_address(uint16_t port) const { return ((port ^ iocr) & 0xffc0) == 0; }
// These fragments declare private members, not independent classes. They
// split register state, bus continuations and instruction handlers by purpose.
#include "z180_state.hpp"
#include "z180cycles.hpp"
#include "z180_instructions.hpp"
};
