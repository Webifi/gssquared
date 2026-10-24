// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller

/*
 *   DD-prefixed IX index-register instructions for Second Sight's Zilog Z8S180.
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

// Retained invalid-prefix diagnostic helper. Reads the offending bytes for the diagnostic call;
// the Z180 decoder traps before these fallback handlers (UM0050 pp. 68-72).
OP(illegal, 1) {
    z180_diagnostic("Z180 '%s' ill. opcode_arg $%02x $%02x\n", "Z180", read_memory((pc32 - 1) & 0xffff),
                    read_memory(pc32));
}

/**********************************************************
 * IX register related opcodes (DD prefix)
 **********************************************************/
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $00. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 00) {
    illegal_1();
    op_00();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $01. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 01) {
    illegal_1();
    op_01();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $02. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 02) {
    illegal_1();
    op_02();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $03. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 03) {
    illegal_1();
    op_03();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $04. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 04) {
    illegal_1();
    op_04();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $05. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 05) {
    illegal_1();
    op_05();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $06. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 06) {
    illegal_1();
    op_06();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $07. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 07) {
    illegal_1();
    op_07();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $08. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 08) {
    illegal_1();
    op_08();
}
// Add BC to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(dd, 09) {
    ADD16(IX, BC);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 0a) {
    illegal_1();
    op_0a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 0b) {
    illegal_1();
    op_0b();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 0c) {
    illegal_1();
    op_0c();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 0d) {
    illegal_1();
    op_0d();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 0e) {
    illegal_1();
    op_0e();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 0f) {
    illegal_1();
    op_0f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $10. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 10) {
    illegal_1();
    op_10();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $11. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 11) {
    illegal_1();
    op_11();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $12. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 12) {
    illegal_1();
    op_12();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $13. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 13) {
    illegal_1();
    op_13();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $14. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 14) {
    illegal_1();
    op_14();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $15. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 15) {
    illegal_1();
    op_15();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $16. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 16) {
    illegal_1();
    op_16();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $17. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 17) {
    illegal_1();
    op_17();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $18. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 18) {
    illegal_1();
    op_18();
}
// Add DE to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(dd, 19) {
    ADD16(IX, DE);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 1a) {
    illegal_1();
    op_1a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 1b) {
    illegal_1();
    op_1b();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 1c) {
    illegal_1();
    op_1c();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 1d) {
    illegal_1();
    op_1d();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 1e) {
    illegal_1();
    op_1e();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 1f) {
    illegal_1();
    op_1f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $20. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 20) {
    illegal_1();
    op_20();
}
// Copy the next immediate word to IX. (UM0050 pp. 222-224).
OP(dd, 21) {
    ix16 = ARG16();
}
// Copy IX to memory at the encoded address. (UM0050 pp. 222-224).
OP(dd, 22) {
    ea = ARG16();
    WM16(ea, &IX);
}
// Increment IX with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
OP(dd, 23) {
    ix16++;
}
// Retained Z80 helper: Increment the high byte of IX with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 24) {
    ixh = INC(ixh);
}
// Retained Z80 helper: Decrement the high byte of IX with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 25) {
    ixh = DEC(ixh);
}
// Retained Z80 helper: Copy the next immediate byte to the high byte of IX. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 26) {
    ixh = ARG();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $27. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 27) {
    illegal_1();
    op_27();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $28. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 28) {
    illegal_1();
    op_28();
}
// Add IX to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(dd, 29) {
    ADD16(IX, IX);
}
// Copy memory at the encoded address to IX. (UM0050 pp. 222-224).
OP(dd, 2a) {
    ea = ARG16();
    RM16(ea, &IX);
}
// Decrement IX with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
OP(dd, 2b) {
    ix16--;
}
// Retained Z80 helper: Increment the low byte of IX with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 2c) {
    ixl = INC(ixl);
}
// Retained Z80 helper: Decrement the low byte of IX with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 2d) {
    ixl = DEC(ixl);
}
// Retained Z80 helper: Copy the next immediate byte to the low byte of IX. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 2e) {
    ixl = ARG();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $2F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 2f) {
    illegal_1();
    op_2f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $30. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 30) {
    illegal_1();
    op_30();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $31. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 31) {
    illegal_1();
    op_31();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $32. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 32) {
    illegal_1();
    op_32();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $33. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 33) {
    illegal_1();
    op_33();
}
// Increment memory at IX plus the signed displacement with 8-bit wrap; update arithmetic flags
// and preserve carry. (UM0050 p. 213).
OP(dd, 34) {
    EAX();
    WM(ea, INC(RM(ea)));
}
// Decrement memory at IX plus the signed displacement with 8-bit wrap; update arithmetic flags
// and preserve carry. (UM0050 p. 213).
OP(dd, 35) {
    EAX();
    WM(ea, DEC(RM(ea)));
}
// Copy the next immediate byte to memory at IX plus the signed displacement. (UM0050 pp.
// 222-224).
OP(dd, 36) {
    EAX();
    WM(ea, ARG());
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $37. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 37) {
    illegal_1();
    op_37();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $38. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 38) {
    illegal_1();
    op_38();
}
// Add SP to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(dd, 39) {
    ADD16(IX, SP);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 3a) {
    illegal_1();
    op_3a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 3b) {
    illegal_1();
    op_3b();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 3c) {
    illegal_1();
    op_3c();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 3d) {
    illegal_1();
    op_3d();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 3e) {
    illegal_1();
    op_3e();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 3f) {
    illegal_1();
    op_3f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $40. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 40) {
    illegal_1();
    op_40();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $41. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 41) {
    illegal_1();
    op_41();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $42. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 42) {
    illegal_1();
    op_42();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $43. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 43) {
    illegal_1();
    op_43();
}
// Retained Z80 helper: Copy the high byte of IX to B. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 44) {
    b8 = ixh;
}
// Retained Z80 helper: Copy the low byte of IX to B. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 45) {
    b8 = ixl;
}
// Copy memory at IX plus the signed displacement to B. (UM0050 pp. 222-224).
OP(dd, 46) {
    EAX();
    b8 = RM(ea);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $47. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 47) {
    illegal_1();
    op_47();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $48. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 48) {
    illegal_1();
    op_48();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $49. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 49) {
    illegal_1();
    op_49();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 4a) {
    illegal_1();
    op_4a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 4b) {
    illegal_1();
    op_4b();
}
// Retained Z80 helper: Copy the high byte of IX to C. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 4c) {
    c8 = ixh;
}
// Retained Z80 helper: Copy the low byte of IX to C. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 4d) {
    c8 = ixl;
}
// Copy memory at IX plus the signed displacement to C. (UM0050 pp. 222-224).
OP(dd, 4e) {
    EAX();
    c8 = RM(ea);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 4f) {
    illegal_1();
    op_4f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $50. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 50) {
    illegal_1();
    op_50();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $51. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 51) {
    illegal_1();
    op_51();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $52. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 52) {
    illegal_1();
    op_52();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $53. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 53) {
    illegal_1();
    op_53();
}
// Retained Z80 helper: Copy the high byte of IX to D. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 54) {
    d8 = ixh;
}
// Retained Z80 helper: Copy the low byte of IX to D. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 55) {
    d8 = ixl;
}
// Copy memory at IX plus the signed displacement to D. (UM0050 pp. 222-224).
OP(dd, 56) {
    EAX();
    d8 = RM(ea);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $57. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 57) {
    illegal_1();
    op_57();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $58. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 58) {
    illegal_1();
    op_58();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $59. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 59) {
    illegal_1();
    op_59();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 5a) {
    illegal_1();
    op_5a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 5b) {
    illegal_1();
    op_5b();
}
// Retained Z80 helper: Copy the high byte of IX to E. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 5c) {
    e8 = ixh;
}
// Retained Z80 helper: Copy the low byte of IX to E. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 5d) {
    e8 = ixl;
}
// Copy memory at IX plus the signed displacement to E. (UM0050 pp. 222-224).
OP(dd, 5e) {
    EAX();
    e8 = RM(ea);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 5f) {
    illegal_1();
    op_5f();
}

