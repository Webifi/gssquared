// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller

/*
 *   ED-prefixed extended instructions for Second Sight's Zilog Z8S180.
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
 *   NMI is the non-maskable interrupt; INT0 is the first maskable input.
 *   M1 identifies an opcode fetch. IOCR controls internal I/O and CCR the
 *   processor clock. An interrupt-enable flag decides whether a request
 *   may be accepted between instructions.
 *
 *   Sources: Zilog UM005004-0918 pp. 173-183 for registers/addressing,
 *   pp. 211-236 for operations, pp. 247-250 for valid encodings, and
 *   pp. 251-280 for bus ordering. Page citations below use printed pages.
 */

// Retained invalid-prefix diagnostic helper. Reads the offending bytes for the diagnostic call;
// the Z180 decoder traps before these fallback handlers (UM0050 pp. 68-72).
OP(illegal, 2) {
    z180_diagnostic("Z180 '%s' ill. opcode_arg $ed $%02x\n", "Z180", read_memory((pc32 - 1) & 0xffff));
}

/**********************************************************
 * special opcodes (ED prefix)
 **********************************************************/
// IN0 affects SZHPN exactly like IN g,(C), preserving carry (UM0050 p231).
// Read the immediate 8-bit port into B. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 00) {
    unsigned n = ARG();
    b8 = IN(n);
    f8 = (f8 & CF) | SZP[b8];
}
// Write B to the immediate 8-bit port. (UM0050 pp. 231-234).
OP(ed, 01) {
    unsigned n = ARG();
    OUT(n, b8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 02) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 03) {
    illegal_2();
}
// Set flags from A AND B without changing either operand. (UM0050 p. 214).
OP(ed, 04) {
    TST(b8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 05) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 06) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 07) {
    illegal_2();
}

// Read the immediate 8-bit port into C. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 08) {
    unsigned n = ARG();
    c8 = IN(n);
    f8 = (f8 & CF) | SZP[c8];
}
// Write C to the immediate 8-bit port. (UM0050 pp. 231-234).
OP(ed, 09) {
    unsigned n = ARG();
    OUT(n, c8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 0a) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 0b) {
    illegal_2();
}
// Set flags from A AND C without changing either operand. (UM0050 p. 214).
OP(ed, 0c) {
    TST(c8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 0d) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 0e) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 0f) {
    illegal_2();
}

// Read the immediate 8-bit port into D. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 10) {
    unsigned n = ARG();
    d8 = IN(n);
    f8 = (f8 & CF) | SZP[d8];
}
// Write D to the immediate 8-bit port. (UM0050 pp. 231-234).
OP(ed, 11) {
    unsigned n = ARG();
    OUT(n, d8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 12) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 13) {
    illegal_2();
}
// Set flags from A AND D without changing either operand. (UM0050 p. 214).
OP(ed, 14) {
    TST(d8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 15) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 16) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 17) {
    illegal_2();
}

// Read the immediate 8-bit port into E. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 18) {
    unsigned n = ARG();
    e8 = IN(n);
    f8 = (f8 & CF) | SZP[e8];
}
// Write E to the immediate 8-bit port. (UM0050 pp. 231-234).
OP(ed, 19) {
    unsigned n = ARG();
    OUT(n, e8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 1a) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 1b) {
    illegal_2();
}
// Set flags from A AND E without changing either operand. (UM0050 p. 214).
OP(ed, 1c) {
    TST(e8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 1d) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 1e) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 1f) {
    illegal_2();
}

// Read the immediate 8-bit port into H. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 20) {
    unsigned n = ARG();
    h8 = IN(n);
    f8 = (f8 & CF) | SZP[h8];
}
// Write H to the immediate 8-bit port. (UM0050 pp. 231-234).
OP(ed, 21) {
    unsigned n = ARG();
    OUT(n, h8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 22) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 23) {
    illegal_2();
}
// Set flags from A AND H without changing either operand. (UM0050 p. 214).
OP(ed, 24) {
    TST(h8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 25) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 26) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 27) {
    illegal_2();
}

