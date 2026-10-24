/*
 *   Instruction-access helpers for Second Sight's Zilog Z8S180 processor.
 *   Z180.cpp includes these definitions after its private member declarations.
 *   Only the CPU execution thread calls them. They turn logical reads/writes
 *   into recorded bus operations and return sampled bytes to opcode handlers.
 *
 *   PHI is the processor clock; half-PHI is one clock edge interval. Ti means
 *   an internal clock with no memory/port transfer. MMU means memory management
 *   unit, translating logical addresses to physical memory. A certified read
 *   means SSCard promises a stable pointer with no read side effect; its byte
 *   still drives the emulated data bus. The journal retains completed accesses
 *   so a handler can resume without repeating device effects.
 *
 *   Source: Zilog UM005004-0918 pp. 19-30 and Tables 51-52, pp. 251-280.
 *   Deferring clocks and bypassing callbacks are implementation choices, valid
 *   only within the next board deadline and with unchanged pins.
 */

// Advance phi whole clocks and defer only peripheral bookkeeping until the next observation.
inline void Z180::cycle_defer_clocks(unsigned phi) {
    assert(!(tcr & 3) && !(rcr & 0x80) && !advance_left && icount >= int(2 * phi));
    icount -= 2 * phi;
    half_phi += 2 * phi;
    quiet_phi += phi;
}

// Return internal PHI clocks required before kind at the current bus position.
inline unsigned Z180::cycle_prelude(cycle_kind kind) const {
    unsigned const n = bus_number, op = opcode;
    if (!cycle_reference && !trapping) {
        if (family == Z180_TABLE_ed && op == 0x4d && !(omcr & 0x80)) {
            if (n == 2)
                return 3;
            if (n == 3)
                return 1;
        }
        if ((family == Z180_TABLE_cb || family == Z180_TABLE_xycb) && kind != cycle_kind::write)
            return 0;
        return n < 8 ? (prelude_schedule >> (3 * n)) & 7 : 0;
    }
    return cycle_prelude_reference(kind);
}

// Copy current pins and interrupt priority once for a span guaranteed to have no input changes.
inline void Z180::cycle_sample_stable_inputs() {
    sampled_iol = iol;
    std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
    sampled_bus_request = bus_released;
    cycle_sample_interrupts();
    fast_interrupt = sampled_interrupt;
    fast_irq_valid = true;
}

// Read from a board-approved pointer and journal the byte at logical address; return the byte.
inline uint8_t Z180::cycle_certified_read(cycle_kind kind, uint32_t address, uint8_t const *direct) {
    // The deadline proof excludes input changes; certification excludes
    // callbacks/waits. Drive the data bus once and retain the exact journal.
    if (!fast_irq_valid)
        cycle_sample_stable_inputs();
    uint8_t const data = *direct;
    drive_direct_read(data);
    cycle_defer_clocks(3);
    if (history_count == std::size(history))
        z180_error("Z180 instruction journal overflow at %04x", previous_pc);
    history[history_count++] = {kind, address, data, 3};
    quiet_record = history_count - 1;
    quiet_no_arbitration = false;
    ++cursor;
    nominal += 3;
    boundary_allowed = true;
    dma_allowed = !dma_start_pending;
    dma_start_pending = false;
    refresh_after_sleep = false;
    return data;
}

// Perform a complete logical access after prelude Ti clocks when the deadline permits it.
inline uint8_t Z180::cycle_whole_access(cycle_kind kind, uint32_t address, uint8_t data, unsigned prelude) {
    if (prelude)
        cycle_whole_internal(prelude, family == Z180_TABLE_op && opcode == 0x10);
    ++bus_number;
    if (kind != cycle_kind::write && direct_reads_enabled) {
        if (auto const page = direct_logical[(address >> 12) & 15])
            return cycle_certified_read(kind, address, page + (address & 4095));
    }
    uint32_t const physical = MMU_REMAP_ADDR(address);
    return cycle_mapped_access(kind, address, physical, data);
}

// Perform or replay a logical CPU access; return its byte. data is used for writes.
inline uint8_t Z180::cycle_access(cycle_kind kind, uint32_t address, uint8_t data) {
    unsigned const pre = cycle_prelude(kind);
    // A whole-instruction proof also certifies subsequent memory cycles.
    // Otherwise admit only this complete cycle before the next deadline.
    bool const admitted = instruction_whole || (!cycle_observer && !cycle.active && cursor == history_count);
    if (admitted && instruction_fast && !trapping && kind <= cycle_kind::write && icount >= int(2 * (pre + 3)))
        return cycle_whole_access(kind, address, data, pre);
    return cycle_access_slow(kind, address, data, pre);
}
