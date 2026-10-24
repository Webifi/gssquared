/*
 *   Private state of the Z8S180 bus scheduler, included inside class Z180.
 *   One Z180 owns these records and only its execution thread uses them.
 *   Opcode handlers ask for memory or port transactions. Z180.cpp advances
 *   those transactions to the next permitted clock edge and calls SSCard.
 *
 *   PHI is the processor clock; a T-state is one PHI and a half-PHI is half
 *   that period. Ti is an internal clock with no data transfer. DMA is direct
 *   memory access by an on-chip copy channel. BUSREQ asks for bus ownership;
 *   BUSACK grants it. A continuation is an unfinished instruction retained
 *   across a scheduler deadline. Its journal records completed bus operations
 *   so the instruction can restart without reading or writing a device twice.
 *
 *   Sources: Zilog UM005004-0918 pp. 19-30 for bus edges, pp. 65-115 for
 *   interrupt/DMA timing, and pp. 251-280 for instruction cycle sequences.
 *   The journal, pointers and fast-path flags are emulator bookkeeping.
 */

// fetch reads an opcode; operand reads a following instruction byte; read/write
// access data memory; input/output access ports; internal consumes Ti clocks.
// acknowledge reads an interrupt vector; nmi enters non-maskable interrupt;
// refresh refreshes dynamic RAM; halt repeats HALT fetch timing; release
// advances clocks while an external master owns the bus.
enum class cycle_kind : uint8_t {
    fetch,
    operand,
    read,
    write,
    input,
    output,
    internal,
    acknowledge,
    nmi,
    refresh,
    halt,
    release
};
enum class cycle_owner : uint8_t { cpu, dma, refresh };
// Test-engine suspension signal; caught on the same CPU execution thread.
struct cycle_yield {};
// A scheduler suspension discards only trivial instruction/engine stack locals.
// Architectural state and the unfinished edge are already in the journal.
// This jump target lives only during execute_machine_cycles; never save it.
std::jmp_buf cycle_resume;
// One completed operation owned by the CPU journal. address is logical for
// CPU memory accesses; data is the sampled/written byte; nominal is whole
// PHI excluding waits. Replay consumes this record instead of a bus callback.
struct cycle_record {
    cycle_kind kind = cycle_kind::internal;
    uint32_t address = 0;
    uint8_t data = 0;
    uint8_t nominal = 0;
};
// One in-flight bus operation owned by Z180, retained when its thread yields.
// address is physical for memory. phase is the resume label in cycle_run;
// waits/trailing/nominal count whole PHI, elapsed counts half-PHI. sampled
// prevents a repeated data callback. ttp requests an undefined-opcode trap
// extension clock. no_arbitration prohibits handing the bus off at the end.
struct cycle_job {
    cycle_kind kind = cycle_kind::internal;
    cycle_owner owner = cycle_owner::cpu;
    uint32_t address = 0;
    uint8_t data = 0;
    uint8_t phase = 0;
    uint16_t waits = 0;
    uint16_t elapsed = 0;
    uint8_t nominal = 3;
    uint8_t trailing = 0;
    uint8_t io_select_edge = 0;
    bool active = false;
    bool sampled = false;
    bool ttp = false;
    bool no_arbitration = false;
};
// CPU-owned register snapshot from before instruction fetch. Only register
// arithmetic is replayed; device state, DMA and elapsed time are not rewound.
// regs holds PC, SP, AF, BC, DE, HL, IX, IY and the four alternate pairs.
// control holds R, R2, IFF1, IFF2, HALT, IM, I, after_EI and a reserved zero
// as assigned by cycle_save_state; both methods must keep the same order.
struct instruction_state {
    uint32_t regs[12]{};
    uint8_t control[9]{};
};
#ifdef GS2_SS_TESTING
bool cycle_engine = false;
bool cycle_reference = false;
#else
static constexpr bool cycle_engine = true;
static constexpr bool cycle_reference = false;
#endif
bool cycle_executing = false;
bool instruction_fast = false;
bool instruction_whole = false;
bool fast_irq_valid = false;
int fast_interrupt = -1;
bool instruction_active = false;
bool wait_line = false;
bool bus_ack = false;
bool sampled_bus_request = false;
bool boundary_allowed = true;
bool dma_allowed = false;
bool dma_start_pending = false;
bool refresh_pending = false;
bool refresh_after_sleep = false;
bool dma_edge[2]{};
uint32_t sampled_iol = 0;
uint64_t dma_edge_generation[2]{}, dma_edge_accepted[2]{}, dma_edge_sampled[2]{};
uint8_t phi_half = 0;
uint8_t refresh_address = 0;
uint16_t refresh_phase = 0;
uint16_t advance_left = 0;
uint64_t half_phi = 0;
// Complete certified reads/Ti may compose these peripheral clocks. This is
// flushed before any callback, phase-engine entry or scheduler exit, never
// saved as hardware state. The scheduler budget and bus time stay current.
unsigned quiet_phi = 0;
uint8_t quiet_record = 0;
bool quiet_no_arbitration = false;
uint64_t retired = 0;
uint64_t fast_instructions = 0;
uint64_t resumptions = 0;
#ifdef GS2_SS_TESTING
bool batch_registers = true;
#else
static constexpr bool batch_registers = true;
#endif
cycle_job cycle;
instruction_state instruction_start;
cycle_record history[40]{};
uint8_t history_count = 0;
unsigned cursor = 0, bus_number = 0, nominal = 0;
unsigned family = 0, opcode = 0;
uint32_t prelude_schedule = 0;
bool trapping = false, third_trap = false;
bool reti_notified = false;
bool trap_started = false;
bool interrupt_started = false;
int sampled_interrupt = -1;
bool interrupt_recovery = false;
bool dma_nmi = false;
int instruction_interrupt = -1;
int ack_vector = -1;
uint8_t dma_channel = 0xff;
uint8_t dma_stage = 0;
uint8_t dma_continue = 0xff;
uint8_t dma_byte = 0;
uint32_t dma_source = 0, dma_destination = 0;
bool dma_source_io = false, dma_destination_io = false;
uint8_t dma_source_mode = 0, dma_destination_mode = 0;
std::function<unsigned(uint32_t, bool, bool)> bus_wait;
std::function<void(uint32_t, bool, uint8_t, bool)> io_strobe;
std::function<void(uint64_t, uint8_t, uint8_t, uint32_t, uint8_t)> cycle_observer;
std::function<void(bool)> busack_cb;
std::array<uint8_t const *, 16> direct_read{};
// Derived composition of the MMU, multiplexed A18 and certified physical
// read views. These pointers cache address decoding only, never read data.
std::array<uint8_t const *, 16> direct_logical{};
uint8_t *direct_read_bus = nullptr;
bool direct_read_valid_default = false;
bool *direct_read_valid = &direct_read_valid_default;
// Retain the byte driven by a certified read so floating-bus/IM0 behaviour
// is identical to callback-based reads.
uint8_t drive_direct_read(uint8_t data) {
    *direct_read_bus = data;
    *direct_read_valid = true;
    return data;
}
bool direct_reads_enabled = false;
// Rebuild sixteen logical 4-KiB read views from the MMU and board-owned physical pages.
void cycle_update_direct_reads();
// Return a stable byte pointer for physical address, or null when the board
// has not certified that 64-KiB page. A18 may be driven by the timer output.
uint8_t const *cycle_direct_read(uint32_t address) const {
    if (cycle_reference || !direct_reads_enabled)
        return nullptr;
    address = a18_tout_address(address);
    uint8_t const *const page = direct_read[(address >> 16) & 15];
    return page ? page + (address & 0xffff) : nullptr;
}
#ifdef GS2_SS_TESTING
// Run the test-only whole-instruction engine for comparison.
void execute_legacy();
#endif
// Consume the half-PHI budget, resuming incomplete instructions and servicing bus owners.
void execute_machine_cycles();
// Return whether the next ED A8 backward-copy instruction can use the specialized path.
bool cycle_ldd_ready() const;
// Execute an eligible instruction span; return its final nominal PHI count, or zero to fall back.
int cycle_direct_instruction();
// Execute one LDD backward-copy instruction through the shared read/write timing path.
void cycle_ldd();
// Apply LDD pointer/count/flag changes after byte data reaches the destination (UM0050 p. 225).
void ldd_registers(uint8_t data);
// Refill the scheduler grant or leave the active handler using its saved continuation.
void cycle_suspend();
// Save pre-fetch CPU registers for replay, without copying peripheral or bus state.
void cycle_save_state();
// Restore pre-fetch registers and restart the journal cursor; completed device accesses remain done.
void cycle_restore_state();