// Read the immediate 8-bit port into L. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 28) {
    unsigned n = ARG();
    l8 = IN(n);
    f8 = (f8 & CF) | SZP[l8];
}
// Write L to the immediate 8-bit port. (UM0050 pp. 231-234).
OP(ed, 29) {
    unsigned n = ARG();
    OUT(n, l8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 2a) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 2b) {
    illegal_2();
}
// Set flags from A AND L without changing either operand. (UM0050 p. 214).
OP(ed, 2c) {
    TST(l8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 2d) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 2e) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 2f) {
    illegal_2();
}

// Read the immediate 8-bit port; discard the byte but update flags. (UM0050 pp. 231-234).
OP(ed, 30) {
    unsigned n = ARG();
    uint8_t res = IN(n);
    f8 = (f8 & CF) | SZP[res];
}
// Retained Z80 helper: Write zero to the immediate 8-bit port. The Z180 decoder traps this
// encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 31) {
    unsigned n = ARG();
    OUT(n, 0);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 32) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 33) {
    illegal_2();
}
// Set flags from A AND memory at HL without changing either operand. (UM0050 p. 214).
OP(ed, 34) {
    TST(RM(hl16));
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 35) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 36) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 37) {
    illegal_2();
}

// Read the immediate 8-bit port into A. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 38) {
    unsigned n = ARG();
    a8 = IN(n);
    f8 = (f8 & CF) | SZP[a8];
}
// Write A to the immediate 8-bit port. (UM0050 pp. 231-234).
OP(ed, 39) {
    unsigned n = ARG();
    OUT(n, a8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 3a) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 3b) {
    illegal_2();
}
// Set flags from A AND A without changing either operand. (UM0050 p. 214).
OP(ed, 3c) {
    TST(a8);
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 3d) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 3e) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 3f) {
    illegal_2();
}

// Read the 16-bit port BC into B. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 40) {
    b8 = IN(bc16);
    f8 = (f8 & CF) | SZP[b8];
}
// Write B to the 16-bit port BC. (UM0050 pp. 231-234).
OP(ed, 41) {
    OUT(bc16, b8);
}
// Subtract BC and carry from HL. Store the wrapped result and update 16-bit arithmetic flags.
// (UM0050 p. 221).
OP(ed, 42) {
    SBC16(BC);
}
// Copy BC to memory at the encoded address. (UM0050 pp. 222-224).
OP(ed, 43) {
    ea = ARG16();
    WM16(ea, &BC);
}
// Replace A with zero minus A and set subtraction flags. (UM0050 p. 215).
OP(ed, 44) {
    NEG;
}
// Pop the return PC and restore the interrupt enable saved on NMI entry. (UM0050 pp. 73-75,
// 230).
OP(ed, 45) {
    RETN;
}
// Select INT0 interrupt mode 0. Other interrupt sources retain their fixed vector rules.
// (UM0050 pp. 75-86, 235).
OP(ed, 46) {
    IM = 0;
}
// Copy A to I. (UM0050 pp. 222-224).
OP(ed, 47) {
    LD_I_A;
}

// Read the 16-bit port BC into C. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 48) {
    c8 = IN(bc16);
    f8 = (f8 & CF) | SZP[c8];
}
// Write C to the 16-bit port BC. (UM0050 pp. 231-234).
OP(ed, 49) {
    OUT(bc16, c8);
}
// Add BC and carry to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050
// p. 221).
OP(ed, 4a) {
    ADC16(BC);
}
// Copy memory at the encoded address to BC. (UM0050 pp. 222-224).
OP(ed, 4b) {
    ea = ARG16();
    RM16(ea, &BC);
}
// Multiply the low and high bytes of BC as unsigned values; store their 16-bit product in that
// pair. (UM0050 p. 221).
OP(ed, 4c) {
    MLT(BC);
}
// Pop the return PC and signal return-from-interrupt to the board; M1 mode may add refetch
// cycles. (UM0050 pp. 84-85, 230).
OP(ed, 4d) {
    RETI;
}
// Retained Z80 helper: Select INT0 interrupt mode 0. Other interrupt sources retain their fixed
// vector rules. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 4e) {
    IM = 0;
}
// Copy A to R. (UM0050 pp. 222-224).
OP(ed, 4f) {
    LD_R_A;
}

