// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller

/*
 *   DD/FD-CB indexed bit operations for Second Sight's Zilog Z8S180.
 *   Z180.cpp includes this file; its decoder calls OP(family, hex-byte),
 *   which expands to a void member with no C++ parameters. Operands come
 *   from CPU registers or instruction bytes at PC. Results change those
 *   registers, flags or board memory; the handler returns no C++ value.
 *   Z180 owns the state and only its execution thread calls these functions.
 *   Memory and port helpers pass through the bus scheduler before SSCard.
 *
 *   A is the accumulator; F holds arithmetic flags. B/C, D/E and H/L form
 *   16-bit pairs BC, DE and HL. IX/IY are index registers, SP the stack pointer,
 *   and PC the next instruction address. HX/LX and HY/LY mean the high/low
 *   bytes of IX and IY. I supplies the interrupt-vector high byte; R counts
 *   opcode fetches. A prime denotes the alternate register set. In mnemonic
 *   notation, parentheses mean memory or a port; n/m are immediate bytes,
 *   w/a an immediate word/address, and o a signed 8-bit displacement. XY means
 *   the IX or IY selected by the prefix; ea holds its computed address.
 *
 *   Flags record sign, zero, half-carry (carry across bit 3), parity/overflow,
 *   subtraction and carry. PHI is the processor clock; a T-state is one PHI.
 *   The decoder supplies base timing; helpers add conditional/repeat timing.
 *   Empty self-load handlers still consume clocks. A trap is the processor's
 *   entry to address zero for an undefined instruction. The inherited Z80
 *   helpers for unsupported encodings remain here, but the Z180 decoder
 *   rejects them before execution.
 *
 *   Sources: Zilog UM005004-0918 pp. 173-183 for registers/addressing,
 *   pp. 211-236 for operations, pp. 247-250 for valid encodings, and
 *   pp. 251-280 for bus ordering. Page citations below use printed pages.
 */