// Discard partial cycles, journals and pending bus ownership on processor reset.
void cycle_reset();
// Choose family/opcode timing and Ti insertion positions (UM0050 pp. 251-280).
void cycle_decode(unsigned family_arg, unsigned opcode_arg);
// Return internal PHI clocks required before kind at the current bus position.
unsigned cycle_prelude(cycle_kind kind) const;
// Return the same Ti count from explicit instruction rules for comparison and traps.
unsigned cycle_prelude_reference(cycle_kind kind) const;
// Perform or replay a logical CPU access; return its byte. data is used for writes.
inline uint8_t cycle_access(cycle_kind kind, uint32_t address, uint8_t data = 0);
// Insert prelude Ti clocks, then perform kind at logical address with outgoing data.
uint8_t cycle_access_slow(cycle_kind kind, uint32_t address, uint8_t data, unsigned prelude);
// Perform a complete logical access after prelude Ti clocks when the deadline permits it.
inline uint8_t cycle_whole_access(cycle_kind kind, uint32_t address, uint8_t data, unsigned prelude);
// Read from a board-approved pointer and journal the byte at logical address; return the byte.
inline uint8_t cycle_certified_read(cycle_kind kind, uint32_t address, uint8_t const *direct);
// Read/write physical address while retaining logical address for replay; return sampled data.
uint8_t cycle_mapped_access(cycle_kind kind, uint32_t address, uint32_t physical, uint8_t data);
// Consume count whole Ti clocks; no_arbitration suppresses the final bus hand-off.
void cycle_whole_internal(unsigned count, bool no_arbitration = false);
// Advance phi whole clocks and defer only peripheral bookkeeping until the next observation.
void cycle_defer_clocks(unsigned phi);
// Apply deferred peripheral clocks and reconstruct the last observable cycle record.
void cycle_flush_quiet();
// Copy current pins and interrupt priority once for a span guaranteed to have no input changes.
void cycle_sample_stable_inputs();
// Recheck whether the remaining instruction may use a fast path after a partial access.
void cycle_resume_admission();
// Advance ticks half-PHI without external callbacks while preserving divider phase.
void cycle_advance_quiet(unsigned ticks);
// Replay or perform one access; address is logical for CPU memory, data is the write byte.
uint8_t cycle_transaction(cycle_kind kind, uint32_t address, uint8_t data, bool no_arbitration = false);
// Initialize a new owner/kind cycle at physical address; data is its outgoing byte.
void cycle_begin(cycle_owner owner, cycle_kind kind, uint32_t address, uint8_t data = 0, bool no_arbitration = false);
// Advance the saved phase until cycle completion or a scheduler suspension.
void cycle_run();
// Try a complete memory/internal cycle without an observer; true means completed.
bool cycle_run_unobserved();
// Try an entire port cycle without an observer; true means completed, false retains its phase.
bool cycle_run_io_unobserved();
// Advance a saved simple-cycle phase without per-edge dispatch; true means completed.
bool cycle_run_partial();
// Try a whole stable cycle within the scheduler deadline; true means completed.
bool cycle_timeline();
// Return whether inputs and peripheral state allow a cycle timeline shortcut.
bool cycle_timeline_ready() const;
// Read the opcode on its documented edge and retain it across suspension (UM0050 pp. 19-20).
void cycle_sample_opcode();
// Perform the data read/write once on its sampling edge (UM0050 pp. 21-23).
void cycle_sample_data();
// Advance half_phi half clocks, checking budget and on-chip divider transitions.
void cycle_advance(unsigned half_phi);
// Choose external master, refresh, DMA or CPU in priority order (UM0050 pp. 281-282).
void cycle_arbitrate();
// Latch channel 0/1 addresses and transfer modes before granting its first byte.
void cycle_dma_begin(unsigned channel);
// Perform one DMA read or write phase and retire a byte only after its write.
void cycle_dma_run();
// Return whether channel 0/1 is enabled and requested; do not consume a request edge.
bool cycle_dma_ready(unsigned channel) const;
// Fill Ti clocks up to nominal_arg whole PHI and verify the journal is fully replayed.
void cycle_finish_cpu(int nominal_arg);
// Send edge number 0-5 and current bus state to the optional observer.
void cycle_emit(uint8_t edge);
// Execute the selected interrupt entry and return nominal PHI (UM0050 pp. 73-86).
int cycle_interrupt();
// Return the eligible interrupt priority index or -1; respect EI and recovery delay.
int cycle_choose_interrupt();
// Latch the current highest-priority input at a CPU sampling edge (UM0050 pp. 73-81).
void cycle_sample_interrupts();
// Notify the board once that return-from-interrupt completed, even after replay.
void cycle_reti();

// Checkpoints contain fields, never C++ struct padding. Padding can differ
// between the in-stack scheduler and a resumed handler even with equal state.
// Pass each retained field to io for either reading or writing. The callback
// returns no value; the owning save/load routine checks its stream status.
template <typename IO> void checkpoint_continuation(IO &&io) {
    io(cycle.kind);
    io(cycle.owner);
    io(cycle.address);
    io(cycle.data);
    io(cycle.phase);
    io(cycle.waits);
    io(cycle.elapsed);
    io(cycle.nominal);
    io(cycle.trailing);
    io(cycle.io_select_edge);
    io(cycle.active);
    io(cycle.sampled);
    io(cycle.ttp);
    io(cycle.no_arbitration);
    io(instruction_start.regs);
    io(instruction_start.control);
    for (auto &h : history) {
        io(h.kind);
        io(h.address);
        io(h.data);
        io(h.nominal);
    }
}
