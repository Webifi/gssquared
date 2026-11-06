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
 *   entry to address zero for an undefined instruction. Z80 helpers for
 *   unsupported encodings remain here, but the Z180 decoder
 *   rejects them before execution.
 *
 *   Sources: Zilog UM005004-0918 pp. 68-72 for traps, 173-183 for registers,
 *   pp. 211-236 for operations, pp. 247-250 for valid encodings, and
 *   pp. 251-280 for bus ordering. Page citations below use printed pages.
 */

/**********************************************************
 * IY register related opcodes (FD prefix)
 **********************************************************/
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $00.
OP(fd, 00) {
    illegal_1();
    op_00();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $01.
OP(fd, 01) {
    illegal_1();
    op_01();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $02.
OP(fd, 02) {
    illegal_1();
    op_02();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $03.
OP(fd, 03) {
    illegal_1();
    op_03();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $04.
OP(fd, 04) {
    illegal_1();
    op_04();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $05.
OP(fd, 05) {
    illegal_1();
    op_05();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $06.
OP(fd, 06) {
    illegal_1();
    op_06();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $07.
OP(fd, 07) {
    illegal_1();
    op_07();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $08.
OP(fd, 08) {
    illegal_1();
    op_08();
}
// Add BC to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(fd, 09) {
    ADD16(IY, BC);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0A.
OP(fd, 0a) {
    illegal_1();
    op_0a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0B.
OP(fd, 0b) {
    illegal_1();
    op_0b();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0C.
OP(fd, 0c) {
    illegal_1();
    op_0c();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0D.
OP(fd, 0d) {
    illegal_1();
    op_0d();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0E.
OP(fd, 0e) {
    illegal_1();
    op_0e();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0F.
OP(fd, 0f) {
    illegal_1();
    op_0f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $10.
OP(fd, 10) {
    illegal_1();
    op_10();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $11.
OP(fd, 11) {
    illegal_1();
    op_11();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $12.
OP(fd, 12) {
    illegal_1();
    op_12();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $13.
OP(fd, 13) {
    illegal_1();
    op_13();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $14.
OP(fd, 14) {
    illegal_1();
    op_14();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $15.
OP(fd, 15) {
    illegal_1();
    op_15();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $16.
OP(fd, 16) {
    illegal_1();
    op_16();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $17.
OP(fd, 17) {
    illegal_1();
    op_17();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $18.
OP(fd, 18) {
    illegal_1();
    op_18();
}
// Add DE to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(fd, 19) {
    ADD16(IY, DE);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1A.
OP(fd, 1a) {
    illegal_1();
    op_1a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1B.
OP(fd, 1b) {
    illegal_1();
    op_1b();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1C.
OP(fd, 1c) {
    illegal_1();
    op_1c();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1D.
OP(fd, 1d) {
    illegal_1();
    op_1d();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1E.
OP(fd, 1e) {
    illegal_1();
    op_1e();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1F.
OP(fd, 1f) {
    illegal_1();
    op_1f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $20.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $27.
OP(fd, 27) {
    illegal_1();
    op_27();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $28.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $2F.
OP(fd, 2f) {
    illegal_1();
    op_2f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $30.
OP(fd, 30) {
    illegal_1();
    op_30();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $31.
OP(fd, 31) {
    illegal_1();
    op_31();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $32.
OP(fd, 32) {
    illegal_1();
    op_32();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $33.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $37.
OP(fd, 37) {
    illegal_1();
    op_37();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $38.
OP(fd, 38) {
    illegal_1();
    op_38();
}
// Add SP to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(fd, 39) {
    ADD16(IY, SP);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3A.
OP(fd, 3a) {
    illegal_1();
    op_3a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3B.
OP(fd, 3b) {
    illegal_1();
    op_3b();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3C.
OP(fd, 3c) {
    illegal_1();
    op_3c();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3D.
OP(fd, 3d) {
    illegal_1();
    op_3d();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3E.
OP(fd, 3e) {
    illegal_1();
    op_3e();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3F.
OP(fd, 3f) {
    illegal_1();
    op_3f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $40.
OP(fd, 40) {
    illegal_1();
    op_40();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $41.
OP(fd, 41) {
    illegal_1();
    op_41();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $42.
OP(fd, 42) {
    illegal_1();
    op_42();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $43.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $47.
OP(fd, 47) {
    illegal_1();
    op_47();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $48.
OP(fd, 48) {
    illegal_1();
    op_48();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $49.
OP(fd, 49) {
    illegal_1();
    op_49();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $4A.
OP(fd, 4a) {
    illegal_1();
    op_4a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $4B.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $4F.
OP(fd, 4f) {
    illegal_1();
    op_4f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $50.
OP(fd, 50) {
    illegal_1();
    op_50();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $51.
OP(fd, 51) {
    illegal_1();
    op_51();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $52.
OP(fd, 52) {
    illegal_1();
    op_52();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $53.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $57.
OP(fd, 57) {
    illegal_1();
    op_57();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $58.
OP(fd, 58) {
    illegal_1();
    op_58();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $59.
OP(fd, 59) {
    illegal_1();
    op_59();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $5A.
OP(fd, 5a) {
    illegal_1();
    op_5a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $5B.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $5F.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $76.
OP(fd, 76) {
    illegal_1();
    op_76();
}
// Copy A to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
OP(fd, 77) {
    EAY();
    WM(ea, a8);
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $78.
OP(fd, 78) {
    illegal_1();
    op_78();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $79.
OP(fd, 79) {
    illegal_1();
    op_79();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $7A.
OP(fd, 7a) {
    illegal_1();
    op_7a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $7B.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $7F.
OP(fd, 7f) {
    illegal_1();
    op_7f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $80.
OP(fd, 80) {
    illegal_1();
    op_80();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $81.
OP(fd, 81) {
    illegal_1();
    op_81();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $82.
OP(fd, 82) {
    illegal_1();
    op_82();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $83.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $87.
OP(fd, 87) {
    illegal_1();
    op_87();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $88.
OP(fd, 88) {
    illegal_1();
    op_88();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $89.
OP(fd, 89) {
    illegal_1();
    op_89();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $8A.
OP(fd, 8a) {
    illegal_1();
    op_8a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $8B.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $8F.
OP(fd, 8f) {
    illegal_1();
    op_8f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $90.
OP(fd, 90) {
    illegal_1();
    op_90();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $91.
OP(fd, 91) {
    illegal_1();
    op_91();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $92.
OP(fd, 92) {
    illegal_1();
    op_92();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $93.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $97.
OP(fd, 97) {
    illegal_1();
    op_97();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $98.
OP(fd, 98) {
    illegal_1();
    op_98();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $99.
OP(fd, 99) {
    illegal_1();
    op_99();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $9A.
OP(fd, 9a) {
    illegal_1();
    op_9a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $9B.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $9F.
OP(fd, 9f) {
    illegal_1();
    op_9f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A0.
OP(fd, a0) {
    illegal_1();
    op_a0();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A1.
OP(fd, a1) {
    illegal_1();
    op_a1();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A2.
OP(fd, a2) {
    illegal_1();
    op_a2();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A3.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A7.
OP(fd, a7) {
    illegal_1();
    op_a7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A8.
OP(fd, a8) {
    illegal_1();
    op_a8();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A9.
OP(fd, a9) {
    illegal_1();
    op_a9();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $AA.
OP(fd, aa) {
    illegal_1();
    op_aa();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $AB.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $AF.
OP(fd, af) {
    illegal_1();
    op_af();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B0.
OP(fd, b0) {
    illegal_1();
    op_b0();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B1.
OP(fd, b1) {
    illegal_1();
    op_b1();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B2.
OP(fd, b2) {
    illegal_1();
    op_b2();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B3.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B7.
OP(fd, b7) {
    illegal_1();
    op_b7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B8.
OP(fd, b8) {
    illegal_1();
    op_b8();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B9.
OP(fd, b9) {
    illegal_1();
    op_b9();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $BA.
OP(fd, ba) {
    illegal_1();
    op_ba();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $BB.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $BF.
OP(fd, bf) {
    illegal_1();
    op_bf();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C0.
OP(fd, c0) {
    illegal_1();
    op_c0();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C1.
OP(fd, c1) {
    illegal_1();
    op_c1();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C2.
OP(fd, c2) {
    illegal_1();
    op_c2();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C3.
OP(fd, c3) {
    illegal_1();
    op_c3();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C4.
OP(fd, c4) {
    illegal_1();
    op_c4();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C5.
OP(fd, c5) {
    illegal_1();
    op_c5();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C6.
OP(fd, c6) {
    illegal_1();
    op_c6();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C7.
OP(fd, c7) {
    illegal_1();
    op_c7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C8.
OP(fd, c8) {
    illegal_1();
    op_c8();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C9.
OP(fd, c9) {
    illegal_1();
    op_c9();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $CA.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $CC.
OP(fd, cc) {
    illegal_1();
    op_cc();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $CD.
OP(fd, cd) {
    illegal_1();
    op_cd();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $CE.
OP(fd, ce) {
    illegal_1();
    op_ce();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $CF.
OP(fd, cf) {
    illegal_1();
    op_cf();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D0.
OP(fd, d0) {
    illegal_1();
    op_d0();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D1.
OP(fd, d1) {
    illegal_1();
    op_d1();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D2.
OP(fd, d2) {
    illegal_1();
    op_d2();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D3.
OP(fd, d3) {
    illegal_1();
    op_d3();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D4.
OP(fd, d4) {
    illegal_1();
    op_d4();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D5.
OP(fd, d5) {
    illegal_1();
    op_d5();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D6.
OP(fd, d6) {
    illegal_1();
    op_d6();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D7.
OP(fd, d7) {
    illegal_1();
    op_d7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D8.
OP(fd, d8) {
    illegal_1();
    op_d8();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D9.
OP(fd, d9) {
    illegal_1();
    op_d9();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DA.
OP(fd, da) {
    illegal_1();
    op_da();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DB.
OP(fd, db) {
    illegal_1();
    op_db();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DC.
OP(fd, dc) {
    illegal_1();
    op_dc();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DD.
OP(fd, dd) {
    illegal_1();
    op_dd();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DE.
OP(fd, de) {
    illegal_1();
    op_de();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DF.
OP(fd, df) {
    illegal_1();
    op_df();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E0.
OP(fd, e0) {
    illegal_1();
    op_e0();
}
// Read the low then high stack bytes into IY and advance SP. (UM0050 p. 227).
OP(fd, e1) {
    POP(IY);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E2.
OP(fd, e2) {
    illegal_1();
    op_e2();
}
// Exchange the word at SP and IY without changing arithmetic flags. (UM0050 p. 228).
OP(fd, e3) {
    EXSP(IY);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E4.
OP(fd, e4) {
    illegal_1();
    op_e4();
}
// Store IY on the descending stack, high byte first. (UM0050 p. 227).
OP(fd, e5) {
    PUSH(IY);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E6.
OP(fd, e6) {
    illegal_1();
    op_e6();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E7.
OP(fd, e7) {
    illegal_1();
    op_e7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E8.
OP(fd, e8) {
    illegal_1();
    op_e8();
}
// Set PC to IY (the register value, without a memory read). (UM0050 pp. 229-230).
OP(fd, e9) {
    pc16 = iy16;
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $EA.
OP(fd, ea) {
    illegal_1();
    op_ea();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $EB.
OP(fd, eb) {
    illegal_1();
    op_eb();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $EC.
OP(fd, ec) {
    illegal_1();
    op_ec();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $ED.
OP(fd, ed) {
    illegal_1();
    op_ed();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $EE.
OP(fd, ee) {
    illegal_1();
    op_ee();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $EF.
OP(fd, ef) {
    illegal_1();
    op_ef();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F0.
OP(fd, f0) {
    illegal_1();
    op_f0();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F1.
OP(fd, f1) {
    illegal_1();
    op_f1();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F2.
OP(fd, f2) {
    illegal_1();
    op_f2();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F3.
OP(fd, f3) {
    illegal_1();
    op_f3();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F4.
OP(fd, f4) {
    illegal_1();
    op_f4();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F5.
OP(fd, f5) {
    illegal_1();
    op_f5();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F6.
OP(fd, f6) {
    illegal_1();
    op_f6();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F7.
OP(fd, f7) {
    illegal_1();
    op_f7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F8.
OP(fd, f8) {
    illegal_1();
    op_f8();
}
// Copy IY to SP. (UM0050 pp. 222-224).
OP(fd, f9) {
    sp16 = iy16;
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FA.
OP(fd, fa) {
    illegal_1();
    op_fa();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FB.
OP(fd, fb) {
    illegal_1();
    op_fb();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FC.
OP(fd, fc) {
    illegal_1();
    op_fc();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FD.
OP(fd, fd) {
    illegal_1();
    op_fd();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FE.
OP(fd, fe) {
    illegal_1();
    op_fe();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FF.
OP(fd, ff) {
    illegal_1();
    op_ff();
}