// Retained Z80 helper: Copy B to the high byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 60) {
    ixh = b8;
}
// Retained Z80 helper: Copy C to the high byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 61) {
    ixh = c8;
}
// Retained Z80 helper: Copy D to the high byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 62) {
    ixh = d8;
}
// Retained Z80 helper: Copy E to the high byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 63) {
    ixh = e8;
}
// Retained Z80 helper: Leave the high byte of IX unchanged; this self-load still consumes
// instruction time. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 64) {}
// Retained Z80 helper: Copy the low byte of IX to the high byte of IX. The Z180 decoder traps
// this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 65) {
    ixh = ixl;
}
// Copy memory at IX plus the signed displacement to H. (UM0050 pp. 222-224).
OP(dd, 66) {
    EAX();
    h8 = RM(ea);
}
// Retained Z80 helper: Copy A to the high byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 67) {
    ixh = a8;
}

// Retained Z80 helper: Copy B to the low byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 68) {
    ixl = b8;
}
// Retained Z80 helper: Copy C to the low byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 69) {
    ixl = c8;
}
// Retained Z80 helper: Copy D to the low byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 6a) {
    ixl = d8;
}
// Retained Z80 helper: Copy E to the low byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 6b) {
    ixl = e8;
}
// Retained Z80 helper: Copy the high byte of IX to the low byte of IX. The Z180 decoder traps
// this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 6c) {
    ixl = ixh;
}
// Retained Z80 helper: Leave the low byte of IX unchanged; this self-load still consumes
// instruction time. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 6d) {}
// Copy memory at IX plus the signed displacement to L. (UM0050 pp. 222-224).
OP(dd, 6e) {
    EAX();
    l8 = RM(ea);
}
// Retained Z80 helper: Copy A to the low byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 6f) {
    ixl = a8;
}

