// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller

/*
 *   FD-prefixed IY index-register instructions for Second Sight's Zilog Z8S180.
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
 * IY register related opcodes (FD prefix)
 **********************************************************/
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $00. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 00) {
    illegal_1();
    op_00();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $01. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 01) {
    illegal_1();
    op_01();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $02. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 02) {
    illegal_1();
    op_02();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $03. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 03) {
    illegal_1();
    op_03();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $04. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 04) {
    illegal_1();
    op_04();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $05. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 05) {
    illegal_1();
    op_05();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $06. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 06) {
    illegal_1();
    op_06();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $07. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 07) {
    illegal_1();
    op_07();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $08. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 08) {
    illegal_1();
    op_08();
}
// Add BC to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(fd, 09) {
    ADD16(IY, BC);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 0a) {
    illegal_1();
    op_0a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 0b) {
    illegal_1();
    op_0b();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 0c) {
    illegal_1();
    op_0c();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 0d) {
    illegal_1();
    op_0d();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 0e) {
    illegal_1();
    op_0e();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 0f) {
    illegal_1();
    op_0f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $10. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 10) {
    illegal_1();
    op_10();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $11. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 11) {
    illegal_1();
    op_11();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $12. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 12) {
    illegal_1();
    op_12();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $13. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 13) {
    illegal_1();
    op_13();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $14. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 14) {
    illegal_1();
    op_14();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $15. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 15) {
    illegal_1();
    op_15();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $16. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 16) {
    illegal_1();
    op_16();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $17. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 17) {
    illegal_1();
    op_17();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $18. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 18) {
    illegal_1();
    op_18();
}
// Add DE to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(fd, 19) {
    ADD16(IY, DE);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 1a) {
    illegal_1();
    op_1a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 1b) {
    illegal_1();
    op_1b();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 1c) {
    illegal_1();
    op_1c();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 1d) {
    illegal_1();
    op_1d();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 1e) {
    illegal_1();
    op_1e();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 1f) {
    illegal_1();
    op_1f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $20. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 20) {
    illegal_1();
    op_20();
}
// Copy the next immediate word to IY. (UM0050 pp. 222-224).
OP(fd, 21) {
    iy16 = ARG16();
}
// Copy IY to memory at the encoded address. (UM0050 pp. 222-224).
OP(fd, 22) {
    ea = ARG16();
    WM16(ea, &IY);
}
// Increment IY with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
OP(fd, 23) {
    iy16++;
}
// Retained Z80 helper: Increment the high byte of IY with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 24) {
    iyh = INC(iyh);
}
// Retained Z80 helper: Decrement the high byte of IY with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 25) {
    iyh = DEC(iyh);
}
// Retained Z80 helper: Copy the next immediate byte to the high byte of IY. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 26) {
    iyh = ARG();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $27. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 27) {
    illegal_1();
    op_27();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $28. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 28) {
    illegal_1();
    op_28();
}
// Add IY to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(fd, 29) {
    ADD16(IY, IY);
}
// Copy memory at the encoded address to IY. (UM0050 pp. 222-224).
OP(fd, 2a) {
    ea = ARG16();
    RM16(ea, &IY);
}
// Decrement IY with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
OP(fd, 2b) {
    iy16--;
}
// Retained Z80 helper: Increment the low byte of IY with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 2c) {
    iyl = INC(iyl);
}
// Retained Z80 helper: Decrement the low byte of IY with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 2d) {
    iyl = DEC(iyl);
}
// Retained Z80 helper: Copy the next immediate byte to the low byte of IY. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 2e) {
    iyl = ARG();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $2F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 2f) {
    illegal_1();
    op_2f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $30. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 30) {
    illegal_1();
    op_30();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $31. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 31) {
    illegal_1();
    op_31();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $32. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 32) {
    illegal_1();
    op_32();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $33. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 33) {
    illegal_1();
    op_33();
}
// Increment memory at IY plus the signed displacement with 8-bit wrap; update arithmetic flags
// and preserve carry. (UM0050 p. 213).
OP(fd, 34) {
    EAY();
    WM(ea, INC(RM(ea)));
}
// Decrement memory at IY plus the signed displacement with 8-bit wrap; update arithmetic flags
// and preserve carry. (UM0050 p. 213).
OP(fd, 35) {
    EAY();
    WM(ea, DEC(RM(ea)));
}
// Copy the next immediate byte to memory at IY plus the signed displacement. (UM0050 pp.
// 222-224).
OP(fd, 36) {
    EAY();
    WM(ea, ARG());
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $37. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 37) {
    illegal_1();
    op_37();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $38. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 38) {
    illegal_1();
    op_38();
}
// Add SP to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(fd, 39) {
    ADD16(IY, SP);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 3a) {
    illegal_1();
    op_3a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 3b) {
    illegal_1();
    op_3b();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 3c) {
    illegal_1();
    op_3c();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 3d) {
    illegal_1();
    op_3d();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 3e) {
    illegal_1();
    op_3e();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 3f) {
    illegal_1();
    op_3f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $40. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 40) {
    illegal_1();
    op_40();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $41. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 41) {
    illegal_1();
    op_41();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $42. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 42) {
    illegal_1();
    op_42();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $43. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 43) {
    illegal_1();
    op_43();
}
// Retained Z80 helper: Copy the high byte of IY to B. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 44) {
    b8 = iyh;
}
// Retained Z80 helper: Copy the low byte of IY to B. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 45) {
    b8 = iyl;
}
// Copy memory at IY plus the signed displacement to B. (UM0050 pp. 222-224).
OP(fd, 46) {
    EAY();
    b8 = RM(ea);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $47. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 47) {
    illegal_1();
    op_47();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $48. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 48) {
    illegal_1();
    op_48();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $49. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 49) {
    illegal_1();
    op_49();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 4a) {
    illegal_1();
    op_4a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 4b) {
    illegal_1();
    op_4b();
}
// Retained Z80 helper: Copy the high byte of IY to C. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 4c) {
    c8 = iyh;
}
// Retained Z80 helper: Copy the low byte of IY to C. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 4d) {
    c8 = iyl;
}
// Copy memory at IY plus the signed displacement to C. (UM0050 pp. 222-224).
OP(fd, 4e) {
    EAY();
    c8 = RM(ea);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 4f) {
    illegal_1();
    op_4f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $50. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 50) {
    illegal_1();
    op_50();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $51. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 51) {
    illegal_1();
    op_51();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $52. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 52) {
    illegal_1();
    op_52();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $53. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 53) {
    illegal_1();
    op_53();
}
// Retained Z80 helper: Copy the high byte of IY to D. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 54) {
    d8 = iyh;
}
// Retained Z80 helper: Copy the low byte of IY to D. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 55) {
    d8 = iyl;
}
// Copy memory at IY plus the signed displacement to D. (UM0050 pp. 222-224).
OP(fd, 56) {
    EAY();
    d8 = RM(ea);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $57. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 57) {
    illegal_1();
    op_57();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $58. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 58) {
    illegal_1();
    op_58();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $59. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 59) {
    illegal_1();
    op_59();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 5a) {
    illegal_1();
    op_5a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 5b) {
    illegal_1();
    op_5b();
}
// Retained Z80 helper: Copy the high byte of IY to E. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 5c) {
    e8 = iyh;
}
// Retained Z80 helper: Copy the low byte of IY to E. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 5d) {
    e8 = iyl;
}
// Copy memory at IY plus the signed displacement to E. (UM0050 pp. 222-224).
OP(fd, 5e) {
    EAY();
    e8 = RM(ea);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 5f) {
    illegal_1();
    op_5f();
}

