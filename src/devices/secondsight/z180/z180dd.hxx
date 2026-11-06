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
 *   entry to address zero for an undefined instruction. Z80 helpers for
 *   unsupported encodings remain here, but the Z180 decoder
 *   rejects them before execution.
 *
 *   Sources: Zilog UM005004-0918 pp. 68-72 for traps, 173-183 for registers,
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $00.
OP(dd, 00) {
    illegal_1();
    op_00();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $01.
OP(dd, 01) {
    illegal_1();
    op_01();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $02.
OP(dd, 02) {
    illegal_1();
    op_02();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $03.
OP(dd, 03) {
    illegal_1();
    op_03();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $04.
OP(dd, 04) {
    illegal_1();
    op_04();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $05.
OP(dd, 05) {
    illegal_1();
    op_05();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $06.
OP(dd, 06) {
    illegal_1();
    op_06();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $07.
OP(dd, 07) {
    illegal_1();
    op_07();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $08.
OP(dd, 08) {
    illegal_1();
    op_08();
}
// Add BC to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(dd, 09) {
    ADD16(IX, BC);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0A.
OP(dd, 0a) {
    illegal_1();
    op_0a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0B.
OP(dd, 0b) {
    illegal_1();
    op_0b();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0C.
OP(dd, 0c) {
    illegal_1();
    op_0c();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0D.
OP(dd, 0d) {
    illegal_1();
    op_0d();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0E.
OP(dd, 0e) {
    illegal_1();
    op_0e();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $0F.
OP(dd, 0f) {
    illegal_1();
    op_0f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $10.
OP(dd, 10) {
    illegal_1();
    op_10();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $11.
OP(dd, 11) {
    illegal_1();
    op_11();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $12.
OP(dd, 12) {
    illegal_1();
    op_12();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $13.
OP(dd, 13) {
    illegal_1();
    op_13();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $14.
OP(dd, 14) {
    illegal_1();
    op_14();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $15.
OP(dd, 15) {
    illegal_1();
    op_15();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $16.
OP(dd, 16) {
    illegal_1();
    op_16();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $17.
OP(dd, 17) {
    illegal_1();
    op_17();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $18.
OP(dd, 18) {
    illegal_1();
    op_18();
}
// Add DE to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(dd, 19) {
    ADD16(IX, DE);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1A.
OP(dd, 1a) {
    illegal_1();
    op_1a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1B.
OP(dd, 1b) {
    illegal_1();
    op_1b();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1C.
OP(dd, 1c) {
    illegal_1();
    op_1c();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1D.
OP(dd, 1d) {
    illegal_1();
    op_1d();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1E.
OP(dd, 1e) {
    illegal_1();
    op_1e();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $1F.
OP(dd, 1f) {
    illegal_1();
    op_1f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $20.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $27.
OP(dd, 27) {
    illegal_1();
    op_27();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $28.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $2F.
OP(dd, 2f) {
    illegal_1();
    op_2f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $30.
OP(dd, 30) {
    illegal_1();
    op_30();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $31.
OP(dd, 31) {
    illegal_1();
    op_31();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $32.
OP(dd, 32) {
    illegal_1();
    op_32();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $33.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $37.
OP(dd, 37) {
    illegal_1();
    op_37();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $38.
OP(dd, 38) {
    illegal_1();
    op_38();
}
// Add SP to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
OP(dd, 39) {
    ADD16(IX, SP);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3A.
OP(dd, 3a) {
    illegal_1();
    op_3a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3B.
OP(dd, 3b) {
    illegal_1();
    op_3b();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3C.
OP(dd, 3c) {
    illegal_1();
    op_3c();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3D.
OP(dd, 3d) {
    illegal_1();
    op_3d();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3E.
OP(dd, 3e) {
    illegal_1();
    op_3e();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $3F.
OP(dd, 3f) {
    illegal_1();
    op_3f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $40.
OP(dd, 40) {
    illegal_1();
    op_40();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $41.
OP(dd, 41) {
    illegal_1();
    op_41();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $42.
OP(dd, 42) {
    illegal_1();
    op_42();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $43.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $47.
OP(dd, 47) {
    illegal_1();
    op_47();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $48.
OP(dd, 48) {
    illegal_1();
    op_48();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $49.
OP(dd, 49) {
    illegal_1();
    op_49();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $4A.
OP(dd, 4a) {
    illegal_1();
    op_4a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $4B.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $4F.
OP(dd, 4f) {
    illegal_1();
    op_4f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $50.
OP(dd, 50) {
    illegal_1();
    op_50();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $51.
OP(dd, 51) {
    illegal_1();
    op_51();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $52.
OP(dd, 52) {
    illegal_1();
    op_52();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $53.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $57.
OP(dd, 57) {
    illegal_1();
    op_57();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $58.
OP(dd, 58) {
    illegal_1();
    op_58();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $59.
OP(dd, 59) {
    illegal_1();
    op_59();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $5A.
OP(dd, 5a) {
    illegal_1();
    op_5a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $5B.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $5F.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $76.
OP(dd, 76) {
    illegal_1();
    op_76();
}
// Copy A to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
OP(dd, 77) {
    EAX();
    WM(ea, a8);
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $78.
OP(dd, 78) {
    illegal_1();
    op_78();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $79.
OP(dd, 79) {
    illegal_1();
    op_79();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $7A.
OP(dd, 7a) {
    illegal_1();
    op_7a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $7B.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $7F.
OP(dd, 7f) {
    illegal_1();
    op_7f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $80.
OP(dd, 80) {
    illegal_1();
    op_80();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $81.
OP(dd, 81) {
    illegal_1();
    op_81();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $82.
OP(dd, 82) {
    illegal_1();
    op_82();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $83.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $87.
OP(dd, 87) {
    illegal_1();
    op_87();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $88.
OP(dd, 88) {
    illegal_1();
    op_88();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $89.
OP(dd, 89) {
    illegal_1();
    op_89();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $8A.
OP(dd, 8a) {
    illegal_1();
    op_8a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $8B.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $8F.
OP(dd, 8f) {
    illegal_1();
    op_8f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $90.
OP(dd, 90) {
    illegal_1();
    op_90();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $91.
OP(dd, 91) {
    illegal_1();
    op_91();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $92.
OP(dd, 92) {
    illegal_1();
    op_92();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $93.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $97.
OP(dd, 97) {
    illegal_1();
    op_97();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $98.
OP(dd, 98) {
    illegal_1();
    op_98();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $99.
OP(dd, 99) {
    illegal_1();
    op_99();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $9A.
OP(dd, 9a) {
    illegal_1();
    op_9a();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $9B.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $9F.
OP(dd, 9f) {
    illegal_1();
    op_9f();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A0.
OP(dd, a0) {
    illegal_1();
    op_a0();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A1.
OP(dd, a1) {
    illegal_1();
    op_a1();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A2.
OP(dd, a2) {
    illegal_1();
    op_a2();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A3.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A7.
OP(dd, a7) {
    illegal_1();
    op_a7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A8.
OP(dd, a8) {
    illegal_1();
    op_a8();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $A9.
OP(dd, a9) {
    illegal_1();
    op_a9();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $AA.
OP(dd, aa) {
    illegal_1();
    op_aa();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $AB.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $AF.
OP(dd, af) {
    illegal_1();
    op_af();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B0.
OP(dd, b0) {
    illegal_1();
    op_b0();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B1.
OP(dd, b1) {
    illegal_1();
    op_b1();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B2.
OP(dd, b2) {
    illegal_1();
    op_b2();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B3.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B7.
OP(dd, b7) {
    illegal_1();
    op_b7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B8.
OP(dd, b8) {
    illegal_1();
    op_b8();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $B9.
OP(dd, b9) {
    illegal_1();
    op_b9();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $BA.
OP(dd, ba) {
    illegal_1();
    op_ba();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $BB.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $BF.
OP(dd, bf) {
    illegal_1();
    op_bf();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C0.
OP(dd, c0) {
    illegal_1();
    op_c0();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C1.
OP(dd, c1) {
    illegal_1();
    op_c1();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C2.
OP(dd, c2) {
    illegal_1();
    op_c2();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C3.
OP(dd, c3) {
    illegal_1();
    op_c3();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C4.
OP(dd, c4) {
    illegal_1();
    op_c4();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C5.
OP(dd, c5) {
    illegal_1();
    op_c5();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C6.
OP(dd, c6) {
    illegal_1();
    op_c6();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C7.
OP(dd, c7) {
    illegal_1();
    op_c7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C8.
OP(dd, c8) {
    illegal_1();
    op_c8();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $C9.
OP(dd, c9) {
    illegal_1();
    op_c9();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $CA.
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
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $CC.
OP(dd, cc) {
    illegal_1();
    op_cc();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $CD.
OP(dd, cd) {
    illegal_1();
    op_cd();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $CE.
OP(dd, ce) {
    illegal_1();
    op_ce();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $CF.
OP(dd, cf) {
    illegal_1();
    op_cf();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D0.
OP(dd, d0) {
    illegal_1();
    op_d0();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D1.
OP(dd, d1) {
    illegal_1();
    op_d1();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D2.
OP(dd, d2) {
    illegal_1();
    op_d2();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D3.
OP(dd, d3) {
    illegal_1();
    op_d3();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D4.
OP(dd, d4) {
    illegal_1();
    op_d4();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D5.
OP(dd, d5) {
    illegal_1();
    op_d5();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D6.
OP(dd, d6) {
    illegal_1();
    op_d6();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D7.
OP(dd, d7) {
    illegal_1();
    op_d7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D8.
OP(dd, d8) {
    illegal_1();
    op_d8();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $D9.
OP(dd, d9) {
    illegal_1();
    op_d9();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DA.
OP(dd, da) {
    illegal_1();
    op_da();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DB.
OP(dd, db) {
    illegal_1();
    op_db();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DC.
OP(dd, dc) {
    illegal_1();
    op_dc();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DD.
OP(dd, dd) {
    illegal_1();
    op_dd();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DE.
OP(dd, de) {
    illegal_1();
    op_de();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $DF.
OP(dd, df) {
    illegal_1();
    op_df();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E0.
OP(dd, e0) {
    illegal_1();
    op_e0();
}
// Read the low then high stack bytes into IX and advance SP. (UM0050 p. 227).
OP(dd, e1) {
    POP(IX);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E2.
OP(dd, e2) {
    illegal_1();
    op_e2();
}
// Exchange the word at SP and IX without changing arithmetic flags. (UM0050 p. 228).
OP(dd, e3) {
    EXSP(IX);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E4.
OP(dd, e4) {
    illegal_1();
    op_e4();
}
// Store IX on the descending stack, high byte first. (UM0050 p. 227).
OP(dd, e5) {
    PUSH(IX);
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E6.
OP(dd, e6) {
    illegal_1();
    op_e6();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E7.
OP(dd, e7) {
    illegal_1();
    op_e7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $E8.
OP(dd, e8) {
    illegal_1();
    op_e8();
}
// Set PC to IX (the register value, without a memory read). (UM0050 pp. 229-230).
OP(dd, e9) {
    pc16 = ix16;
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $EA.
OP(dd, ea) {
    illegal_1();
    op_ea();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $EB.
OP(dd, eb) {
    illegal_1();
    op_eb();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $EC.
OP(dd, ec) {
    illegal_1();
    op_ec();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $ED.
OP(dd, ed) {
    illegal_1();
    op_ed();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $EE.
OP(dd, ee) {
    illegal_1();
    op_ee();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $EF.
OP(dd, ef) {
    illegal_1();
    op_ef();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F0.
OP(dd, f0) {
    illegal_1();
    op_f0();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F1.
OP(dd, f1) {
    illegal_1();
    op_f1();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F2.
OP(dd, f2) {
    illegal_1();
    op_f2();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F3.
OP(dd, f3) {
    illegal_1();
    op_f3();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F4.
OP(dd, f4) {
    illegal_1();
    op_f4();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F5.
OP(dd, f5) {
    illegal_1();
    op_f5();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F6.
OP(dd, f6) {
    illegal_1();
    op_f6();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F7.
OP(dd, f7) {
    illegal_1();
    op_f7();
}

// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $F8.
OP(dd, f8) {
    illegal_1();
    op_f8();
}
// Copy IX to SP. (UM0050 pp. 222-224).
OP(dd, f9) {
    sp16 = ix16;
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FA.
OP(dd, fa) {
    illegal_1();
    op_fa();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FB.
OP(dd, fb) {
    illegal_1();
    op_fb();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FC.
OP(dd, fc) {
    illegal_1();
    op_fc();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FD.
OP(dd, fd) {
    illegal_1();
    op_fd();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FE.
OP(dd, fe) {
    illegal_1();
    op_fe();
}
// Z180 traps this encoding; the Z80 fallback diagnoses it and runs opcode $FF.
OP(dd, ff) {
    illegal_1();
    op_ff();
}