// Copy B to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
OP(dd, 70) {
    EAX();
    WM(ea, b8);
}
// Copy C to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
OP(dd, 71) {
    EAX();
    WM(ea, c8);
}
// Copy D to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
OP(dd, 72) {
    EAX();
    WM(ea, d8);
}
// Copy E to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
OP(dd, 73) {
    EAX();
    WM(ea, e8);
}
// Copy H to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
OP(dd, 74) {
    EAX();
    WM(ea, h8);
}
// Copy L to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
OP(dd, 75) {
    EAX();
    WM(ea, l8);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $76. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 76) {
    illegal_1();
    op_76();
}
// Copy A to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
OP(dd, 77) {
    EAX();
    WM(ea, a8);
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $78. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 78) {
    illegal_1();
    op_78();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $79. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 79) {
    illegal_1();
    op_79();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 7a) {
    illegal_1();
    op_7a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 7b) {
    illegal_1();
    op_7b();
}
// Retained Z80 helper: Copy the high byte of IX to A. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 7c) {
    a8 = ixh;
}
// Retained Z80 helper: Copy the low byte of IX to A. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(dd, 7d) {
    a8 = ixl;
}
// Copy memory at IX plus the signed displacement to A. (UM0050 pp. 222-224).
OP(dd, 7e) {
    EAX();
    a8 = RM(ea);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 7f) {
    illegal_1();
    op_7f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $80. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 80) {
    illegal_1();
    op_80();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $81. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 81) {
    illegal_1();
    op_81();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $82. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 82) {
    illegal_1();
    op_82();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $83. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 83) {
    illegal_1();
    op_83();
}
// Retained Z80 helper: Add the high byte of IX to A. Store the wrapped result and update 8-bit
// arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 84) {
    ADD(ixh);
}
// Retained Z80 helper: Add the low byte of IX to A. Store the wrapped result and update 8-bit
// arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 85) {
    ADD(ixl);
}
// Add memory at IX plus the signed displacement to A. Store the wrapped result and update 8-bit
// arithmetic flags. (UM0050 pp. 211-212).
OP(dd, 86) {
    EAX();
    ADD(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $87. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 87) {
    illegal_1();
    op_87();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $88. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 88) {
    illegal_1();
    op_88();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $89. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 89) {
    illegal_1();
    op_89();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 8a) {
    illegal_1();
    op_8a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 8b) {
    illegal_1();
    op_8b();
}
// Retained Z80 helper: Add the high byte of IX and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(dd, 8c) {
    ADC(ixh);
}
// Retained Z80 helper: Add the low byte of IX and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(dd, 8d) {
    ADC(ixl);
}
// Add memory at IX plus the signed displacement and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. (UM0050 pp. 211-212).
OP(dd, 8e) {
    EAX();
    ADC(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 8f) {
    illegal_1();
    op_8f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $90. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 90) {
    illegal_1();
    op_90();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $91. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 91) {
    illegal_1();
    op_91();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $92. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 92) {
    illegal_1();
    op_92();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $93. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 93) {
    illegal_1();
    op_93();
}
// Retained Z80 helper: Subtract the high byte of IX from A. Store the wrapped result and update
// 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 94) {
    SUB(ixh);
}
// Retained Z80 helper: Subtract the low byte of IX from A. Store the wrapped result and update
// 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, 95) {
    SUB(ixl);
}
// Subtract memory at IX plus the signed displacement from A. Store the wrapped result and
// update 8-bit arithmetic flags. (UM0050 pp. 211-212).
OP(dd, 96) {
    EAX();
    SUB(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $97. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 97) {
    illegal_1();
    op_97();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $98. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 98) {
    illegal_1();
    op_98();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $99. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 99) {
    illegal_1();
    op_99();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 9a) {
    illegal_1();
    op_9a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 9b) {
    illegal_1();
    op_9b();
}
// Retained Z80 helper: Subtract the high byte of IX and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(dd, 9c) {
    SBC(ixh);
}
// Retained Z80 helper: Subtract the low byte of IX and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(dd, 9d) {
    SBC(ixl);
}
// Subtract memory at IX plus the signed displacement and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
OP(dd, 9e) {
    EAX();
    SBC(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, 9f) {
    illegal_1();
    op_9f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, a0) {
    illegal_1();
    op_a0();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, a1) {
    illegal_1();
    op_a1();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, a2) {
    illegal_1();
    op_a2();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, a3) {
    illegal_1();
    op_a3();
}
// Retained Z80 helper: Combine A with the high byte of IX using bitwise AND. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, a4) {
    AND(ixh);
}
// Retained Z80 helper: Combine A with the low byte of IX using bitwise AND. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, a5) {
    AND(ixl);
}
// Combine A with memory at IX plus the signed displacement using bitwise AND. Store A and set
// logical-result flags. (UM0050 p. 214).
OP(dd, a6) {
    EAX();
    AND(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, a7) {
    illegal_1();
    op_a7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, a8) {
    illegal_1();
    op_a8();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, a9) {
    illegal_1();
    op_a9();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, aa) {
    illegal_1();
    op_aa();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, ab) {
    illegal_1();
    op_ab();
}
// Retained Z80 helper: Combine A with the high byte of IX using bitwise exclusive OR. Store A
// and set logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(dd, ac) {
    XOR(ixh);
}
// Retained Z80 helper: Combine A with the low byte of IX using bitwise exclusive OR. Store A
// and set logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(dd, ad) {
    XOR(ixl);
}
// Combine A with memory at IX plus the signed displacement using bitwise exclusive OR. Store A
// and set logical-result flags. (UM0050 p. 214).
OP(dd, ae) {
    EAX();
    XOR(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, af) {
    illegal_1();
    op_af();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, b0) {
    illegal_1();
    op_b0();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, b1) {
    illegal_1();
    op_b1();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, b2) {
    illegal_1();
    op_b2();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, b3) {
    illegal_1();
    op_b3();
}
// Retained Z80 helper: Combine A with the high byte of IX using bitwise OR. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, b4) {
    OR(ixh);
}
// Retained Z80 helper: Combine A with the low byte of IX using bitwise OR. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, b5) {
    OR(ixl);
}
// Combine A with memory at IX plus the signed displacement using bitwise OR. Store A and set
// logical-result flags. (UM0050 p. 214).
OP(dd, b6) {
    EAX();
    OR(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, b7) {
    illegal_1();
    op_b7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, b8) {
    illegal_1();
    op_b8();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, b9) {
    illegal_1();
    op_b9();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, ba) {
    illegal_1();
    op_ba();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, bb) {
    illegal_1();
    op_bb();
}
// Retained Z80 helper: Compare A with the high byte of IX by setting subtraction flags; leave A
// unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, bc) {
    CP(ixh);
}
// Retained Z80 helper: Compare A with the low byte of IX by setting subtraction flags; leave A
// unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(dd, bd) {
    CP(ixl);
}
// Compare A with memory at IX plus the signed displacement by setting subtraction flags; leave
// A unchanged. (UM0050 p. 213).
OP(dd, be) {
    EAX();
    CP(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, bf) {
    illegal_1();
    op_bf();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, c0) {
    illegal_1();
    op_c0();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, c1) {
    illegal_1();
    op_c1();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, c2) {
    illegal_1();
    op_c2();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, c3) {
    illegal_1();
    op_c3();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, c4) {
    illegal_1();
    op_c4();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, c5) {
    illegal_1();
    op_c5();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, c6) {
    illegal_1();
    op_c6();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, c7) {
    illegal_1();
    op_c7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, c8) {
    illegal_1();
    op_c8();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, c9) {
    illegal_1();
    op_c9();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, ca) {
    illegal_1();
    op_ca();
}
// Read the signed index displacement, save its effective address, then fetch and dispatch the
// CB bit-operation byte (UM0050 pp. 182, 249, 267-271).
OP(dd, cb) {
    EAX();
    extra_cycles += exec_xycb(ROP());
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, cc) {
    illegal_1();
    op_cc();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, cd) {
    illegal_1();
    op_cd();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, ce) {
    illegal_1();
    op_ce();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, cf) {
    illegal_1();
    op_cf();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, d0) {
    illegal_1();
    op_d0();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, d1) {
    illegal_1();
    op_d1();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, d2) {
    illegal_1();
    op_d2();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, d3) {
    illegal_1();
    op_d3();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, d4) {
    illegal_1();
    op_d4();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, d5) {
    illegal_1();
    op_d5();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, d6) {
    illegal_1();
    op_d6();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, d7) {
    illegal_1();
    op_d7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, d8) {
    illegal_1();
    op_d8();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, d9) {
    illegal_1();
    op_d9();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, da) {
    illegal_1();
    op_da();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, db) {
    illegal_1();
    op_db();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, dc) {
    illegal_1();
    op_dc();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, dd) {
    illegal_1();
    op_dd();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, de) {
    illegal_1();
    op_de();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, df) {
    illegal_1();
    op_df();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, e0) {
    illegal_1();
    op_e0();
}
// Read the low then high stack bytes into IX and advance SP. (UM0050 p. 227).
OP(dd, e1) {
    POP(IX);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, e2) {
    illegal_1();
    op_e2();
}
// Exchange the word at SP and IX without changing arithmetic flags. (UM0050 p. 228).
OP(dd, e3) {
    EXSP(IX);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, e4) {
    illegal_1();
    op_e4();
}
// Store IX on the descending stack, high byte first. (UM0050 p. 227).
OP(dd, e5) {
    PUSH(IX);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, e6) {
    illegal_1();
    op_e6();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, e7) {
    illegal_1();
    op_e7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, e8) {
    illegal_1();
    op_e8();
}
// Set PC to IX (the register value, without a memory read). (UM0050 pp. 229-230).
OP(dd, e9) {
    pc16 = ix16;
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, ea) {
    illegal_1();
    op_ea();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, eb) {
    illegal_1();
    op_eb();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, ec) {
    illegal_1();
    op_ec();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $ED. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, ed) {
    illegal_1();
    op_ed();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, ee) {
    illegal_1();
    op_ee();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, ef) {
    illegal_1();
    op_ef();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, f0) {
    illegal_1();
    op_f0();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, f1) {
    illegal_1();
    op_f1();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, f2) {
    illegal_1();
    op_f2();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, f3) {
    illegal_1();
    op_f3();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, f4) {
    illegal_1();
    op_f4();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, f5) {
    illegal_1();
    op_f5();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, f6) {
    illegal_1();
    op_f6();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, f7) {
    illegal_1();
    op_f7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, f8) {
    illegal_1();
    op_f8();
}
// Copy IX to SP. (UM0050 pp. 222-224).
OP(dd, f9) {
    sp16 = ix16;
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, fa) {
    illegal_1();
    op_fa();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, fb) {
    illegal_1();
    op_fb();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, fc) {
    illegal_1();
    op_fc();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, fd) {
    illegal_1();
    op_fd();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, fe) {
    illegal_1();
    op_fe();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(dd, ff) {
    illegal_1();
    op_ff();
}