// Read the 16-bit port BC into D. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 50) {
    d8 = IN(bc16);
    f8 = (f8 & CF) | SZP[d8];
}
// Write D to the 16-bit port BC. (UM0050 pp. 231-234).
OP(ed, 51) {
    OUT(bc16, d8);
}
// Subtract DE and carry from HL. Store the wrapped result and update 16-bit arithmetic flags.
// (UM0050 p. 221).
OP(ed, 52) {
    SBC16(DE);
}
// Copy DE to memory at the encoded address. (UM0050 pp. 222-224).
OP(ed, 53) {
    ea = ARG16();
    WM16(ea, &DE);
}
// Retained Z80 helper: Replace A with zero minus A and set subtraction flags. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 54) {
    NEG;
}
// Retained Z80 helper: Pop the return PC and restore the interrupt enable saved on NMI entry.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 55) {
    RETN;
}
// Select INT0 interrupt mode 1. Other interrupt sources retain their fixed vector rules.
// (UM0050 pp. 75-86, 235).
OP(ed, 56) {
    IM = 1;
}
// Copy I to A. Set parity/overflow from the saved interrupt enable. (UM0050 pp. 222-224).
OP(ed, 57) {
    LD_A_I;
}

// Read the 16-bit port BC into E. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 58) {
    e8 = IN(bc16);
    f8 = (f8 & CF) | SZP[e8];
}
// Write E to the 16-bit port BC. (UM0050 pp. 231-234).
OP(ed, 59) {
    OUT(bc16, e8);
}
// Add DE and carry to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050
// p. 221).
OP(ed, 5a) {
    ADC16(DE);
}
// Copy memory at the encoded address to DE. (UM0050 pp. 222-224).
OP(ed, 5b) {
    ea = ARG16();
    RM16(ea, &DE);
}
// Multiply the low and high bytes of DE as unsigned values; store their 16-bit product in that
// pair. (UM0050 p. 221).
OP(ed, 5c) {
    MLT(DE);
}
// Retained Z80 helper: Pop the return PC and signal return-from-interrupt to the board; M1 mode
// may add refetch cycles. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 5d) {
    RETI;
}
// Select INT0 interrupt mode 2. Other interrupt sources retain their fixed vector rules.
// (UM0050 pp. 75-86, 235).
OP(ed, 5e) {
    IM = 2;
}
// Copy R to A. Set parity/overflow from the saved interrupt enable. (UM0050 pp. 222-224).
OP(ed, 5f) {
    LD_A_R;
}

// Read the 16-bit port BC into H. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 60) {
    h8 = IN(bc16);
    f8 = (f8 & CF) | SZP[h8];
}
// Write H to the 16-bit port BC. (UM0050 pp. 231-234).
OP(ed, 61) {
    OUT(bc16, h8);
}
// Subtract HL and carry from HL. Store the wrapped result and update 16-bit arithmetic flags.
// (UM0050 p. 221).
OP(ed, 62) {
    SBC16(HL);
}
// Copy HL to memory at the encoded address. (UM0050 pp. 222-224).
OP(ed, 63) {
    ea = ARG16();
    WM16(ea, &HL);
}
// Set flags from A AND the next immediate mask without changing either operand. (UM0050 p.
// 214).
OP(ed, 64) {
    unsigned m = ARG();
    TST(m);
}
// Retained Z80 helper: Pop the return PC and restore the interrupt enable saved on NMI entry.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 65) {
    RETN;
}
// Retained Z80 helper: Select INT0 interrupt mode 0. Other interrupt sources retain their fixed
// vector rules. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 66) {
    IM = 0;
}
// Rotate four-bit digits right between A's low digit and memory at HL; preserve carry. (UM0050
// p. 218).
OP(ed, 67) {
    RRD;
}

