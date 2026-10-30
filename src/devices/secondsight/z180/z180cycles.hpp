/*
 *   Private state of the Z8S180 bus scheduler, included inside class Z180.
 *   One Z180 owns these records and only its execution thread uses them.
 *   Opcode handlers ask for memory or port transactions. Z180.cpp advances
 *   those transactions to the next permitted clock edge and calls SSCard.
 *
 *   PHI is the processor clock; a T-state is one PHI and a half-PHI is half
 *   that period. Ti is an internal clock with no data transfer. DMA is direct
 *   memory access by an on-chip copy channel. BUSREQ asks for bus ownership;
 *   BUSACK grants it. The card worker keeps an instruction on its own stack
 *   across scheduler deadlines. Only the test reference keeps a journal to
 *   restart a handler without reading or writing a device twice.
 *
 *   Sources: Zilog UM005004-0918 pp. 19-30 for bus edges, pp. 65-115 for
 *   interrupt/DMA timing, and pp. 251-280 for instruction cycle sequences.
 *   FRC is the free-running divider counter; PRTs are the two programmable
 *   reload timers. TIF records a timeout and TOUT is the timer output pin,
 *   which can replace address pin A18. These counters follow pp. 157-164.
 *   The test journal, pointers and fast-path flags are emulator bookkeeping.
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
#ifdef GS2_SS_TESTING
struct cycle_yield {};
#endif
// Test-only journal entry. Address is logical; nominal is whole PHI excluding waits.
#ifdef GS2_SS_TESTING
struct cycle_record {
    cycle_kind kind = cycle_kind::internal;
    uint32_t address = 0;
    uint8_t data = 0;
    uint8_t nominal = 0;
};
#endif
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
// Test-reference register snapshot from before instruction fetch. Only test
// replay restores register arithmetic; device state, DMA and time are not rewound.
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
// Set when arbitration or a meeting may have restored the fast-path proof.
bool instruction_whole = false;
bool fast_irq_valid = false;
int fast_interrupt = -1;
bool instruction_active = false;
// Only an exhausted scheduler grant may cancel for reset. Admitted cycles
// never test this flag; reset drains the remaining handler without clocks.
bool reset_cancelled = false;
bool scheduler_in_handler = false;
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
// FRC and PRT state is materialized at this PHI edge.  The next PRT timeout
// is a half-PHI deadline; it may change TIF, interrupt priority or TOUT/A18.
uint64_t timer_anchor = 0;
uint64_t timer_deadline = UINT64_MAX;
// Composed spans with running PRTs stop at timer_limit, the next timeout that
// can change a sampled input or A18. budget_end is the grant end without that
// stop. Both are scheduling bookkeeping, recomputed at each admission.
uint64_t timer_limit = UINT64_MAX;
uint64_t budget_end = 0;
uint64_t retired = 0;
uint64_t fast_instructions = 0;
uint64_t resumptions = 0;
cycle_job cycle;
#ifdef GS2_SS_TESTING
instruction_state instruction_start;
cycle_record history[40]{};
uint8_t history_count = 0;
unsigned cursor = 0;
#else
static constexpr unsigned history_count = 0, cursor = 0;
#endif
unsigned bus_number = 0, nominal = 0;
unsigned family = 0, opcode = 0;
bool trapping = false, third_trap = false;
#ifdef GS2_SS_TESTING
bool reti_notified = false;
bool trap_started = false;
bool interrupt_started = false;
#endif
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
// Apply LDD pointer/count/flag changes after byte data reaches the destination (UM0050 p. 225).
void ldd_registers(uint8_t data);
// Service a scheduler deadline and continue the same handler on this stack.
void cycle_suspend();
// Save pre-fetch CPU registers for replay, without copying peripheral or bus state.
#ifdef GS2_SS_TESTING
void cycle_save_state();
#endif
// Restore pre-fetch registers and restart the journal cursor; completed device accesses remain done.
#ifdef GS2_SS_TESTING
void cycle_restore_state();
#endif

// Discard partial cycles, journals and pending bus ownership on processor reset.
void cycle_reset();
// Choose family/opcode timing and Ti insertion positions (UM0050 pp. 251-280).
void cycle_decode(unsigned family_arg, unsigned opcode_arg);
// Return internal PHI clocks required before kind at the current bus position.
unsigned cycle_prelude(cycle_kind kind) const;
// Return the same Ti count from explicit instruction rules for comparison and traps.
unsigned cycle_prelude_reference(cycle_kind kind) const;
// Perform a logical CPU access; return its byte. data is used for writes.
inline uint8_t cycle_access(cycle_kind kind, uint32_t address, uint8_t data = 0);
// Insert prelude Ti clocks, then perform kind at logical address with outgoing data.
uint8_t cycle_access_slow(cycle_kind kind, uint32_t address, uint8_t data, unsigned prelude);
// Perform a complete logical access after prelude Ti clocks when the deadline permits it.
inline uint8_t cycle_whole_access(cycle_kind kind, uint32_t address, uint8_t data, unsigned prelude);
// Read a stable board-approved pointer, drive its byte on the bus and return it.
// Only the test engine also records the logical address for replay.
inline uint8_t cycle_certified_read(cycle_kind kind, uint32_t address, uint8_t const *direct);
// Read/write physical address while retaining the logical address for bookkeeping.
uint8_t cycle_mapped_access(cycle_kind kind, uint32_t address, uint32_t physical, uint8_t data);
// Consume count whole Ti clocks; no_arbitration suppresses the final bus hand-off.
void cycle_whole_internal(unsigned count, bool no_arbitration = false);
// Advance whole PHI and peripheral clocks within the admitted deadline (UM0050 pp. 86, 157-164).
void cycle_clock_span(unsigned phi);
// Retain the completed bus state after an admitted cycle.
void cycle_completed_record(cycle_kind kind, uint32_t address, uint8_t data, bool no_arbitration);
// Test-only completed access: retain its logical address, byte and nominal
// whole-PHI duration. Production retains no replay journal.
void cycle_record_access(cycle_kind kind, uint32_t address, uint8_t data, uint8_t duration) {
#ifdef GS2_SS_TESTING
    assert(history_count < std::size(history));
    history[history_count++] = {kind, address, data, duration};
    ++cursor;
#else
    (void)kind; (void)address; (void)data; (void)duration;
#endif
}
// Retire a CPU bus access after its final edge, regardless of how the
// preceding phases were executed. Keep test-reference journal and arbitration
// bookkeeping together so fast and observed paths use the same completion rule.
[[gnu::always_inline]] inline void cycle_finish_access(cycle_kind kind, uint32_t address, uint8_t data,
                                                      uint8_t duration, bool no_arbitration) {
    cycle_record_access(kind, address, data, duration);
    nominal += duration;
    boundary_allowed = !no_arbitration;
    dma_allowed = !dma_start_pending;
    dma_start_pending = false;
    refresh_after_sleep = false;
}
// Perform an admitted instruction's port cycle after pre Ti clocks; return its byte.
uint8_t cycle_whole_io(cycle_kind kind, uint32_t port, uint8_t data, unsigned pre);
// Update timer counters when half_phi reaches the next timeout. phi is
// retained in the helper signature; elapsed time comes from timer_anchor.
void advance_dividers(unsigned phi);
// Materialize the FRC and PRT counters and schedule the next timeout.
void sync_timers();
void schedule_timers();
// Return the next timer stop in absolute half-PHI, or UINT64_MAX for none.
// Production stops at every timeout; the reference can omit those that do
// not affect an enabled interrupt or TOUT. Its counters must be up to date.
uint64_t timer_horizon() const;
// Bound the admitted span by timer_horizon(); false if that edge is too close.
bool admit_timers();
// Copy current pins and interrupt priority once for a span guaranteed to have no input changes.
void cycle_sample_stable_inputs();
// Latch DREQ and request-edge generations at a documented bus sample.
inline void cycle_sample_dma_inputs() {
    sampled_iol = iol;
    std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
}
// Sample IRQ or remember a DMA-time NMI at the falling edge. A certified
// unobserved span may reuse the interrupt sample while its inputs stay fixed.
inline void cycle_sample_irq_edge(bool cached = false) {
    if (cycle.owner == cycle_owner::cpu && cycle.kind != cycle_kind::release) {
        if (cached && instruction_fast && fast_irq_valid)
            sampled_interrupt = fast_interrupt;
        else {
            cycle_sample_interrupts();
            if (cached) {
                fast_interrupt = sampled_interrupt;
                fast_irq_valid = instruction_fast;
            }
        }
    } else if (cycle.owner == cycle_owner::dma && dma_stage && nmi_pending)
        dma_nmi = true;
}
// Recheck whether the remaining instruction may use a fast path after a partial access.
void cycle_resume_admission(bool interrupt = false);
// One Ti of an interrupt entry sequence.
void cycle_interrupt_internal();
// Apply a granted half-PHI span with the shared divider and refresh rules.
void cycle_clock_step(unsigned ticks);
// Advance ticks half-PHI without external callbacks while preserving divider phase.
void cycle_advance_quiet(unsigned ticks);
// Avoid the resumable path only when the whole step is already granted.
bool cycle_advance_granted(unsigned ticks);
// Perform one access; only the test reference replays. Memory addresses are logical.
uint8_t cycle_transaction(cycle_kind kind, uint32_t address, uint8_t data, bool no_arbitration = false);
// Initialize the common physical cycle record before either edge execution
// or final-state reconstruction of a certified, unobservable span.
inline void cycle_initialize(cycle_owner owner, cycle_kind kind, uint32_t address, uint8_t data,
                             bool no_arbitration) {
    cycle = {};
    cycle.owner = owner;
    cycle.kind = kind;
    cycle.address = address;
    cycle.data = data;
    cycle.no_arbitration = no_arbitration;
    cycle.active = true;
}
// Complete the shared phase cursor, including a materialized certified span.
inline void cycle_complete(uint8_t phase) {
    cycle.phase = phase;
    cycle.active = false;
}
// Initialize a new owner/kind cycle at physical address; data is its outgoing byte.
void cycle_begin(cycle_owner owner, cycle_kind kind, uint32_t address, uint8_t data = 0,
                 bool no_arbitration = false, bool force_memory_wait = false);
// Advance the saved phase until cycle completion or a scheduler suspension.
void cycle_run();
// Read the opcode on its documented edge and retain it across suspension (UM0050 pp. 19-20).
void cycle_sample_opcode();
// Perform the common memory data transfer at the opcode or data edge.
void cycle_sample_memory();
// WAIT is ignored for internal I/O but not for external memory or I/O.
bool cycle_wait_pending() const;
// Perform the data read/write once on its sampling edge (UM0050 pp. 21-23).
void cycle_sample_data();
// Advance half_phi half clocks, checking budget and on-chip divider transitions.
// Return false only when reset cancels the remaining clocks.
bool cycle_advance(unsigned half_phi);
// Choose external master, refresh, DMA or CPU in priority order (UM0050 pp. 281-282).
void cycle_arbitrate();
// Latch channel 0/1 addresses and transfer modes before granting its first byte.
void cycle_dma_begin(unsigned channel);
// Perform one DMA read or write phase and retire a byte only after its write.
void cycle_dma_run();
// Return whether channel 0/1 is enabled and requested; do not consume a request edge.
bool cycle_dma_ready(unsigned channel) const;
// Evaluate channel 0/1 using pin-level mask lines and request-edge serial edge.
// Comparing edge numbers distinguishes a fresh DREQ pulse from a retained level.
bool dma_request(unsigned channel, uint32_t lines, uint64_t edge) const;
// Return true when no DMA transfer is active and no enabled channel requests.
bool dma_idle() const;
// Fill Ti clocks up to nominal_arg whole PHI; verify any test journal is consumed.
void cycle_finish_cpu(int nominal_arg);
// Send edge number 0-5 and current bus state to the optional observer.
void cycle_emit(uint8_t edge);
// Execute the selected interrupt entry and return nominal PHI (UM0050 pp. 73-86).
int cycle_interrupt();
// Return the eligible interrupt priority index or -1; respect EI and recovery delay.
int cycle_choose_interrupt();
// Latch the current highest-priority input at a CPU sampling edge (UM0050 pp. 73-81).
void cycle_sample_interrupts();
// Notify the board once that return-from-interrupt completed; test replay stays silent.
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
#ifdef GS2_SS_TESTING
    io(instruction_start.regs);
    io(instruction_start.control);
#endif

#ifdef GS2_SS_TESTING
    for (auto &h : history) {
        io(h.kind);
        io(h.address);
        io(h.data);
        io(h.nominal);
    }
#endif
}