// Retained Z80 helper: Copy B to the high byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 60) {
    iyh = b8;
}
// Retained Z80 helper: Copy C to the high byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 61) {
    iyh = c8;
}
// Retained Z80 helper: Copy D to the high byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 62) {
    iyh = d8;
}
// Retained Z80 helper: Copy E to the high byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 63) {
    iyh = e8;
}
// Retained Z80 helper: Leave the high byte of IY unchanged; this self-load still consumes
// instruction time. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 64) {}
// Retained Z80 helper: Copy the low byte of IY to the high byte of IY. The Z180 decoder traps
// this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 65) {
    iyh = iyl;
}
// Copy memory at IY plus the signed displacement to H. (UM0050 pp. 222-224).
OP(fd, 66) {
    EAY();
    h8 = RM(ea);
}
// Retained Z80 helper: Copy A to the high byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 67) {
    iyh = a8;
}

// Retained Z80 helper: Copy B to the low byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 68) {
    iyl = b8;
}
// Retained Z80 helper: Copy C to the low byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 69) {
    iyl = c8;
}
// Retained Z80 helper: Copy D to the low byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 6a) {
    iyl = d8;
}
// Retained Z80 helper: Copy E to the low byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 6b) {
    iyl = e8;
}
// Retained Z80 helper: Copy the high byte of IY to the low byte of IY. The Z180 decoder traps
// this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 6c) {
    iyl = iyh;
}
// Retained Z80 helper: Leave the low byte of IY unchanged; this self-load still consumes
// instruction time. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 6d) {}
// Copy memory at IY plus the signed displacement to L. (UM0050 pp. 222-224).
OP(fd, 6e) {
    EAY();
    l8 = RM(ea);
}
// Retained Z80 helper: Copy A to the low byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 6f) {
    iyl = a8;
}