// Read the 16-bit port BC into L. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 68) {
    l8 = IN(bc16);
    f8 = (f8 & CF) | SZP[l8];
}
// Write L to the 16-bit port BC. (UM0050 pp. 231-234).
OP(ed, 69) {
    OUT(bc16, l8);
}
// Add HL and carry to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050
// p. 221).
OP(ed, 6a) {
    ADC16(HL);
}
// Copy memory at the encoded address to HL. (UM0050 pp. 222-224).
OP(ed, 6b) {
    ea = ARG16();
    RM16(ea, &HL);
}
// Multiply the low and high bytes of HL as unsigned values; store their 16-bit product in that
// pair. (UM0050 p. 221).
OP(ed, 6c) {
    MLT(HL);
}
// Retained Z80 helper: Pop the return PC and signal return-from-interrupt to the board; M1 mode
// may add refetch cycles. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 6d) {
    RETI;
}
// Retained Z80 helper: Select INT0 interrupt mode 0. Other interrupt sources retain their fixed
// vector rules. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 6e) {
    IM = 0;
}
// Rotate four-bit digits left between A's low digit and memory at HL; preserve carry. (UM0050
// p. 218).
OP(ed, 6f) {
    RLD;
}

// Read the 16-bit port BC; discard the byte but update flags. (UM0050 pp. 231-234).
OP(ed, 70) {
    uint8_t res = IN(bc16);
    f8 = (f8 & CF) | SZP[res];
}
// Retained Z80 helper: Write 0 to the 16-bit port BC. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(ed, 71) {
    OUT(bc16, 0);
}
// Subtract SP and carry from HL. Store the wrapped result and update 16-bit arithmetic flags.
// (UM0050 p. 221).
OP(ed, 72) {
    SBC16(SP);
}
// Copy SP to memory at the encoded address. (UM0050 pp. 222-224).
OP(ed, 73) {
    ea = ARG16();
    WM16(ea, &SP);
}
// UM0050 p233: TSTIO is a test/AND, not IN; H is set and C is cleared.
// Read port C, AND it with the immediate mask, and set flags without retaining the byte.
// (UM0050 p. 234).
OP(ed, 74) {
    unsigned m = ARG();
    f8 = HF | SZP[IN(c8) & m];
}
// Retained Z80 helper: Pop the return PC and restore the interrupt enable saved on NMI entry.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 75) {
    RETN;
}
// Enter SLP; clock stopping and wake delay depend on IOCR/CCR and silicon revision. (UM0050 pp.
// 34-41, 235).
OP(ed, 76) {
    SLP;
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 77) {
    illegal_2();
}

// Read the 16-bit port BC into A. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
OP(ed, 78) {
    a8 = IN(bc16);
    f8 = (f8 & CF) | SZP[a8];
}
// Write A to the 16-bit port BC. (UM0050 pp. 231-234).
OP(ed, 79) {
    OUT(bc16, a8);
}
// Add SP and carry to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050
// p. 221).
OP(ed, 7a) {
    ADC16(SP);
}
// Copy memory at the encoded address to SP. (UM0050 pp. 222-224).
OP(ed, 7b) {
    ea = ARG16();
    RM16(ea, &SP);
}
// Multiply the low and high bytes of SP as unsigned values; store their 16-bit product in that
// pair. (UM0050 p. 221).
OP(ed, 7c) {
    MLT(SP);
}
// Retained Z80 helper: Pop the return PC and signal return-from-interrupt to the board; M1 mode
// may add refetch cycles. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 7d) {
    RETI;
}
// Retained Z80 helper: Select INT0 interrupt mode 2. Other interrupt sources retain their fixed
// vector rules. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(ed, 7e) {
    IM = 2;
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 7f) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 80) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 81) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 82) {
    illegal_2();
}
// Write memory at HL to port C; decrement B and increment HL. Move C in the same direction.
// (UM0050 pp. 232-234).
OP(ed, 83) {
    OTIM;
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 84) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 85) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 86) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 87) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 88) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 89) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 8a) {
    illegal_2();
}
// Write memory at HL to port C; decrement B and decrement HL. Move C in the same direction.
// (UM0050 pp. 232-234).
OP(ed, 8b) {
    OTDM;
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 8c) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 8d) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 8e) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 8f) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 90) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 91) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 92) {
    illegal_2();
}
// Write memory at HL to port C; decrement B and increment HL. Move C in the same direction.
// Repeat until B reaches zero; PC backs up so each byte remains interruptible. (UM0050 pp.
// 232-234).
OP(ed, 93) {
    OTIMR;
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 94) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 95) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 96) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 97) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 98) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 99) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 9a) {
    illegal_2();
}
// Write memory at HL to port C; decrement B and decrement HL. Move C in the same direction.
// Repeat until B reaches zero; PC backs up so each byte remains interruptible. (UM0050 pp.
// 232-234).
OP(ed, 9b) {
    OTDMR;
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 9c) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 9d) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 9e) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, 9f) {
    illegal_2();
}