/**********************************************************
 * opcodes with DD/FD CB prefix
 * rotate, shift and bit operations with (IX+o)
 **********************************************************/
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to B. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 00) {
    b8 = RLC(RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to C. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 01) {
    c8 = RLC(RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to D. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 02) {
    d8 = RLC(RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to E. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 03) {
    e8 = RLC(RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to H. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 04) {
    h8 = RLC(RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to L. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 05) {
    l8 = RLC(RM(ea));
    WM(ea, l8);
}
// Rotate memory at the saved indexed address left, wrapping bit 7 into bit 0. Set carry from
// the outgoing bit and flags from the result. (UM0050 pp. 216-220).
OP(xycb, 06) {
    WM(ea, RLC(RM(ea)));
}
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to A. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 07) {
    a8 = RLC(RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to B.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 08) {
    b8 = RRC(RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to C.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 09) {
    c8 = RRC(RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to D.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 0a) {
    d8 = RRC(RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to E.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 0b) {
    e8 = RRC(RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to H.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 0c) {
    h8 = RRC(RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to L.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 0d) {
    l8 = RRC(RM(ea));
    WM(ea, l8);
}
// Rotate memory at the saved indexed address right, wrapping bit 0 into bit 7. Set carry from
// the outgoing bit and flags from the result. (UM0050 pp. 216-220).
OP(xycb, 0e) {
    WM(ea, RRC(RM(ea)));
}
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to A.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 0f) {
    a8 = RRC(RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to B. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 10) {
    b8 = RL(RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to C. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 11) {
    c8 = RL(RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to D. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 12) {
    d8 = RL(RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to E. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 13) {
    e8 = RL(RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to H. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 14) {
    h8 = RL(RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to L. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 15) {
    l8 = RL(RM(ea));
    WM(ea, l8);
}
// Rotate memory at the saved indexed address left through carry. Set carry from the outgoing
// bit and flags from the result. (UM0050 pp. 216-220).
OP(xycb, 16) {
    WM(ea, RL(RM(ea)));
}
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to A. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 17) {
    a8 = RL(RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to B. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 18) {
    b8 = RR(RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to C. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 19) {
    c8 = RR(RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to D. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 1a) {
    d8 = RR(RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to E. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 1b) {
    e8 = RR(RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to H. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 1c) {
    h8 = RR(RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to L. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 1d) {
    l8 = RR(RM(ea));
    WM(ea, l8);
}
// Rotate memory at the saved indexed address right through carry. Set carry from the outgoing
// bit and flags from the result. (UM0050 pp. 216-220).
OP(xycb, 1e) {
    WM(ea, RR(RM(ea)));
}
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to A. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 1f) {
    a8 = RR(RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to B. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 20) {
    b8 = SLA(RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to C. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 21) {
    c8 = SLA(RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to D. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 22) {
    d8 = SLA(RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to E. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 23) {
    e8 = SLA(RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to H. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 24) {
    h8 = SLA(RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to L. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 25) {
    l8 = SLA(RM(ea));
    WM(ea, l8);
}
// Shift memory at the saved indexed address left, inserting zero. Set carry from the outgoing
// bit and flags from the result. (UM0050 pp. 216-220).
OP(xycb, 26) {
    WM(ea, SLA(RM(ea)));
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to A. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 27) {
    a8 = SLA(RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to B. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 28) {
    b8 = SRA(RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to C. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 29) {
    c8 = SRA(RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to D. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 2a) {
    d8 = SRA(RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to E. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 2b) {
    e8 = SRA(RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to H. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 2c) {
    h8 = SRA(RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to L. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 2d) {
    l8 = SRA(RM(ea));
    WM(ea, l8);
}
// Shift memory at the saved indexed address right, retaining its sign bit. Set carry from the
// outgoing bit and flags from the result. (UM0050 pp. 216-220).
OP(xycb, 2e) {
    WM(ea, SRA(RM(ea)));
}
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to A. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 2f) {
    a8 = SRA(RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to B. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 30) {
    b8 = SLL(RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to C. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 31) {
    c8 = SLL(RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to D. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 32) {
    d8 = SLL(RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to E. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 33) {
    e8 = SLL(RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to H. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 34) {
    h8 = SLL(RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to L. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 35) {
    l8 = SLL(RM(ea));
    WM(ea, l8);
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. The Z180 decoder traps this encoding (UM0050
// pp. 68-72, 247-250).
OP(xycb, 36) {
    WM(ea, SLL(RM(ea)));
}
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to A. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 37) {
    a8 = SLL(RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to B. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 38) {
    b8 = SRL(RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to C. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 39) {
    c8 = SRL(RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to D. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 3a) {
    d8 = SRL(RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to E. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 3b) {
    e8 = SRL(RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to H. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 3c) {
    h8 = SRL(RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to L. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 3d) {
    l8 = SRL(RM(ea));
    WM(ea, l8);
}
// Shift memory at the saved indexed address right, inserting zero. Set carry from the outgoing
// bit and flags from the result. (UM0050 pp. 216-220).
OP(xycb, 3e) {
    WM(ea, SRL(RM(ea)));
}
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to A. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 3f) {
    a8 = SRL(RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 40) {
    xycb_46();
}
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 41) {
    xycb_46();
}
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 42) {
    xycb_46();
}
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 43) {
    xycb_46();
}
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 44) {
    xycb_46();
}
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 45) {
    xycb_46();
}
// Test bit 0 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
OP(xycb, 46) {
    BIT_XY(0, RM(ea));
}
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 47) {
    xycb_46();
}

// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 48) {
    xycb_4e();
}
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 49) {
    xycb_4e();
}
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 4a) {
    xycb_4e();
}
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 4b) {
    xycb_4e();
}
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 4c) {
    xycb_4e();
}
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 4d) {
    xycb_4e();
}
// Test bit 1 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
OP(xycb, 4e) {
    BIT_XY(1, RM(ea));
}
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 4f) {
    xycb_4e();
}

// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 50) {
    xycb_56();
}
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 51) {
    xycb_56();
}
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 52) {
    xycb_56();
}
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 53) {
    xycb_56();
}
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 54) {
    xycb_56();
}
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 55) {
    xycb_56();
}
// Test bit 2 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
OP(xycb, 56) {
    BIT_XY(2, RM(ea));
}
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 57) {
    xycb_56();
}

// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 58) {
    xycb_5e();
}
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 59) {
    xycb_5e();
}
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 5a) {
    xycb_5e();
}
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 5b) {
    xycb_5e();
}
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 5c) {
    xycb_5e();
}
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 5d) {
    xycb_5e();
}
// Test bit 3 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
OP(xycb, 5e) {
    BIT_XY(3, RM(ea));
}
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 5f) {
    xycb_5e();
}

// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 60) {
    xycb_66();
}
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 61) {
    xycb_66();
}
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 62) {
    xycb_66();
}
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 63) {
    xycb_66();
}
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 64) {
    xycb_66();
}
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 65) {
    xycb_66();
}
// Test bit 4 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
OP(xycb, 66) {
    BIT_XY(4, RM(ea));
}
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 67) {
    xycb_66();
}

// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 68) {
    xycb_6e();
}
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 69) {
    xycb_6e();
}
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 6a) {
    xycb_6e();
}
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 6b) {
    xycb_6e();
}
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 6c) {
    xycb_6e();
}
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 6d) {
    xycb_6e();
}
// Test bit 5 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
OP(xycb, 6e) {
    BIT_XY(5, RM(ea));
}
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 6f) {
    xycb_6e();
}

// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 70) {
    xycb_76();
}
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 71) {
    xycb_76();
}
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 72) {
    xycb_76();
}
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 73) {
    xycb_76();
}
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 74) {
    xycb_76();
}
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 75) {
    xycb_76();
}
// Test bit 6 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
OP(xycb, 76) {
    BIT_XY(6, RM(ea));
}
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 77) {
    xycb_76();
}

// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 78) {
    xycb_7e();
}
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 79) {
    xycb_7e();
}
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 7a) {
    xycb_7e();
}
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 7b) {
    xycb_7e();
}
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 7c) {
    xycb_7e();
}
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 7d) {
    xycb_7e();
}
// Test bit 7 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
OP(xycb, 7e) {
    BIT_XY(7, RM(ea));
}
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, 7f) {
    xycb_7e();
}

// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 80) {
    b8 = RES(0, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 81) {
    c8 = RES(0, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 82) {
    d8 = RES(0, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 83) {
    e8 = RES(0, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 84) {
    h8 = RES(0, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 85) {
    l8 = RES(0, RM(ea));
    WM(ea, l8);
}
// Clear bit 0 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, 86) {
    WM(ea, RES(0, RM(ea)));
}
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 87) {
    a8 = RES(0, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 88) {
    b8 = RES(1, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 89) {
    c8 = RES(1, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 8a) {
    d8 = RES(1, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 8b) {
    e8 = RES(1, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 8c) {
    h8 = RES(1, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 8d) {
    l8 = RES(1, RM(ea));
    WM(ea, l8);
}
// Clear bit 1 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, 8e) {
    WM(ea, RES(1, RM(ea)));
}
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 8f) {
    a8 = RES(1, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 90) {
    b8 = RES(2, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 91) {
    c8 = RES(2, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 92) {
    d8 = RES(2, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 93) {
    e8 = RES(2, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 94) {
    h8 = RES(2, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 95) {
    l8 = RES(2, RM(ea));
    WM(ea, l8);
}
// Clear bit 2 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, 96) {
    WM(ea, RES(2, RM(ea)));
}
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 97) {
    a8 = RES(2, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 98) {
    b8 = RES(3, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 99) {
    c8 = RES(3, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 9a) {
    d8 = RES(3, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 9b) {
    e8 = RES(3, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 9c) {
    h8 = RES(3, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 9d) {
    l8 = RES(3, RM(ea));
    WM(ea, l8);
}
// Clear bit 3 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, 9e) {
    WM(ea, RES(3, RM(ea)));
}
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, 9f) {
    a8 = RES(3, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, a0) {
    b8 = RES(4, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, a1) {
    c8 = RES(4, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, a2) {
    d8 = RES(4, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, a3) {
    e8 = RES(4, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, a4) {
    h8 = RES(4, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, a5) {
    l8 = RES(4, RM(ea));
    WM(ea, l8);
}
// Clear bit 4 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, a6) {
    WM(ea, RES(4, RM(ea)));
}
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, a7) {
    a8 = RES(4, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, a8) {
    b8 = RES(5, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, a9) {
    c8 = RES(5, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, aa) {
    d8 = RES(5, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, ab) {
    e8 = RES(5, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, ac) {
    h8 = RES(5, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, ad) {
    l8 = RES(5, RM(ea));
    WM(ea, l8);
}
// Clear bit 5 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, ae) {
    WM(ea, RES(5, RM(ea)));
}
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, af) {
    a8 = RES(5, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, b0) {
    b8 = RES(6, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, b1) {
    c8 = RES(6, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, b2) {
    d8 = RES(6, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, b3) {
    e8 = RES(6, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, b4) {
    h8 = RES(6, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, b5) {
    l8 = RES(6, RM(ea));
    WM(ea, l8);
}
// Clear bit 6 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, b6) {
    WM(ea, RES(6, RM(ea)));
}
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, b7) {
    a8 = RES(6, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, b8) {
    b8 = RES(7, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, b9) {
    c8 = RES(7, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, ba) {
    d8 = RES(7, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, bb) {
    e8 = RES(7, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, bc) {
    h8 = RES(7, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, bd) {
    l8 = RES(7, RM(ea));
    WM(ea, l8);
}
// Clear bit 7 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, be) {
    WM(ea, RES(7, RM(ea)));
}
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
OP(xycb, bf) {
    a8 = RES(7, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, c0) {
    b8 = SET(0, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, c1) {
    c8 = SET(0, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, c2) {
    d8 = SET(0, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, c3) {
    e8 = SET(0, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, c4) {
    h8 = SET(0, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, c5) {
    l8 = SET(0, RM(ea));
    WM(ea, l8);
}
// Set bit 0 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, c6) {
    WM(ea, SET(0, RM(ea)));
}
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, c7) {
    a8 = SET(0, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, c8) {
    b8 = SET(1, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, c9) {
    c8 = SET(1, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, ca) {
    d8 = SET(1, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, cb) {
    e8 = SET(1, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, cc) {
    h8 = SET(1, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, cd) {
    l8 = SET(1, RM(ea));
    WM(ea, l8);
}
// Set bit 1 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, ce) {
    WM(ea, SET(1, RM(ea)));
}
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, cf) {
    a8 = SET(1, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, d0) {
    b8 = SET(2, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, d1) {
    c8 = SET(2, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, d2) {
    d8 = SET(2, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, d3) {
    e8 = SET(2, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, d4) {
    h8 = SET(2, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, d5) {
    l8 = SET(2, RM(ea));
    WM(ea, l8);
}
// Set bit 2 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, d6) {
    WM(ea, SET(2, RM(ea)));
}
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, d7) {
    a8 = SET(2, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, d8) {
    b8 = SET(3, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, d9) {
    c8 = SET(3, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, da) {
    d8 = SET(3, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, db) {
    e8 = SET(3, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, dc) {
    h8 = SET(3, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, dd) {
    l8 = SET(3, RM(ea));
    WM(ea, l8);
}
// Set bit 3 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, de) {
    WM(ea, SET(3, RM(ea)));
}
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, df) {
    a8 = SET(3, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, e0) {
    b8 = SET(4, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, e1) {
    c8 = SET(4, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, e2) {
    d8 = SET(4, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, e3) {
    e8 = SET(4, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, e4) {
    h8 = SET(4, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, e5) {
    l8 = SET(4, RM(ea));
    WM(ea, l8);
}
// Set bit 4 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, e6) {
    WM(ea, SET(4, RM(ea)));
}
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, e7) {
    a8 = SET(4, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, e8) {
    b8 = SET(5, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, e9) {
    c8 = SET(5, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, ea) {
    d8 = SET(5, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, eb) {
    e8 = SET(5, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, ec) {
    h8 = SET(5, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, ed) {
    l8 = SET(5, RM(ea));
    WM(ea, l8);
}
// Set bit 5 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, ee) {
    WM(ea, SET(5, RM(ea)));
}
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, ef) {
    a8 = SET(5, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, f0) {
    b8 = SET(6, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, f1) {
    c8 = SET(6, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, f2) {
    d8 = SET(6, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, f3) {
    e8 = SET(6, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, f4) {
    h8 = SET(6, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, f5) {
    l8 = SET(6, RM(ea));
    WM(ea, l8);
}
// Set bit 6 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, f6) {
    WM(ea, SET(6, RM(ea)));
}
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, f7) {
    a8 = SET(6, RM(ea));
    WM(ea, a8);
}

// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, f8) {
    b8 = SET(7, RM(ea));
    WM(ea, b8);
}
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, f9) {
    c8 = SET(7, RM(ea));
    WM(ea, c8);
}
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, fa) {
    d8 = SET(7, RM(ea));
    WM(ea, d8);
}
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, fb) {
    e8 = SET(7, RM(ea));
    WM(ea, e8);
}
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, fc) {
    h8 = SET(7, RM(ea));
    WM(ea, h8);
}
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, fd) {
    l8 = SET(7, RM(ea));
    WM(ea, l8);
}
// Set bit 7 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
OP(xycb, fe) {
    WM(ea, SET(7, RM(ea)));
}
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(xycb, ff) {
    a8 = SET(7, RM(ea));
    WM(ea, a8);
}