// Copy B to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
OP(fd, 70) {
    EAY();
    WM(ea, b8);
}
// Copy C to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
OP(fd, 71) {
    EAY();
    WM(ea, c8);
}
// Copy D to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
OP(fd, 72) {
    EAY();
    WM(ea, d8);
}
// Copy E to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
OP(fd, 73) {
    EAY();
    WM(ea, e8);
}
// Copy H to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
OP(fd, 74) {
    EAY();
    WM(ea, h8);
}
// Copy L to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
OP(fd, 75) {
    EAY();
    WM(ea, l8);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $76. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 76) {
    illegal_1();
    op_76();
}
// Copy A to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
OP(fd, 77) {
    EAY();
    WM(ea, a8);
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $78. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 78) {
    illegal_1();
    op_78();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $79. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 79) {
    illegal_1();
    op_79();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 7a) {
    illegal_1();
    op_7a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 7b) {
    illegal_1();
    op_7b();
}
// Retained Z80 helper: Copy the high byte of IY to A. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 7c) {
    a8 = iyh;
}
// Retained Z80 helper: Copy the low byte of IY to A. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
OP(fd, 7d) {
    a8 = iyl;
}
// Copy memory at IY plus the signed displacement to A. (UM0050 pp. 222-224).
OP(fd, 7e) {
    EAY();
    a8 = RM(ea);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 7f) {
    illegal_1();
    op_7f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $80. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 80) {
    illegal_1();
    op_80();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $81. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 81) {
    illegal_1();
    op_81();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $82. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 82) {
    illegal_1();
    op_82();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $83. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 83) {
    illegal_1();
    op_83();
}
// Retained Z80 helper: Add the high byte of IY to A. Store the wrapped result and update 8-bit
// arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 84) {
    ADD(iyh);
}
// Retained Z80 helper: Add the low byte of IY to A. Store the wrapped result and update 8-bit
// arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 85) {
    ADD(iyl);
}
// Add memory at IY plus the signed displacement to A. Store the wrapped result and update 8-bit
// arithmetic flags. (UM0050 pp. 211-212).
OP(fd, 86) {
    EAY();
    ADD(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $87. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 87) {
    illegal_1();
    op_87();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $88. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 88) {
    illegal_1();
    op_88();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $89. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 89) {
    illegal_1();
    op_89();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 8a) {
    illegal_1();
    op_8a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 8b) {
    illegal_1();
    op_8b();
}
// Retained Z80 helper: Add the high byte of IY and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(fd, 8c) {
    ADC(iyh);
}
// Retained Z80 helper: Add the low byte of IY and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(fd, 8d) {
    ADC(iyl);
}
// Add memory at IY plus the signed displacement and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. (UM0050 pp. 211-212).
OP(fd, 8e) {
    EAY();
    ADC(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 8f) {
    illegal_1();
    op_8f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $90. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 90) {
    illegal_1();
    op_90();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $91. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 91) {
    illegal_1();
    op_91();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $92. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 92) {
    illegal_1();
    op_92();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $93. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 93) {
    illegal_1();
    op_93();
}
// Retained Z80 helper: Subtract the high byte of IY from A. Store the wrapped result and update
// 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 94) {
    SUB(iyh);
}
// Retained Z80 helper: Subtract the low byte of IY from A. Store the wrapped result and update
// 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, 95) {
    SUB(iyl);
}
// Subtract memory at IY plus the signed displacement from A. Store the wrapped result and
// update 8-bit arithmetic flags. (UM0050 pp. 211-212).
OP(fd, 96) {
    EAY();
    SUB(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $97. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 97) {
    illegal_1();
    op_97();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $98. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 98) {
    illegal_1();
    op_98();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $99. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 99) {
    illegal_1();
    op_99();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 9a) {
    illegal_1();
    op_9a();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 9b) {
    illegal_1();
    op_9b();
}
// Retained Z80 helper: Subtract the high byte of IY and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(fd, 9c) {
    SBC(iyh);
}
// Retained Z80 helper: Subtract the low byte of IY and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(fd, 9d) {
    SBC(iyl);
}
// Subtract memory at IY plus the signed displacement and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
OP(fd, 9e) {
    EAY();
    SBC(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, 9f) {
    illegal_1();
    op_9f();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, a0) {
    illegal_1();
    op_a0();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, a1) {
    illegal_1();
    op_a1();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, a2) {
    illegal_1();
    op_a2();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, a3) {
    illegal_1();
    op_a3();
}
// Retained Z80 helper: Combine A with the high byte of IY using bitwise AND. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, a4) {
    AND(iyh);
}
// Retained Z80 helper: Combine A with the low byte of IY using bitwise AND. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, a5) {
    AND(iyl);
}
// Combine A with memory at IY plus the signed displacement using bitwise AND. Store A and set
// logical-result flags. (UM0050 p. 214).
OP(fd, a6) {
    EAY();
    AND(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, a7) {
    illegal_1();
    op_a7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, a8) {
    illegal_1();
    op_a8();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, a9) {
    illegal_1();
    op_a9();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, aa) {
    illegal_1();
    op_aa();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, ab) {
    illegal_1();
    op_ab();
}
// Retained Z80 helper: Combine A with the high byte of IY using bitwise exclusive OR. Store A
// and set logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(fd, ac) {
    XOR(iyh);
}
// Retained Z80 helper: Combine A with the low byte of IY using bitwise exclusive OR. Store A
// and set logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
OP(fd, ad) {
    XOR(iyl);
}
// Combine A with memory at IY plus the signed displacement using bitwise exclusive OR. Store A
// and set logical-result flags. (UM0050 p. 214).
OP(fd, ae) {
    EAY();
    XOR(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, af) {
    illegal_1();
    op_af();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, b0) {
    illegal_1();
    op_b0();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, b1) {
    illegal_1();
    op_b1();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, b2) {
    illegal_1();
    op_b2();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, b3) {
    illegal_1();
    op_b3();
}
// Retained Z80 helper: Combine A with the high byte of IY using bitwise OR. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, b4) {
    OR(iyh);
}
// Retained Z80 helper: Combine A with the low byte of IY using bitwise OR. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, b5) {
    OR(iyl);
}
// Combine A with memory at IY plus the signed displacement using bitwise OR. Store A and set
// logical-result flags. (UM0050 p. 214).
OP(fd, b6) {
    EAY();
    OR(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, b7) {
    illegal_1();
    op_b7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, b8) {
    illegal_1();
    op_b8();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, b9) {
    illegal_1();
    op_b9();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, ba) {
    illegal_1();
    op_ba();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, bb) {
    illegal_1();
    op_bb();
}
// Retained Z80 helper: Compare A with the high byte of IY by setting subtraction flags; leave A
// unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, bc) {
    CP(iyh);
}
// Retained Z80 helper: Compare A with the low byte of IY by setting subtraction flags; leave A
// unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
OP(fd, bd) {
    CP(iyl);
}
// Compare A with memory at IY plus the signed displacement by setting subtraction flags; leave
// A unchanged. (UM0050 p. 213).
OP(fd, be) {
    EAY();
    CP(RM(ea));
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, bf) {
    illegal_1();
    op_bf();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, c0) {
    illegal_1();
    op_c0();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, c1) {
    illegal_1();
    op_c1();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, c2) {
    illegal_1();
    op_c2();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, c3) {
    illegal_1();
    op_c3();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, c4) {
    illegal_1();
    op_c4();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, c5) {
    illegal_1();
    op_c5();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, c6) {
    illegal_1();
    op_c6();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, c7) {
    illegal_1();
    op_c7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, c8) {
    illegal_1();
    op_c8();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, c9) {
    illegal_1();
    op_c9();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, ca) {
    illegal_1();
    op_ca();
}
// Read the signed index displacement, save its effective address, then fetch and dispatch the
// CB bit-operation byte (UM0050 pp. 182, 249, 267-271).
OP(fd, cb) {
    EAY();
    extra_cycles += exec_xycb(ROP());
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, cc) {
    illegal_1();
    op_cc();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, cd) {
    illegal_1();
    op_cd();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, ce) {
    illegal_1();
    op_ce();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, cf) {
    illegal_1();
    op_cf();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, d0) {
    illegal_1();
    op_d0();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, d1) {
    illegal_1();
    op_d1();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, d2) {
    illegal_1();
    op_d2();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, d3) {
    illegal_1();
    op_d3();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, d4) {
    illegal_1();
    op_d4();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, d5) {
    illegal_1();
    op_d5();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, d6) {
    illegal_1();
    op_d6();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, d7) {
    illegal_1();
    op_d7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, d8) {
    illegal_1();
    op_d8();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, d9) {
    illegal_1();
    op_d9();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, da) {
    illegal_1();
    op_da();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, db) {
    illegal_1();
    op_db();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, dc) {
    illegal_1();
    op_dc();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, dd) {
    illegal_1();
    op_dd();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, de) {
    illegal_1();
    op_de();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, df) {
    illegal_1();
    op_df();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, e0) {
    illegal_1();
    op_e0();
}
// Read the low then high stack bytes into IY and advance SP. (UM0050 p. 227).
OP(fd, e1) {
    POP(IY);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, e2) {
    illegal_1();
    op_e2();
}
// Exchange the word at SP and IY without changing arithmetic flags. (UM0050 p. 228).
OP(fd, e3) {
    EXSP(IY);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, e4) {
    illegal_1();
    op_e4();
}
// Store IY on the descending stack, high byte first. (UM0050 p. 227).
OP(fd, e5) {
    PUSH(IY);
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, e6) {
    illegal_1();
    op_e6();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, e7) {
    illegal_1();
    op_e7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, e8) {
    illegal_1();
    op_e8();
}
// Set PC to IY (the register value, without a memory read). (UM0050 pp. 229-230).
OP(fd, e9) {
    pc16 = iy16;
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, ea) {
    illegal_1();
    op_ea();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, eb) {
    illegal_1();
    op_eb();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, ec) {
    illegal_1();
    op_ec();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $ED. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, ed) {
    illegal_1();
    op_ed();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, ee) {
    illegal_1();
    op_ee();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, ef) {
    illegal_1();
    op_ef();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, f0) {
    illegal_1();
    op_f0();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, f1) {
    illegal_1();
    op_f1();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, f2) {
    illegal_1();
    op_f2();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, f3) {
    illegal_1();
    op_f3();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, f4) {
    illegal_1();
    op_f4();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, f5) {
    illegal_1();
    op_f5();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, f6) {
    illegal_1();
    op_f6();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, f7) {
    illegal_1();
    op_f7();
}

// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, f8) {
    illegal_1();
    op_f8();
}
// Copy IY to SP. (UM0050 pp. 222-224).
OP(fd, f9) {
    sp16 = iy16;
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, fa) {
    illegal_1();
    op_fa();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, fb) {
    illegal_1();
    op_fb();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, fc) {
    illegal_1();
    op_fc();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, fd) {
    illegal_1();
    op_fd();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, fe) {
    illegal_1();
    op_fe();
}
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
OP(fd, ff) {
    illegal_1();
    op_ff();
}