// Copy one byte from HL to DE; increment both addresses and decrement BC. (UM0050 p. 225).
OP(ed, a0) {
    LDI;
}
// Compare A with memory at HL; increment HL and decrement BC. (UM0050 p. 226).
OP(ed, a1) {
    CPI;
}
// Read port BC into memory at HL; decrement B and increment HL. (UM0050 pp. 232-234).
OP(ed, a2) {
    INI;
}
// Read memory at HL, decrement B, then write to port BC and increment HL. (UM0050 pp. 232-234).
OP(ed, a3) {
    OUTI;
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, a4) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, a5) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, a6) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, a7) {
    illegal_2();
}

// Copy one byte from HL to DE; decrement both addresses and decrement BC. (UM0050 p. 225).
OP(ed, a8) {
    LDD;
}
// Compare A with memory at HL; decrement HL and decrement BC. (UM0050 p. 226).
OP(ed, a9) {
    CPD;
}
// Read port BC into memory at HL; decrement B and decrement HL. (UM0050 pp. 232-234).
OP(ed, aa) {
    IND;
}
// Read memory at HL, decrement B, then write to port BC and decrement HL. (UM0050 pp. 232-234).
OP(ed, ab) {
    OUTD;
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ac) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ad) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ae) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, af) {
    illegal_2();
}

// Copy one byte from HL to DE; increment both addresses and decrement BC. Repeat by backing up
// PC while BC is nonzero, allowing interrupts between bytes. (UM0050 p. 225).
OP(ed, b0) {
    LDIR;
}
// Compare A with memory at HL; increment HL and decrement BC. Repeat until a match or BC
// reaches zero. (UM0050 p. 226).
OP(ed, b1) {
    CPIR;
}
// Read port BC into memory at HL; decrement B and increment HL. Repeat until B reaches zero; PC
// backs up so each byte remains interruptible. (UM0050 pp. 232-234).
OP(ed, b2) {
    INIR;
}
// Read memory at HL, decrement B, then write to port BC and increment HL. Repeat until B reaches zero; PC
// backs up so each byte remains interruptible. (UM0050 pp. 232-234).
OP(ed, b3) {
    OTIR;
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, b4) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, b5) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, b6) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, b7) {
    illegal_2();
}

// Copy one byte from HL to DE; decrement both addresses and decrement BC. Repeat by backing up
// PC while BC is nonzero, allowing interrupts between bytes. (UM0050 p. 225).
OP(ed, b8) {
    LDDR;
}
// Compare A with memory at HL; decrement HL and decrement BC. Repeat until a match or BC
// reaches zero. (UM0050 p. 226).
OP(ed, b9) {
    CPDR;
}
// Read port BC into memory at HL; decrement B and decrement HL. Repeat until B reaches zero; PC
// backs up so each byte remains interruptible. (UM0050 pp. 232-234).
OP(ed, ba) {
    INDR;
}
// Read memory at HL, decrement B, then write to port BC and decrement HL. Repeat until B reaches zero; PC
// backs up so each byte remains interruptible. (UM0050 pp. 232-234).
OP(ed, bb) {
    OTDR;
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, bc) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, bd) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, be) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, bf) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, c0) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, c1) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, c2) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, c3) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, c4) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, c5) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, c6) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, c7) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, c8) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, c9) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ca) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, cb) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, cc) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, cd) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ce) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, cf) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, d0) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, d1) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, d2) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, d3) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, d4) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, d5) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, d6) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, d7) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, d8) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, d9) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, da) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, db) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, dc) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, dd) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, de) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, df) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, e0) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, e1) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, e2) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, e3) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, e4) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, e5) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, e6) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, e7) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, e8) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, e9) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ea) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, eb) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ec) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ed) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ee) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ef) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, f0) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, f1) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, f2) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, f3) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, f4) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, f5) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, f6) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, f7) {
    illegal_2();
}

// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, f8) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, f9) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, fa) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, fb) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, fc) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, fd) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, fe) {
    illegal_2();
}
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
OP(ed, ff) {
    illegal_2();
}
