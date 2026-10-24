// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller

/*
 *   Private instruction entry points for Second Sight's Zilog Z8S180 CPU.
 *   This fragment is included inside Z180. The CPU owns all implicit operands,
 *   and its execution thread alone calls these members. z180ops.hpp supplies
 *   the shared helpers; the opcode .hxx files supply the byte-specific handlers.
 *   Memory/port helpers call the bus scheduler, which calls SSCard at the
 *   proper edge. Handler methods have no parameters or return value: their
 *   inputs and results are CPU registers, flags and emulated memory.
 *
 *   PC is the program counter, SP the stack pointer, A the accumulator and F
 *   the flags. BC/DE/HL are register pairs; IX/IY are address index registers.
 *   I supplies interrupt-vector high bits; R counts opcode fetches. A prime
 *   means an alternate register set. PHI is the CPU clock and one T-state is
 *   one PHI. MMU is the memory management unit translating program addresses.
 *   The declaration suffix is the hexadecimal instruction byte. op has no
 *   prefix; cb is bit operations; ed extensions; dd/fd select IX/IY; xycb
 *   uses a previously computed indexed address. Unsupported Z80 helpers
 *   remain declared but are rejected by the Z180 decoder's trap gate.
 *
 *   NMI is the non-maskable interrupt; INT0 is the first maskable input.
 *   M1 identifies an opcode fetch. IOCR controls internal I/O and CCR the
 *   processor clock. An interrupt-enable flag decides whether a request
 *   may be accepted between instructions.
 *
 *   Sources: Zilog UM005004-0918 pp. 173-183, 211-236, 247-280.
 *   The per-handler descriptions below use printed manual page numbers.
 */

typedef void (Z180::*opcode_func)();
static const opcode_func opcodes[6][0x100];

// Rebuild sixteen 4-KiB translated page bases. CBAR selects the bank/common
// boundaries; BBR/CBR add page offsets above them. Mask to 20 bits, then
// refresh direct-read views (UM0050 pp. 58-64).
void z180_mmu();
// Read and return a byte at logical address addr modulo 64 KiB. Translate
// through the MMU and retain bus waits/sample timing (UM0050 pp. 21-22, 58-64).
inline uint8_t RM(uint32_t addr);
// Write data at logical address addr modulo 64 KiB through the same MMU
// and bus timing path as RM (UM0050 pp. 21-22, 58-64).
inline void write_mem(uint32_t addr, uint8_t data);
// Read one byte from 16-bit port. Decode the internal register window
// before external board I/O; machine-cycle execution retains sampling order
// and waits (UM0050 pp. 23, 42-48, 231). Return the sampled byte.
inline uint8_t IN(uint16_t port);
// Write value to 16-bit port with internal-register decode and port-cycle
// timing. No byte is returned (UM0050 pp. 23, 42-48, 231).
inline void OUT(uint16_t port, uint8_t value);
// Read low then high bytes at logical addr and addr+1 into r. Each read
// is a distinct cycle; address wrap occurs at 64 KiB (UM0050 pp. 258-261).
inline void RM16(uint32_t addr, Z180Register *r);
// Write r low then high bytes to logical addr and addr+1 as distinct
// cycles. Stack pushes use their own high-first order (UM0050 pp. 258-261, 272-273).
inline void WM16(uint32_t addr, Z180Register *r);
// Fetch and return the instruction byte at PC, then advance PC and R.
// Opcode sampling occurs earlier in T3 than ordinary data (UM0050 pp. 19-22, 177).
inline uint8_t ROP();
// Read and return the next instruction operand byte, advancing PC without
// advancing R. Operand reads use memory-data timing (UM0050 pp. 177, 251-278).
inline uint8_t ARG();
// Read two consecutive instruction operands, low byte first, and return
// the 16-bit word. Separate calls preserve cycle ordering and PC wrap.
inline uint32_t ARG16();
// Return value+1 with 8-bit wrap. Set result/half-carry/overflow flags and
// preserve carry; 0x7f to 0x80 is signed overflow (UM0050 p. 213).
inline uint8_t INC(uint8_t value);
// Return value-1 with 8-bit wrap. Set subtract/result/half-borrow/overflow
// flags and preserve carry; 0x80 to 0x7f overflows (UM0050 p. 213).
inline uint8_t DEC(uint8_t value);
// Return value rotated left with bit 7 wrapping into bit 0 and carry.
// Set sign/zero/parity from the result (UM0050 pp. 216-217).
inline uint8_t RLC(uint8_t value);
// Return value rotated right with bit 0 wrapping into bit 7 and carry.
// Set sign/zero/parity from the result (UM0050 pp. 216-217).
inline uint8_t RRC(uint8_t value);
// Return value rotated left through carry; its old bit 7 becomes carry.
// Set sign/zero/parity from the result (UM0050 pp. 216-217).
inline uint8_t RL(uint8_t value);
// Return value rotated right through carry; its old bit 0 becomes carry.
// Set sign/zero/parity from the result (UM0050 pp. 216-217).
inline uint8_t RR(uint8_t value);
// Return value shifted left with zero inserted; bit 7 becomes carry.
// Set sign/zero/parity from the result (UM0050 pp. 218-219).
inline uint8_t SLA(uint8_t value);
// Return value shifted right with its sign bit retained; bit 0 becomes
// carry. Set sign/zero/parity from the result (UM0050 pp. 218-219).
inline uint8_t SRA(uint8_t value);
// Retained Z80 helper: shift value left, insert one and set result flags.
// The Z180 decoder traps this unsupported encoding (UM0050 pp. 68-72, 249).
inline uint8_t SLL(uint8_t value);
// Return value shifted right with zero inserted; bit 0 becomes carry.
// Set sign/zero/parity from the result (UM0050 pp. 218-219).
inline uint8_t SRL(uint8_t value);
// Return value with bit (0-7) cleared; leave flags unchanged (UM0050 p. 220).
inline uint8_t RES(uint8_t bit, uint8_t value);
// Return value with bit (0-7) set; leave flags unchanged (UM0050 p. 220).
inline uint8_t SET(uint8_t bit, uint8_t value);
// Decode opcode_arg in the op family, trap unsupported bytes and return
// nominal whole-PHI timing. Operands/results are the owning CPU state (UM0050 pp. 247-280).
inline int exec_op(const uint8_t opcode_arg);
// Decode opcode_arg in the cb family, trap unsupported bytes and return
// nominal whole-PHI timing. Operands/results are the owning CPU state (UM0050 pp. 247-280).
inline int exec_cb(const uint8_t opcode_arg);
// Decode opcode_arg in the dd family, trap unsupported bytes and return
// nominal whole-PHI timing. Operands/results are the owning CPU state (UM0050 pp. 247-280).
inline int exec_dd(const uint8_t opcode_arg);
// Decode opcode_arg in the ed family, trap unsupported bytes and return
// nominal whole-PHI timing. Operands/results are the owning CPU state (UM0050 pp. 247-280).
inline int exec_ed(const uint8_t opcode_arg);
// Decode opcode_arg in the fd family, trap unsupported bytes and return
// nominal whole-PHI timing. Operands/results are the owning CPU state (UM0050 pp. 247-280).
inline int exec_fd(const uint8_t opcode_arg);
// Decode opcode_arg in the xycb family, trap unsupported bytes and return
// nominal whole-PHI timing. Operands/results are the owning CPU state (UM0050 pp. 247-280).
inline int exec_xycb(const uint8_t opcode_arg);
// Enter undefined-instruction handling at address zero and return its
// nominal PHI count. third selects the indexed third-opcode trap, including
// its operand read and stacked PC offset; interrupt enables stay unchanged
// (UM0050 pp. 68-72, Figs. 32-33).
int take_trap(bool third);
// Read the internal-register offset from a decoded 16-bit port; return its byte.
uint8_t z180_readcontrol(uint32_t port);
// Write data to the internal-register offset of a decoded port.
void z180_writecontrol(uint32_t port, uint8_t data);
// Merge input bits into the CPU pin image; output callbacks own output bits.
void z180_write_iolines(uint32_t data);
// Clock both programmable reload timers once at PHI/20 (UM0050 pp. 157-164).
void clock_timers();
// Advance the free-running divider and reload timers by cycles whole PHI.
void handle_io_timers(int cycles);
#ifdef GS2_SS_TESTING
// Test-only interrupt entry for priority index irq; return whole-PHI entry timing.
int take_interrupt(int irq);
// Test-only non-maskable interrupt entry; return whole-PHI entry timing.
int take_nmi();
// Test-only instruction-engine check for an enabled/requested DMA0 transfer.
bool dma0_ready() const;
// Test-only DMA0 execution within max_cycles whole-PHI budget; return consumed clocks.
int z180_dma0(int max_cycles);
// Test-only DMA1 byte transfer; return its whole-PHI cost.
int z180_dma1();
// Test-only instruction-engine interrupt priority scan; return selected entry timing.
int check_interrupts();
#endif

// Consume instruction time without changing programmer registers. (UM0050 p. 235).
void op_00();
// Copy the next immediate word to BC. (UM0050 pp. 222-224).
void op_01();
// Copy A to memory at BC. (UM0050 pp. 222-224).
void op_02();
// Increment BC with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void op_03();
// Increment B with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_04();
// Decrement B with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_05();
// Copy the next immediate byte to B. (UM0050 pp. 222-224).
void op_06();
// Rotate A left with bit 7 wrapping to bit 0 and carry; preserve sign, zero and parity. (UM0050
// p. 216).
void op_07();
// Exchange A and F with their alternate pair, including the flag bits (UM0050 p. 228).
void op_08();
// Add BC to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void op_09();
// Copy memory at BC to A. (UM0050 pp. 222-224).
void op_0a();
// Decrement BC with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void op_0b();
// Increment C with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_0c();
// Decrement C with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_0d();
// Copy the next immediate byte to C. (UM0050 pp. 222-224).
void op_0e();
// Rotate A right with bit 0 wrapping to bit 7 and carry; preserve sign, zero and parity.
// (UM0050 p. 216).
void op_0f();
// Decrement B; branch by the signed displacement when B remains nonzero. Flags stay unchanged.
// (UM0050 pp. 229-230).
void op_10();
// Copy the next immediate word to DE. (UM0050 pp. 222-224).
void op_11();
// Copy A to memory at DE. (UM0050 pp. 222-224).
void op_12();
// Increment DE with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void op_13();
// Increment D with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_14();
// Decrement D with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_15();
// Copy the next immediate byte to D. (UM0050 pp. 222-224).
void op_16();
// Rotate A left through carry; preserve sign, zero and parity. (UM0050 p. 216).
void op_17();
// Add the signed displacement to PC after reading it. (UM0050 pp. 229-230).
void op_18();
// Add DE to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void op_19();
// Copy memory at DE to A. (UM0050 pp. 222-224).
void op_1a();
// Decrement DE with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void op_1b();
// Increment E with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_1c();
// Decrement E with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_1d();
// Copy the next immediate byte to E. (UM0050 pp. 222-224).
void op_1e();
// Rotate A right through carry; preserve sign, zero and parity. (UM0050 p. 216).
void op_1f();
// Add the signed displacement to PC after reading it if zero is clear. (UM0050 pp. 229-230).
void op_20();
// Copy the next immediate word to HL. (UM0050 pp. 222-224).
void op_21();
// Copy HL to memory at the encoded address. (UM0050 pp. 222-224).
void op_22();
// Increment HL with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void op_23();
// Increment H with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_24();
// Decrement H with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_25();
// Copy the next immediate byte to H. (UM0050 pp. 222-224).
void op_26();
// Correct A after decimal-digit arithmetic using the carry, half-carry and subtract flags.
// (UM0050 p. 215).
void op_27();
// Add the signed displacement to PC after reading it if zero is set. (UM0050 pp. 229-230).
void op_28();
// Add HL to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void op_29();
// Copy memory at the encoded address to HL. (UM0050 pp. 222-224).
void op_2a();
// Decrement HL with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void op_2b();
// Increment L with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_2c();
// Decrement L with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_2d();
// Copy the next immediate byte to L. (UM0050 pp. 222-224).
void op_2e();
// Invert all bits of A; set half-carry/subtract and preserve sign, zero, parity and carry.
// (UM0050 p. 215).
void op_2f();
// Add the signed displacement to PC after reading it if carry is clear. (UM0050 pp. 229-230).
void op_30();
// Copy the next immediate word to SP. (UM0050 pp. 222-224).
void op_31();
// Copy A to memory at the encoded address. (UM0050 pp. 222-224).
void op_32();
// Increment SP with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void op_33();
// Increment memory at HL with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050
// p. 213).
void op_34();
// Decrement memory at HL with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050
// p. 213).
void op_35();
// Copy the next immediate byte to memory at HL. (UM0050 pp. 222-224).
void op_36();
// Set carry and clear half-carry/subtract; preserve sign, zero and parity. (UM0050 p. 235).
void op_37();
// Add the signed displacement to PC after reading it if carry is set. (UM0050 pp. 229-230).
void op_38();
// Add SP to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void op_39();
// Copy memory at the encoded address to A. (UM0050 pp. 222-224).
void op_3a();
// Decrement SP with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void op_3b();
// Increment A with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_3c();
// Decrement A with 8-bit wrap; update arithmetic flags and preserve carry. (UM0050 p. 213).
void op_3d();
// Copy the next immediate byte to A. (UM0050 pp. 222-224).
void op_3e();
// Invert carry and clear half-carry/subtract; preserve sign, zero and parity. (UM0050 p. 235).
void op_3f();
// Leave B unchanged; this self-load still consumes instruction time. (UM0050 pp. 222-224).
void op_40();
// Copy C to B. (UM0050 pp. 222-224).
void op_41();
// Copy D to B. (UM0050 pp. 222-224).
void op_42();
// Copy E to B. (UM0050 pp. 222-224).
void op_43();
// Copy H to B. (UM0050 pp. 222-224).
void op_44();
// Copy L to B. (UM0050 pp. 222-224).
void op_45();
// Copy memory at HL to B. (UM0050 pp. 222-224).
void op_46();
// Copy A to B. (UM0050 pp. 222-224).
void op_47();
// Copy B to C. (UM0050 pp. 222-224).
void op_48();
// Leave C unchanged; this self-load still consumes instruction time. (UM0050 pp. 222-224).
void op_49();
// Copy D to C. (UM0050 pp. 222-224).
void op_4a();
// Copy E to C. (UM0050 pp. 222-224).
void op_4b();
// Copy H to C. (UM0050 pp. 222-224).
void op_4c();
// Copy L to C. (UM0050 pp. 222-224).
void op_4d();
// Copy memory at HL to C. (UM0050 pp. 222-224).
void op_4e();
// Copy A to C. (UM0050 pp. 222-224).
void op_4f();
// Copy B to D. (UM0050 pp. 222-224).
void op_50();
// Copy C to D. (UM0050 pp. 222-224).
void op_51();
// Leave D unchanged; this self-load still consumes instruction time. (UM0050 pp. 222-224).
void op_52();
// Copy E to D. (UM0050 pp. 222-224).
void op_53();
// Copy H to D. (UM0050 pp. 222-224).
void op_54();
// Copy L to D. (UM0050 pp. 222-224).
void op_55();
// Copy memory at HL to D. (UM0050 pp. 222-224).
void op_56();
// Copy A to D. (UM0050 pp. 222-224).
void op_57();
// Copy B to E. (UM0050 pp. 222-224).
void op_58();
// Copy C to E. (UM0050 pp. 222-224).
void op_59();
// Copy D to E. (UM0050 pp. 222-224).
void op_5a();
// Leave E unchanged; this self-load still consumes instruction time. (UM0050 pp. 222-224).
void op_5b();
// Copy H to E. (UM0050 pp. 222-224).
void op_5c();
// Copy L to E. (UM0050 pp. 222-224).
void op_5d();
// Copy memory at HL to E. (UM0050 pp. 222-224).
void op_5e();
// Copy A to E. (UM0050 pp. 222-224).
void op_5f();
// Copy B to H. (UM0050 pp. 222-224).
void op_60();
// Copy C to H. (UM0050 pp. 222-224).
void op_61();
// Copy D to H. (UM0050 pp. 222-224).
void op_62();
// Copy E to H. (UM0050 pp. 222-224).
void op_63();
// Leave H unchanged; this self-load still consumes instruction time. (UM0050 pp. 222-224).
void op_64();
// Copy L to H. (UM0050 pp. 222-224).
void op_65();
// Copy memory at HL to H. (UM0050 pp. 222-224).
void op_66();
// Copy A to H. (UM0050 pp. 222-224).
void op_67();
// Copy B to L. (UM0050 pp. 222-224).
void op_68();
// Copy C to L. (UM0050 pp. 222-224).
void op_69();
// Copy D to L. (UM0050 pp. 222-224).
void op_6a();
// Copy E to L. (UM0050 pp. 222-224).
void op_6b();
// Copy H to L. (UM0050 pp. 222-224).
void op_6c();
// Leave L unchanged; this self-load still consumes instruction time. (UM0050 pp. 222-224).
void op_6d();
// Copy memory at HL to L. (UM0050 pp. 222-224).
void op_6e();
// Copy A to L. (UM0050 pp. 222-224).
void op_6f();
// Copy B to memory at HL. (UM0050 pp. 222-224).
void op_70();
// Copy C to memory at HL. (UM0050 pp. 222-224).
void op_71();
// Copy D to memory at HL. (UM0050 pp. 222-224).
void op_72();
// Copy E to memory at HL. (UM0050 pp. 222-224).
void op_73();
// Copy H to memory at HL. (UM0050 pp. 222-224).
void op_74();
// Copy L to memory at HL. (UM0050 pp. 222-224).
void op_75();
// Stop executing new instructions while bus/refresh clocks continue until an interrupt. (UM0050
// pp. 33, 235).
void op_76();
// Copy A to memory at HL. (UM0050 pp. 222-224).
void op_77();
// Copy B to A. (UM0050 pp. 222-224).
void op_78();
// Copy C to A. (UM0050 pp. 222-224).
void op_79();
// Copy D to A. (UM0050 pp. 222-224).
void op_7a();
// Copy E to A. (UM0050 pp. 222-224).
void op_7b();
// Copy H to A. (UM0050 pp. 222-224).
void op_7c();
// Copy L to A. (UM0050 pp. 222-224).
void op_7d();
// Copy memory at HL to A. (UM0050 pp. 222-224).
void op_7e();
// Leave A unchanged; this self-load still consumes instruction time. (UM0050 pp. 222-224).
void op_7f();
// Add B to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void op_80();
// Add C to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void op_81();
// Add D to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void op_82();
// Add E to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void op_83();
// Add H to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void op_84();
// Add L to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void op_85();
// Add memory at HL to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050
// pp. 211-212).
void op_86();
// Add A to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void op_87();
// Add B and carry to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_88();
// Add C and carry to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_89();
// Add D and carry to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_8a();
// Add E and carry to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_8b();
// Add H and carry to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_8c();
// Add L and carry to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_8d();
// Add memory at HL and carry to A. Store the wrapped result and update 8-bit arithmetic flags.
// (UM0050 pp. 211-212).
void op_8e();
// Add A and carry to A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_8f();
// Subtract B from A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_90();
// Subtract C from A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_91();
// Subtract D from A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_92();
// Subtract E from A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_93();
// Subtract H from A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_94();
// Subtract L from A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_95();
// Subtract memory at HL from A. Store the wrapped result and update 8-bit arithmetic flags.
// (UM0050 pp. 211-212).
void op_96();
// Subtract A from A. Store the wrapped result and update 8-bit arithmetic flags. (UM0050 pp.
// 211-212).
void op_97();
// Subtract B and carry from A. Store the wrapped result and update 8-bit arithmetic flags.
// (UM0050 pp. 211-212).
void op_98();
// Subtract C and carry from A. Store the wrapped result and update 8-bit arithmetic flags.
// (UM0050 pp. 211-212).
void op_99();
// Subtract D and carry from A. Store the wrapped result and update 8-bit arithmetic flags.
// (UM0050 pp. 211-212).
void op_9a();
// Subtract E and carry from A. Store the wrapped result and update 8-bit arithmetic flags.
// (UM0050 pp. 211-212).
void op_9b();
// Subtract H and carry from A. Store the wrapped result and update 8-bit arithmetic flags.
// (UM0050 pp. 211-212).
void op_9c();
// Subtract L and carry from A. Store the wrapped result and update 8-bit arithmetic flags.
// (UM0050 pp. 211-212).
void op_9d();
// Subtract memory at HL and carry from A. Store the wrapped result and update 8-bit arithmetic
// flags. (UM0050 pp. 211-212).
void op_9e();
// Subtract A and carry from A. Store the wrapped result and update 8-bit arithmetic flags.
// (UM0050 pp. 211-212).
void op_9f();
// Combine A with B using bitwise AND. Store A and set logical-result flags. (UM0050 p. 214).
void op_a0();
// Combine A with C using bitwise AND. Store A and set logical-result flags. (UM0050 p. 214).
void op_a1();
// Combine A with D using bitwise AND. Store A and set logical-result flags. (UM0050 p. 214).
void op_a2();
// Combine A with E using bitwise AND. Store A and set logical-result flags. (UM0050 p. 214).
void op_a3();
// Combine A with H using bitwise AND. Store A and set logical-result flags. (UM0050 p. 214).
void op_a4();
// Combine A with L using bitwise AND. Store A and set logical-result flags. (UM0050 p. 214).
void op_a5();
// Combine A with memory at HL using bitwise AND. Store A and set logical-result flags. (UM0050
// p. 214).
void op_a6();
// Combine A with A using bitwise AND. Store A and set logical-result flags. (UM0050 p. 214).
void op_a7();
// Combine A with B using bitwise exclusive OR. Store A and set logical-result flags. (UM0050 p.
// 214).
void op_a8();
// Combine A with C using bitwise exclusive OR. Store A and set logical-result flags. (UM0050 p.
// 214).
void op_a9();
// Combine A with D using bitwise exclusive OR. Store A and set logical-result flags. (UM0050 p.
// 214).
void op_aa();
// Combine A with E using bitwise exclusive OR. Store A and set logical-result flags. (UM0050 p.
// 214).
void op_ab();
// Combine A with H using bitwise exclusive OR. Store A and set logical-result flags. (UM0050 p.
// 214).
void op_ac();
// Combine A with L using bitwise exclusive OR. Store A and set logical-result flags. (UM0050 p.
// 214).
void op_ad();
// Combine A with memory at HL using bitwise exclusive OR. Store A and set logical-result flags.
// (UM0050 p. 214).
void op_ae();
// Combine A with A using bitwise exclusive OR. Store A and set logical-result flags. (UM0050 p.
// 214).
void op_af();
// Combine A with B using bitwise OR. Store A and set logical-result flags. (UM0050 p. 214).
void op_b0();
// Combine A with C using bitwise OR. Store A and set logical-result flags. (UM0050 p. 214).
void op_b1();
// Combine A with D using bitwise OR. Store A and set logical-result flags. (UM0050 p. 214).
void op_b2();
// Combine A with E using bitwise OR. Store A and set logical-result flags. (UM0050 p. 214).
void op_b3();
// Combine A with H using bitwise OR. Store A and set logical-result flags. (UM0050 p. 214).
void op_b4();
// Combine A with L using bitwise OR. Store A and set logical-result flags. (UM0050 p. 214).
void op_b5();
// Combine A with memory at HL using bitwise OR. Store A and set logical-result flags. (UM0050
// p. 214).
void op_b6();
// Combine A with A using bitwise OR. Store A and set logical-result flags. (UM0050 p. 214).
void op_b7();
// Compare A with B by setting subtraction flags; leave A unchanged. (UM0050 p. 213).
void op_b8();
// Compare A with C by setting subtraction flags; leave A unchanged. (UM0050 p. 213).
void op_b9();
// Compare A with D by setting subtraction flags; leave A unchanged. (UM0050 p. 213).
void op_ba();
// Compare A with E by setting subtraction flags; leave A unchanged. (UM0050 p. 213).
void op_bb();
// Compare A with H by setting subtraction flags; leave A unchanged. (UM0050 p. 213).
void op_bc();
// Compare A with L by setting subtraction flags; leave A unchanged. (UM0050 p. 213).
void op_bd();
// Compare A with memory at HL by setting subtraction flags; leave A unchanged. (UM0050 p. 213).
void op_be();
// Compare A with A by setting subtraction flags; leave A unchanged. (UM0050 p. 213).
void op_bf();
// Pop the return address into PC if zero is clear. (UM0050 pp. 229-230).
void op_c0();
// Read the low then high stack bytes into BC and advance SP. (UM0050 p. 227).
void op_c1();
// Set PC to the encoded address if zero is clear. (UM0050 pp. 229-230).
void op_c2();
// Set PC to the encoded address. (UM0050 pp. 229-230).
void op_c3();
// Push the address after this instruction and jump to the encoded address if zero is clear.
// (UM0050 pp. 229-230).
void op_c4();
// Store BC on the descending stack, high byte first. (UM0050 p. 227).
void op_c5();
// Add the next immediate byte to A. Store the wrapped result and update 8-bit arithmetic flags.
// (UM0050 pp. 211-212).
void op_c6();
// Push the return address and enter fixed routine $00. (UM0050 p. 230).
void op_c7();
// Pop the return address into PC if zero is set. (UM0050 pp. 229-230).
void op_c8();
// Pop the return address into PC. (UM0050 pp. 229-230).
void op_c9();
// Set PC to the encoded address if zero is set. (UM0050 pp. 229-230).
void op_ca();
// Fetch the next opcode byte and dispatch the $CB prefix family; include its timing in this
// instruction (UM0050 pp. 247-250).
void op_cb();
// Push the address after this instruction and jump to the encoded address if zero is set.
// (UM0050 pp. 229-230).
void op_cc();
// Push the address after this instruction and jump to the encoded address. (UM0050 pp.
// 229-230).
void op_cd();
// Add the next immediate byte and carry to A. Store the wrapped result and update 8-bit
// arithmetic flags. (UM0050 pp. 211-212).
void op_ce();
// Push the return address and enter fixed routine $08. (UM0050 p. 230).
void op_cf();
// Pop the return address into PC if carry is clear. (UM0050 pp. 229-230).
void op_d0();
// Read the low then high stack bytes into DE and advance SP. (UM0050 p. 227).
void op_d1();
// Set PC to the encoded address if carry is clear. (UM0050 pp. 229-230).
void op_d2();
// Write A to the port formed by A as high byte and the immediate low byte. (UM0050 pp.
// 231-234).
void op_d3();
// Push the address after this instruction and jump to the encoded address if carry is clear.
// (UM0050 pp. 229-230).
void op_d4();
// Store DE on the descending stack, high byte first. (UM0050 p. 227).
void op_d5();
// Subtract the next immediate byte from A. Store the wrapped result and update 8-bit arithmetic
// flags. (UM0050 pp. 211-212).
void op_d6();
// Push the return address and enter fixed routine $10. (UM0050 p. 230).
void op_d7();
// Pop the return address into PC if carry is set. (UM0050 pp. 229-230).
void op_d8();
// Exchange BC, DE and HL with their alternate pairs; leave AF unchanged. (UM0050 p. 228).
void op_d9();
// Set PC to the encoded address if carry is set. (UM0050 pp. 229-230).
void op_da();
// Read the port formed by A as high byte and the immediate low byte into A. (UM0050 pp.
// 231-234).
void op_db();
// Push the address after this instruction and jump to the encoded address if carry is set.
// (UM0050 pp. 229-230).
void op_dc();
// Fetch the next opcode byte and dispatch the $DD prefix family; include its timing in this
// instruction (UM0050 pp. 247-250).
void op_dd();
// Subtract the next immediate byte and carry from A. Store the wrapped result and update 8-bit
// arithmetic flags. (UM0050 pp. 211-212).
void op_de();
// Push the return address and enter fixed routine $18. (UM0050 p. 230).
void op_df();
// Pop the return address into PC if parity/overflow is clear. (UM0050 pp. 229-230).
void op_e0();
// Read the low then high stack bytes into HL and advance SP. (UM0050 p. 227).
void op_e1();
// Set PC to the encoded address if parity/overflow is clear. (UM0050 pp. 229-230).
void op_e2();
// Exchange HL and the word at SP without changing arithmetic flags. (UM0050 p. 228).
void op_e3();
// Push the address after this instruction and jump to the encoded address if parity/overflow is
// clear. (UM0050 pp. 229-230).
void op_e4();
// Store HL on the descending stack, high byte first. (UM0050 p. 227).
void op_e5();
// Combine A with the next immediate byte using bitwise AND. Store A and set logical-result
// flags. (UM0050 p. 214).
void op_e6();
// Push the return address and enter fixed routine $20. (UM0050 p. 230).
void op_e7();
// Pop the return address into PC if parity/overflow is set. (UM0050 pp. 229-230).
void op_e8();
// Set PC to HL (the register value, without a memory read). (UM0050 pp. 229-230).
void op_e9();
// Set PC to the encoded address if parity/overflow is set. (UM0050 pp. 229-230).
void op_ea();
// Exchange DE and HL without changing arithmetic flags. (UM0050 p. 228).
void op_eb();
// Push the address after this instruction and jump to the encoded address if parity/overflow is
// set. (UM0050 pp. 229-230).
void op_ec();
// Fetch the next opcode byte and dispatch the $ED prefix family; include its timing in this
// instruction (UM0050 pp. 247-250).
void op_ed();
// Combine A with the next immediate byte using bitwise exclusive OR. Store A and set logical-
// result flags. (UM0050 p. 214).
void op_ee();
// Push the return address and enter fixed routine $28. (UM0050 p. 230).
void op_ef();
// Pop the return address into PC if sign is clear. (UM0050 pp. 229-230).
void op_f0();
// Read the low then high stack bytes into AF and advance SP. (UM0050 p. 227).
void op_f1();
// Set PC to the encoded address if sign is clear. (UM0050 pp. 229-230).
void op_f2();
// Clear both interrupt enables (UM0050 pp. 68-69, 235). The inherited core
// also delays its next interrupt check through the following instruction.
void op_f3();
// Push the address after this instruction and jump to the encoded address if sign is clear.
// (UM0050 pp. 229-230).
void op_f4();
// Store AF on the descending stack, high byte first. (UM0050 p. 227).
void op_f5();
// Combine A with the next immediate byte using bitwise OR. Store A and set logical-result
// flags. (UM0050 p. 214).
void op_f6();
// Push the return address and enter fixed routine $30. (UM0050 p. 230).
void op_f7();
// Pop the return address into PC if sign is set. (UM0050 pp. 229-230).
void op_f8();
// Copy HL to SP. (UM0050 pp. 222-224).
void op_f9();
// Set PC to the encoded address if sign is set. (UM0050 pp. 229-230).
void op_fa();
// Set both interrupt enables; acceptance waits until the following instruction completes.
// (UM0050 pp. 68-69, 235).
void op_fb();
// Push the address after this instruction and jump to the encoded address if sign is set.
// (UM0050 pp. 229-230).
void op_fc();
// Fetch the next opcode byte and dispatch the $FD prefix family; include its timing in this
// instruction (UM0050 pp. 247-250).
void op_fd();
// Compare A with the next immediate byte by setting subtraction flags; leave A unchanged.
// (UM0050 p. 213).
void op_fe();
// Push the return address and enter fixed routine $38. (UM0050 p. 230).
void op_ff();
// Rotate B left, wrapping bit 7 into bit 0. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_00();
// Rotate C left, wrapping bit 7 into bit 0. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_01();
// Rotate D left, wrapping bit 7 into bit 0. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_02();
// Rotate E left, wrapping bit 7 into bit 0. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_03();
// Rotate H left, wrapping bit 7 into bit 0. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_04();
// Rotate L left, wrapping bit 7 into bit 0. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_05();
// Rotate memory at HL left, wrapping bit 7 into bit 0. Set carry from the outgoing bit and
// flags from the result. (UM0050 pp. 216-220).
void cb_06();
// Rotate A left, wrapping bit 7 into bit 0. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_07();
// Rotate B right, wrapping bit 0 into bit 7. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_08();
// Rotate C right, wrapping bit 0 into bit 7. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_09();
// Rotate D right, wrapping bit 0 into bit 7. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_0a();
// Rotate E right, wrapping bit 0 into bit 7. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_0b();
// Rotate H right, wrapping bit 0 into bit 7. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_0c();
// Rotate L right, wrapping bit 0 into bit 7. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_0d();
// Rotate memory at HL right, wrapping bit 0 into bit 7. Set carry from the outgoing bit and
// flags from the result. (UM0050 pp. 216-220).
void cb_0e();
// Rotate A right, wrapping bit 0 into bit 7. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_0f();
// Rotate B left through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_10();
// Rotate C left through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_11();
// Rotate D left through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_12();
// Rotate E left through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_13();
// Rotate H left through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_14();
// Rotate L left through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_15();
// Rotate memory at HL left through carry. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_16();
// Rotate A left through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_17();
// Rotate B right through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_18();
// Rotate C right through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_19();
// Rotate D right through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_1a();
// Rotate E right through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_1b();
// Rotate H right through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_1c();
// Rotate L right through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_1d();
// Rotate memory at HL right through carry. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_1e();
// Rotate A right through carry. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_1f();
// Shift B left, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_20();
// Shift C left, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_21();
// Shift D left, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_22();
// Shift E left, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_23();
// Shift H left, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_24();
// Shift L left, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_25();
// Shift memory at HL left, inserting zero. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_26();
// Shift A left, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_27();
// Shift B right, retaining its sign bit. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_28();
// Shift C right, retaining its sign bit. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_29();
// Shift D right, retaining its sign bit. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_2a();
// Shift E right, retaining its sign bit. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_2b();
// Shift H right, retaining its sign bit. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_2c();
// Shift L right, retaining its sign bit. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_2d();
// Shift memory at HL right, retaining its sign bit. Set carry from the outgoing bit and flags
// from the result. (UM0050 pp. 216-220).
void cb_2e();
// Shift A right, retaining its sign bit. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_2f();
// Retained Z80 helper: Shift B left, inserting one. Set carry from the outgoing bit and flags
// from the result. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void cb_30();
// Retained Z80 helper: Shift C left, inserting one. Set carry from the outgoing bit and flags
// from the result. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void cb_31();
// Retained Z80 helper: Shift D left, inserting one. Set carry from the outgoing bit and flags
// from the result. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void cb_32();
// Retained Z80 helper: Shift E left, inserting one. Set carry from the outgoing bit and flags
// from the result. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void cb_33();
// Retained Z80 helper: Shift H left, inserting one. Set carry from the outgoing bit and flags
// from the result. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void cb_34();
// Retained Z80 helper: Shift L left, inserting one. Set carry from the outgoing bit and flags
// from the result. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void cb_35();
// Retained Z80 helper: Shift memory at HL left, inserting one. Set carry from the outgoing bit
// and flags from the result. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void cb_36();
// Retained Z80 helper: Shift A left, inserting one. Set carry from the outgoing bit and flags
// from the result. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void cb_37();
// Shift B right, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_38();
// Shift C right, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_39();
// Shift D right, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_3a();
// Shift E right, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_3b();
// Shift H right, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_3c();
// Shift L right, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_3d();
// Shift memory at HL right, inserting zero. Set carry from the outgoing bit and flags from the
// result. (UM0050 pp. 216-220).
void cb_3e();
// Shift A right, inserting zero. Set carry from the outgoing bit and flags from the result.
// (UM0050 pp. 216-220).
void cb_3f();
// Test bit 0 of B in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_40();
// Test bit 0 of C in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_41();
// Test bit 0 of D in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_42();
// Test bit 0 of E in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_43();
// Test bit 0 of H in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_44();
// Test bit 0 of L in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_45();
// Test bit 0 of memory at HL in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_46();
// Test bit 0 of A in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_47();
// Test bit 1 of B in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_48();
// Test bit 1 of C in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_49();
// Test bit 1 of D in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_4a();
// Test bit 1 of E in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_4b();
// Test bit 1 of H in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_4c();
// Test bit 1 of L in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_4d();
// Test bit 1 of memory at HL in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_4e();
// Test bit 1 of A in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_4f();
// Test bit 2 of B in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_50();
// Test bit 2 of C in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_51();
// Test bit 2 of D in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_52();
// Test bit 2 of E in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_53();
// Test bit 2 of H in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_54();
// Test bit 2 of L in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_55();
// Test bit 2 of memory at HL in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_56();
// Test bit 2 of A in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_57();
// Test bit 3 of B in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_58();
// Test bit 3 of C in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_59();
// Test bit 3 of D in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_5a();
// Test bit 3 of E in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_5b();
// Test bit 3 of H in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_5c();
// Test bit 3 of L in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_5d();
// Test bit 3 of memory at HL in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_5e();
// Test bit 3 of A in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_5f();
// Test bit 4 of B in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_60();
// Test bit 4 of C in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_61();
// Test bit 4 of D in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_62();
// Test bit 4 of E in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_63();
// Test bit 4 of H in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_64();
// Test bit 4 of L in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_65();
// Test bit 4 of memory at HL in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_66();
// Test bit 4 of A in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_67();
// Test bit 5 of B in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_68();
// Test bit 5 of C in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_69();
// Test bit 5 of D in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_6a();
// Test bit 5 of E in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_6b();
// Test bit 5 of H in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_6c();
// Test bit 5 of L in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_6d();
// Test bit 5 of memory at HL in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_6e();
// Test bit 5 of A in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_6f();
// Test bit 6 of B in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_70();
// Test bit 6 of C in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_71();
// Test bit 6 of D in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_72();
// Test bit 6 of E in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_73();
// Test bit 6 of H in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_74();
// Test bit 6 of L in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_75();
// Test bit 6 of memory at HL in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_76();
// Test bit 6 of A in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_77();
// Test bit 7 of B in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_78();
// Test bit 7 of C in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_79();
// Test bit 7 of D in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_7a();
// Test bit 7 of E in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_7b();
// Test bit 7 of H in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_7c();
// Test bit 7 of L in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_7d();
// Test bit 7 of memory at HL in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_7e();
// Test bit 7 of A in the flags; leave the operand unchanged. (UM0050 pp. 219-220).
void cb_7f();
// Clear bit 0 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_80();
// Clear bit 0 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_81();
// Clear bit 0 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_82();
// Clear bit 0 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_83();
// Clear bit 0 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_84();
// Clear bit 0 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_85();
// Clear bit 0 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_86();
// Clear bit 0 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_87();
// Clear bit 1 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_88();
// Clear bit 1 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_89();
// Clear bit 1 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_8a();
// Clear bit 1 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_8b();
// Clear bit 1 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_8c();
// Clear bit 1 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_8d();
// Clear bit 1 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_8e();
// Clear bit 1 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_8f();
// Clear bit 2 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_90();
// Clear bit 2 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_91();
// Clear bit 2 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_92();
// Clear bit 2 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_93();
// Clear bit 2 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_94();
// Clear bit 2 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_95();
// Clear bit 2 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_96();
// Clear bit 2 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_97();
// Clear bit 3 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_98();
// Clear bit 3 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_99();
// Clear bit 3 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_9a();
// Clear bit 3 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_9b();
// Clear bit 3 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_9c();
// Clear bit 3 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_9d();
// Clear bit 3 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_9e();
// Clear bit 3 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_9f();
// Clear bit 4 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_a0();
// Clear bit 4 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_a1();
// Clear bit 4 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_a2();
// Clear bit 4 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_a3();
// Clear bit 4 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_a4();
// Clear bit 4 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_a5();
// Clear bit 4 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_a6();
// Clear bit 4 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_a7();
// Clear bit 5 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_a8();
// Clear bit 5 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_a9();
// Clear bit 5 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_aa();
// Clear bit 5 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ab();
// Clear bit 5 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ac();
// Clear bit 5 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ad();
// Clear bit 5 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ae();
// Clear bit 5 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_af();
// Clear bit 6 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_b0();
// Clear bit 6 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_b1();
// Clear bit 6 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_b2();
// Clear bit 6 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_b3();
// Clear bit 6 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_b4();
// Clear bit 6 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_b5();
// Clear bit 6 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_b6();
// Clear bit 6 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_b7();
// Clear bit 7 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_b8();
// Clear bit 7 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_b9();
// Clear bit 7 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ba();
// Clear bit 7 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_bb();
// Clear bit 7 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_bc();
// Clear bit 7 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_bd();
// Clear bit 7 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_be();
// Clear bit 7 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_bf();
// Set bit 0 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_c0();
// Set bit 0 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_c1();
// Set bit 0 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_c2();
// Set bit 0 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_c3();
// Set bit 0 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_c4();
// Set bit 0 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_c5();
// Set bit 0 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_c6();
// Set bit 0 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_c7();
// Set bit 1 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_c8();
// Set bit 1 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_c9();
// Set bit 1 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ca();
// Set bit 1 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_cb();
// Set bit 1 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_cc();
// Set bit 1 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_cd();
// Set bit 1 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ce();
// Set bit 1 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_cf();
// Set bit 2 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_d0();
// Set bit 2 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_d1();
// Set bit 2 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_d2();
// Set bit 2 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_d3();
// Set bit 2 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_d4();
// Set bit 2 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_d5();
// Set bit 2 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_d6();
// Set bit 2 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_d7();
// Set bit 3 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_d8();
// Set bit 3 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_d9();
// Set bit 3 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_da();
// Set bit 3 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_db();
// Set bit 3 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_dc();
// Set bit 3 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_dd();
// Set bit 3 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_de();
// Set bit 3 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_df();
// Set bit 4 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_e0();
// Set bit 4 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_e1();
// Set bit 4 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_e2();
// Set bit 4 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_e3();
// Set bit 4 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_e4();
// Set bit 4 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_e5();
// Set bit 4 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_e6();
// Set bit 4 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_e7();
// Set bit 5 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_e8();
// Set bit 5 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_e9();
// Set bit 5 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ea();
// Set bit 5 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_eb();
// Set bit 5 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ec();
// Set bit 5 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ed();
// Set bit 5 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ee();
// Set bit 5 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ef();
// Set bit 6 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_f0();
// Set bit 6 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_f1();
// Set bit 6 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_f2();
// Set bit 6 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_f3();
// Set bit 6 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_f4();
// Set bit 6 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_f5();
// Set bit 6 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_f6();
// Set bit 6 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_f7();
// Set bit 7 of B; leave flags unchanged. (UM0050 pp. 219-220).
void cb_f8();
// Set bit 7 of C; leave flags unchanged. (UM0050 pp. 219-220).
void cb_f9();
// Set bit 7 of D; leave flags unchanged. (UM0050 pp. 219-220).
void cb_fa();
// Set bit 7 of E; leave flags unchanged. (UM0050 pp. 219-220).
void cb_fb();
// Set bit 7 of H; leave flags unchanged. (UM0050 pp. 219-220).
void cb_fc();
// Set bit 7 of L; leave flags unchanged. (UM0050 pp. 219-220).
void cb_fd();
// Set bit 7 of memory at HL; leave flags unchanged. (UM0050 pp. 219-220).
void cb_fe();
// Set bit 7 of A; leave flags unchanged. (UM0050 pp. 219-220).
void cb_ff();
// Retained invalid-prefix diagnostic helper. Reads the offending bytes for the diagnostic call;
// the Z180 decoder traps before these fallback handlers (UM0050 pp. 68-72).
void illegal_1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $00. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_00();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $01. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_01();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $02. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_02();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $03. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_03();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $04. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_04();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $05. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_05();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $06. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_06();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $07. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_07();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $08. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_08();
// Add BC to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void dd_09();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_0a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_0b();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_0c();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_0d();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_0e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_0f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $10. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_10();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $11. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_11();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $12. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_12();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $13. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_13();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $14. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_14();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $15. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_15();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $16. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_16();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $17. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_17();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $18. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_18();
// Add DE to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void dd_19();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_1a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_1b();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_1c();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_1d();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_1e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_1f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $20. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_20();
// Copy the next immediate word to IX. (UM0050 pp. 222-224).
void dd_21();
// Copy IX to memory at the encoded address. (UM0050 pp. 222-224).
void dd_22();
// Increment IX with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void dd_23();
// Retained Z80 helper: Increment the high byte of IX with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_24();
// Retained Z80 helper: Decrement the high byte of IX with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_25();
// Retained Z80 helper: Copy the next immediate byte to the high byte of IX. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_26();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $27. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_27();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $28. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_28();
// Add IX to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void dd_29();
// Copy memory at the encoded address to IX. (UM0050 pp. 222-224).
void dd_2a();
// Decrement IX with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void dd_2b();
// Retained Z80 helper: Increment the low byte of IX with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_2c();
// Retained Z80 helper: Decrement the low byte of IX with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_2d();
// Retained Z80 helper: Copy the next immediate byte to the low byte of IX. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_2e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $2F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_2f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $30. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_30();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $31. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_31();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $32. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_32();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $33. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_33();
// Increment memory at IX plus the signed displacement with 8-bit wrap; update arithmetic flags
// and preserve carry. (UM0050 p. 213).
void dd_34();
// Decrement memory at IX plus the signed displacement with 8-bit wrap; update arithmetic flags
// and preserve carry. (UM0050 p. 213).
void dd_35();
// Copy the next immediate byte to memory at IX plus the signed displacement. (UM0050 pp.
// 222-224).
void dd_36();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $37. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_37();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $38. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_38();
// Add SP to IX. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void dd_39();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_3a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_3b();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_3c();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_3d();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_3e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_3f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $40. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_40();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $41. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_41();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $42. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_42();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $43. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_43();
// Retained Z80 helper: Copy the high byte of IX to B. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_44();
// Retained Z80 helper: Copy the low byte of IX to B. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_45();
// Copy memory at IX plus the signed displacement to B. (UM0050 pp. 222-224).
void dd_46();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $47. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_47();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $48. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_48();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $49. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_49();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_4a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_4b();
// Retained Z80 helper: Copy the high byte of IX to C. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_4c();
// Retained Z80 helper: Copy the low byte of IX to C. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_4d();
// Copy memory at IX plus the signed displacement to C. (UM0050 pp. 222-224).
void dd_4e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_4f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $50. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_50();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $51. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_51();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $52. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_52();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $53. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_53();
// Retained Z80 helper: Copy the high byte of IX to D. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_54();
// Retained Z80 helper: Copy the low byte of IX to D. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_55();
// Copy memory at IX plus the signed displacement to D. (UM0050 pp. 222-224).
void dd_56();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $57. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_57();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $58. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_58();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $59. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_59();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_5a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_5b();
// Retained Z80 helper: Copy the high byte of IX to E. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_5c();
// Retained Z80 helper: Copy the low byte of IX to E. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_5d();
// Copy memory at IX plus the signed displacement to E. (UM0050 pp. 222-224).
void dd_5e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_5f();
// Retained Z80 helper: Copy B to the high byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_60();
// Retained Z80 helper: Copy C to the high byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_61();
// Retained Z80 helper: Copy D to the high byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_62();
// Retained Z80 helper: Copy E to the high byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_63();
// Retained Z80 helper: Leave the high byte of IX unchanged; this self-load still consumes
// instruction time. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_64();
// Retained Z80 helper: Copy the low byte of IX to the high byte of IX. The Z180 decoder traps
// this encoding (UM0050 pp. 68-72, 247-250).
void dd_65();
// Copy memory at IX plus the signed displacement to H. (UM0050 pp. 222-224).
void dd_66();
// Retained Z80 helper: Copy A to the high byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_67();
// Retained Z80 helper: Copy B to the low byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_68();
// Retained Z80 helper: Copy C to the low byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_69();
// Retained Z80 helper: Copy D to the low byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_6a();
// Retained Z80 helper: Copy E to the low byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_6b();
// Retained Z80 helper: Copy the high byte of IX to the low byte of IX. The Z180 decoder traps
// this encoding (UM0050 pp. 68-72, 247-250).
void dd_6c();
// Retained Z80 helper: Leave the low byte of IX unchanged; this self-load still consumes
// instruction time. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_6d();
// Copy memory at IX plus the signed displacement to L. (UM0050 pp. 222-224).
void dd_6e();
// Retained Z80 helper: Copy A to the low byte of IX. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_6f();
// Copy B to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
void dd_70();
// Copy C to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
void dd_71();
// Copy D to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
void dd_72();
// Copy E to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
void dd_73();
// Copy H to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
void dd_74();
// Copy L to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
void dd_75();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $76. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_76();
// Copy A to memory at IX plus the signed displacement. (UM0050 pp. 222-224).
void dd_77();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $78. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_78();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $79. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_79();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_7a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_7b();
// Retained Z80 helper: Copy the high byte of IX to A. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_7c();
// Retained Z80 helper: Copy the low byte of IX to A. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void dd_7d();
// Copy memory at IX plus the signed displacement to A. (UM0050 pp. 222-224).
void dd_7e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_7f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $80. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_80();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $81. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_81();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $82. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_82();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $83. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_83();
// Retained Z80 helper: Add the high byte of IX to A. Store the wrapped result and update 8-bit
// arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_84();
// Retained Z80 helper: Add the low byte of IX to A. Store the wrapped result and update 8-bit
// arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_85();
// Add memory at IX plus the signed displacement to A. Store the wrapped result and update 8-bit
// arithmetic flags. (UM0050 pp. 211-212).
void dd_86();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $87. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_87();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $88. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_88();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $89. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_89();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_8a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_8b();
// Retained Z80 helper: Add the high byte of IX and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void dd_8c();
// Retained Z80 helper: Add the low byte of IX and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void dd_8d();
// Add memory at IX plus the signed displacement and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void dd_8e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_8f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $90. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_90();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $91. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_91();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $92. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_92();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $93. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_93();
// Retained Z80 helper: Subtract the high byte of IX from A. Store the wrapped result and update
// 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_94();
// Retained Z80 helper: Subtract the low byte of IX from A. Store the wrapped result and update
// 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_95();
// Subtract memory at IX plus the signed displacement from A. Store the wrapped result and
// update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void dd_96();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $97. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_97();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $98. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_98();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $99. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_99();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_9a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_9b();
// Retained Z80 helper: Subtract the high byte of IX and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void dd_9c();
// Retained Z80 helper: Subtract the low byte of IX and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void dd_9d();
// Subtract memory at IX plus the signed displacement and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void dd_9e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_9f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_a0();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_a1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_a2();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_a3();
// Retained Z80 helper: Combine A with the high byte of IX using bitwise AND. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_a4();
// Retained Z80 helper: Combine A with the low byte of IX using bitwise AND. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_a5();
// Combine A with memory at IX plus the signed displacement using bitwise AND. Store A and set
// logical-result flags. (UM0050 p. 214).
void dd_a6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_a7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_a8();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_a9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_aa();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_ab();
// Retained Z80 helper: Combine A with the high byte of IX using bitwise exclusive OR. Store A
// and set logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void dd_ac();
// Retained Z80 helper: Combine A with the low byte of IX using bitwise exclusive OR. Store A
// and set logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void dd_ad();
// Combine A with memory at IX plus the signed displacement using bitwise exclusive OR. Store A
// and set logical-result flags. (UM0050 p. 214).
void dd_ae();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_af();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_b0();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_b1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_b2();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_b3();
// Retained Z80 helper: Combine A with the high byte of IX using bitwise OR. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_b4();
// Retained Z80 helper: Combine A with the low byte of IX using bitwise OR. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_b5();
// Combine A with memory at IX plus the signed displacement using bitwise OR. Store A and set
// logical-result flags. (UM0050 p. 214).
void dd_b6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_b7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_b8();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_b9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_ba();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_bb();
// Retained Z80 helper: Compare A with the high byte of IX by setting subtraction flags; leave A
// unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_bc();
// Retained Z80 helper: Compare A with the low byte of IX by setting subtraction flags; leave A
// unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void dd_bd();
// Compare A with memory at IX plus the signed displacement by setting subtraction flags; leave
// A unchanged. (UM0050 p. 213).
void dd_be();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_bf();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_c0();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_c1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_c2();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_c3();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_c4();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_c5();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_c6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_c7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_c8();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_c9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_ca();
// Read the signed index displacement, save its effective address, then fetch and dispatch the
// CB bit-operation byte (UM0050 pp. 182, 249, 267-271).
void dd_cb();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_cc();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_cd();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_ce();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_cf();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_d0();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_d1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_d2();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_d3();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_d4();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_d5();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_d6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_d7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_d8();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_d9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_da();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_db();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_dc();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_dd();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_de();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_df();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_e0();
// Read the low then high stack bytes into IX and advance SP. (UM0050 p. 227).
void dd_e1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_e2();
// Exchange the word at SP and IX without changing arithmetic flags. (UM0050 p. 228).
void dd_e3();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_e4();
// Store IX on the descending stack, high byte first. (UM0050 p. 227).
void dd_e5();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_e6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_e7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_e8();
// Set PC to IX (the register value, without a memory read). (UM0050 pp. 229-230).
void dd_e9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_ea();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_eb();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_ec();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $ED. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_ed();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_ee();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_ef();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_f0();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_f1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_f2();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_f3();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_f4();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_f5();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_f6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_f7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_f8();
// Copy IX to SP. (UM0050 pp. 222-224).
void dd_f9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_fa();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_fb();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_fc();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_fd();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_fe();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void dd_ff();
// Retained invalid-prefix diagnostic helper. Reads the offending bytes for the diagnostic call;
// the Z180 decoder traps before these fallback handlers (UM0050 pp. 68-72).
void illegal_2();
// Read the immediate 8-bit port into B. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_00();
// Write B to the immediate 8-bit port. (UM0050 pp. 231-234).
void ed_01();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_02();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_03();
// Set flags from A AND B without changing either operand. (UM0050 p. 214).
void ed_04();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_05();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_06();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_07();
// Read the immediate 8-bit port into C. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_08();
// Write C to the immediate 8-bit port. (UM0050 pp. 231-234).
void ed_09();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_0a();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_0b();
// Set flags from A AND C without changing either operand. (UM0050 p. 214).
void ed_0c();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_0d();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_0e();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_0f();
// Read the immediate 8-bit port into D. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_10();
// Write D to the immediate 8-bit port. (UM0050 pp. 231-234).
void ed_11();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_12();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_13();
// Set flags from A AND D without changing either operand. (UM0050 p. 214).
void ed_14();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_15();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_16();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_17();
// Read the immediate 8-bit port into E. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_18();
// Write E to the immediate 8-bit port. (UM0050 pp. 231-234).
void ed_19();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_1a();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_1b();
// Set flags from A AND E without changing either operand. (UM0050 p. 214).
void ed_1c();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_1d();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_1e();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_1f();
// Read the immediate 8-bit port into H. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_20();
// Write H to the immediate 8-bit port. (UM0050 pp. 231-234).
void ed_21();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_22();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_23();
// Set flags from A AND H without changing either operand. (UM0050 p. 214).
void ed_24();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_25();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_26();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_27();
// Read the immediate 8-bit port into L. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_28();
// Write L to the immediate 8-bit port. (UM0050 pp. 231-234).
void ed_29();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_2a();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_2b();
// Set flags from A AND L without changing either operand. (UM0050 p. 214).
void ed_2c();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_2d();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_2e();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_2f();
// Read the immediate 8-bit port; discard the byte but update flags. (UM0050 pp. 231-234).
void ed_30();
// Retained Z80 helper: Write zero to the immediate 8-bit port. The Z180 decoder traps this
// encoding (UM0050 pp. 68-72, 247-250).
void ed_31();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_32();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_33();
// Set flags from A AND memory at HL without changing either operand. (UM0050 p. 214).
void ed_34();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_35();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_36();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_37();
// Read the immediate 8-bit port into A. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_38();
// Write A to the immediate 8-bit port. (UM0050 pp. 231-234).
void ed_39();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_3a();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_3b();
// Set flags from A AND A without changing either operand. (UM0050 p. 214).
void ed_3c();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_3d();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_3e();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_3f();
// Read the 16-bit port BC into B. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_40();
// Write B to the 16-bit port BC. (UM0050 pp. 231-234).
void ed_41();
// Subtract BC and carry from HL. Store the wrapped result and update 16-bit arithmetic flags.
// (UM0050 p. 221).
void ed_42();
// Copy BC to memory at the encoded address. (UM0050 pp. 222-224).
void ed_43();
// Replace A with zero minus A and set subtraction flags. (UM0050 p. 215).
void ed_44();
// Pop the return PC and restore the interrupt enable saved on NMI entry. (UM0050 pp. 73-75,
// 230).
void ed_45();
// Select INT0 interrupt mode 0. Other interrupt sources retain their fixed vector rules.
// (UM0050 pp. 75-86, 235).
void ed_46();
// Copy A to I. (UM0050 pp. 222-224).
void ed_47();
// Read the 16-bit port BC into C. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_48();
// Write C to the 16-bit port BC. (UM0050 pp. 231-234).
void ed_49();
// Add BC and carry to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050
// p. 221).
void ed_4a();
// Copy memory at the encoded address to BC. (UM0050 pp. 222-224).
void ed_4b();
// Multiply the low and high bytes of BC as unsigned values; store their 16-bit product in that
// pair. (UM0050 p. 221).
void ed_4c();
// Pop the return PC and signal return-from-interrupt to the board; M1 mode may add refetch
// cycles. (UM0050 pp. 84-85, 230).
void ed_4d();
// Retained Z80 helper: Select INT0 interrupt mode 0. Other interrupt sources retain their fixed
// vector rules. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_4e();
// Copy A to R. (UM0050 pp. 222-224).
void ed_4f();
// Read the 16-bit port BC into D. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_50();
// Write D to the 16-bit port BC. (UM0050 pp. 231-234).
void ed_51();
// Subtract DE and carry from HL. Store the wrapped result and update 16-bit arithmetic flags.
// (UM0050 p. 221).
void ed_52();
// Copy DE to memory at the encoded address. (UM0050 pp. 222-224).
void ed_53();
// Retained Z80 helper: Replace A with zero minus A and set subtraction flags. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_54();
// Retained Z80 helper: Pop the return PC and restore the interrupt enable saved on NMI entry.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_55();
// Select INT0 interrupt mode 1. Other interrupt sources retain their fixed vector rules.
// (UM0050 pp. 75-86, 235).
void ed_56();
// Copy I to A. Set parity/overflow from the saved interrupt enable. (UM0050 pp. 222-224).
void ed_57();
// Read the 16-bit port BC into E. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_58();
// Write E to the 16-bit port BC. (UM0050 pp. 231-234).
void ed_59();
// Add DE and carry to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050
// p. 221).
void ed_5a();
// Copy memory at the encoded address to DE. (UM0050 pp. 222-224).
void ed_5b();
// Multiply the low and high bytes of DE as unsigned values; store their 16-bit product in that
// pair. (UM0050 p. 221).
void ed_5c();
// Retained Z80 helper: Pop the return PC and signal return-from-interrupt to the board; M1 mode
// may add refetch cycles. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_5d();
// Select INT0 interrupt mode 2. Other interrupt sources retain their fixed vector rules.
// (UM0050 pp. 75-86, 235).
void ed_5e();
// Copy R to A. Set parity/overflow from the saved interrupt enable. (UM0050 pp. 222-224).
void ed_5f();
// Read the 16-bit port BC into H. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_60();
// Write H to the 16-bit port BC. (UM0050 pp. 231-234).
void ed_61();
// Subtract HL and carry from HL. Store the wrapped result and update 16-bit arithmetic flags.
// (UM0050 p. 221).
void ed_62();
// Copy HL to memory at the encoded address. (UM0050 pp. 222-224).
void ed_63();
// Set flags from A AND the next immediate mask without changing either operand. (UM0050 p.
// 214).
void ed_64();
// Retained Z80 helper: Pop the return PC and restore the interrupt enable saved on NMI entry.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_65();
// Retained Z80 helper: Select INT0 interrupt mode 0. Other interrupt sources retain their fixed
// vector rules. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_66();
// Rotate four-bit digits right between A's low digit and memory at HL; preserve carry. (UM0050
// p. 218).
void ed_67();
// Read the 16-bit port BC into L. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_68();
// Write L to the 16-bit port BC. (UM0050 pp. 231-234).
void ed_69();
// Add HL and carry to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050
// p. 221).
void ed_6a();
// Copy memory at the encoded address to HL. (UM0050 pp. 222-224).
void ed_6b();
// Multiply the low and high bytes of HL as unsigned values; store their 16-bit product in that
// pair. (UM0050 p. 221).
void ed_6c();
// Retained Z80 helper: Pop the return PC and signal return-from-interrupt to the board; M1 mode
// may add refetch cycles. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_6d();
// Retained Z80 helper: Select INT0 interrupt mode 0. Other interrupt sources retain their fixed
// vector rules. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_6e();
// Rotate four-bit digits left between A's low digit and memory at HL; preserve carry. (UM0050
// p. 218).
void ed_6f();
// Read the 16-bit port BC; discard the byte but update flags. (UM0050 pp. 231-234).
void ed_70();
// Retained Z80 helper: Write 0 to the 16-bit port BC. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void ed_71();
// Subtract SP and carry from HL. Store the wrapped result and update 16-bit arithmetic flags.
// (UM0050 p. 221).
void ed_72();
// Copy SP to memory at the encoded address. (UM0050 pp. 222-224).
void ed_73();
// Read port C, AND it with the immediate mask, and set flags without retaining the byte.
// (UM0050 p. 234).
void ed_74();
// Retained Z80 helper: Pop the return PC and restore the interrupt enable saved on NMI entry.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_75();
// Enter SLP; clock stopping and wake delay depend on IOCR/CCR and silicon revision. (UM0050 pp.
// 34-41, 235).
void ed_76();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_77();
// Read the 16-bit port BC into A. Set sign/zero/parity and preserve carry. (UM0050 pp.
// 231-234).
void ed_78();
// Write A to the 16-bit port BC. (UM0050 pp. 231-234).
void ed_79();
// Add SP and carry to HL. Store the wrapped result and update 16-bit arithmetic flags. (UM0050
// p. 221).
void ed_7a();
// Copy memory at the encoded address to SP. (UM0050 pp. 222-224).
void ed_7b();
// Multiply the low and high bytes of SP as unsigned values; store their 16-bit product in that
// pair. (UM0050 p. 221).
void ed_7c();
// Retained Z80 helper: Pop the return PC and signal return-from-interrupt to the board; M1 mode
// may add refetch cycles. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_7d();
// Retained Z80 helper: Select INT0 interrupt mode 2. Other interrupt sources retain their fixed
// vector rules. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void ed_7e();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_7f();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_80();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_81();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_82();
// Write memory at HL to port C; decrement B and increment HL. Move C in the same direction.
// (UM0050 pp. 232-234).
void ed_83();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_84();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_85();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_86();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_87();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_88();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_89();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_8a();
// Write memory at HL to port C; decrement B and decrement HL. Move C in the same direction.
// (UM0050 pp. 232-234).
void ed_8b();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_8c();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_8d();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_8e();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_8f();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_90();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_91();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_92();
// Write memory at HL to port C; decrement B and increment HL. Move C in the same direction.
// Repeat until B reaches zero; PC backs up so each byte remains interruptible. (UM0050 pp.
// 232-234).
void ed_93();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_94();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_95();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_96();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_97();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_98();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_99();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_9a();
// Write memory at HL to port C; decrement B and decrement HL. Move C in the same direction.
// Repeat until B reaches zero; PC backs up so each byte remains interruptible. (UM0050 pp.
// 232-234).
void ed_9b();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_9c();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_9d();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_9e();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_9f();
// Copy one byte from HL to DE; increment both addresses and decrement BC. (UM0050 p. 225).
void ed_a0();
// Compare A with memory at HL; increment HL and decrement BC. (UM0050 p. 226).
void ed_a1();
// Read port BC into memory at HL; decrement B and increment HL. (UM0050 pp. 232-234).
void ed_a2();
// Read memory at HL, decrement B, then write to port BC and increment HL. (UM0050 pp. 232-234).
void ed_a3();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_a4();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_a5();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_a6();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_a7();
// Copy one byte from HL to DE; decrement both addresses and decrement BC. (UM0050 p. 225).
void ed_a8();
// Compare A with memory at HL; decrement HL and decrement BC. (UM0050 p. 226).
void ed_a9();
// Read port BC into memory at HL; decrement B and decrement HL. (UM0050 pp. 232-234).
void ed_aa();
// Read memory at HL, decrement B, then write to port BC and decrement HL. (UM0050 pp. 232-234).
void ed_ab();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ac();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ad();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ae();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_af();
// Copy one byte from HL to DE; increment both addresses and decrement BC. Repeat by backing up
// PC while BC is nonzero, allowing interrupts between bytes. (UM0050 p. 225).
void ed_b0();
// Compare A with memory at HL; increment HL and decrement BC. Repeat until a match or BC
// reaches zero. (UM0050 p. 226).
void ed_b1();
// Read port BC into memory at HL; decrement B and increment HL. Repeat until B reaches zero; PC
// backs up so each byte remains interruptible. (UM0050 pp. 232-234).
void ed_b2();
// Read memory at HL, decrement B, then write to port BC and increment HL. Repeat until B reaches zero; PC
// backs up so each byte remains interruptible. (UM0050 pp. 232-234).
void ed_b3();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_b4();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_b5();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_b6();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_b7();
// Copy one byte from HL to DE; decrement both addresses and decrement BC. Repeat by backing up
// PC while BC is nonzero, allowing interrupts between bytes. (UM0050 p. 225).
void ed_b8();
// Compare A with memory at HL; decrement HL and decrement BC. Repeat until a match or BC
// reaches zero. (UM0050 p. 226).
void ed_b9();
// Read port BC into memory at HL; decrement B and decrement HL. Repeat until B reaches zero; PC
// backs up so each byte remains interruptible. (UM0050 pp. 232-234).
void ed_ba();
// Read memory at HL, decrement B, then write to port BC and decrement HL. Repeat until B reaches zero; PC
// backs up so each byte remains interruptible. (UM0050 pp. 232-234).
void ed_bb();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_bc();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_bd();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_be();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_bf();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_c0();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_c1();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_c2();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_c3();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_c4();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_c5();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_c6();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_c7();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_c8();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_c9();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ca();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_cb();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_cc();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_cd();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ce();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_cf();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_d0();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_d1();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_d2();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_d3();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_d4();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_d5();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_d6();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_d7();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_d8();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_d9();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_da();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_db();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_dc();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_dd();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_de();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_df();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_e0();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_e1();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_e2();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_e3();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_e4();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_e5();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_e6();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_e7();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_e8();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_e9();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ea();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_eb();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ec();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ed();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ee();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ef();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_f0();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_f1();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_f2();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_f3();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_f4();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_f5();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_f6();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_f7();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_f8();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_f9();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_fa();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_fb();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_fc();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_fd();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_fe();
// Retained invalid ED-byte diagnostic handler. The Z180 decoder takes an undefined-instruction
// trap before this handler (UM0050 pp. 68-72, 247-250).
void ed_ff();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $00. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_00();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $01. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_01();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $02. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_02();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $03. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_03();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $04. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_04();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $05. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_05();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $06. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_06();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $07. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_07();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $08. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_08();
// Add BC to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void fd_09();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_0a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_0b();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_0c();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_0d();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_0e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $0F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_0f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $10. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_10();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $11. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_11();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $12. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_12();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $13. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_13();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $14. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_14();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $15. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_15();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $16. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_16();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $17. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_17();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $18. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_18();
// Add DE to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void fd_19();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_1a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_1b();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_1c();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_1d();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_1e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $1F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_1f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $20. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_20();
// Copy the next immediate word to IY. (UM0050 pp. 222-224).
void fd_21();
// Copy IY to memory at the encoded address. (UM0050 pp. 222-224).
void fd_22();
// Increment IY with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void fd_23();
// Retained Z80 helper: Increment the high byte of IY with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_24();
// Retained Z80 helper: Decrement the high byte of IY with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_25();
// Retained Z80 helper: Copy the next immediate byte to the high byte of IY. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_26();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $27. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_27();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $28. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_28();
// Add IY to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void fd_29();
// Copy memory at the encoded address to IY. (UM0050 pp. 222-224).
void fd_2a();
// Decrement IY with 16-bit wrap; flags stay unchanged. (UM0050 p. 221).
void fd_2b();
// Retained Z80 helper: Increment the low byte of IY with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_2c();
// Retained Z80 helper: Decrement the low byte of IY with 8-bit wrap; update arithmetic flags
// and preserve carry. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_2d();
// Retained Z80 helper: Copy the next immediate byte to the low byte of IY. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_2e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $2F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_2f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $30. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_30();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $31. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_31();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $32. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_32();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $33. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_33();
// Increment memory at IY plus the signed displacement with 8-bit wrap; update arithmetic flags
// and preserve carry. (UM0050 p. 213).
void fd_34();
// Decrement memory at IY plus the signed displacement with 8-bit wrap; update arithmetic flags
// and preserve carry. (UM0050 p. 213).
void fd_35();
// Copy the next immediate byte to memory at IY plus the signed displacement. (UM0050 pp.
// 222-224).
void fd_36();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $37. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_37();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $38. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_38();
// Add SP to IY. Store the wrapped result and update 16-bit arithmetic flags. (UM0050 p. 221).
void fd_39();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_3a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_3b();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3C. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_3c();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3D. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_3d();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3E. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_3e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $3F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_3f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $40. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_40();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $41. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_41();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $42. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_42();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $43. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_43();
// Retained Z80 helper: Copy the high byte of IY to B. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_44();
// Retained Z80 helper: Copy the low byte of IY to B. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_45();
// Copy memory at IY plus the signed displacement to B. (UM0050 pp. 222-224).
void fd_46();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $47. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_47();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $48. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_48();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $49. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_49();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_4a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_4b();
// Retained Z80 helper: Copy the high byte of IY to C. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_4c();
// Retained Z80 helper: Copy the low byte of IY to C. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_4d();
// Copy memory at IY plus the signed displacement to C. (UM0050 pp. 222-224).
void fd_4e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $4F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_4f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $50. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_50();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $51. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_51();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $52. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_52();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $53. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_53();
// Retained Z80 helper: Copy the high byte of IY to D. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_54();
// Retained Z80 helper: Copy the low byte of IY to D. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_55();
// Copy memory at IY plus the signed displacement to D. (UM0050 pp. 222-224).
void fd_56();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $57. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_57();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $58. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_58();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $59. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_59();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_5a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_5b();
// Retained Z80 helper: Copy the high byte of IY to E. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_5c();
// Retained Z80 helper: Copy the low byte of IY to E. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_5d();
// Copy memory at IY plus the signed displacement to E. (UM0050 pp. 222-224).
void fd_5e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $5F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_5f();
// Retained Z80 helper: Copy B to the high byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_60();
// Retained Z80 helper: Copy C to the high byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_61();
// Retained Z80 helper: Copy D to the high byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_62();
// Retained Z80 helper: Copy E to the high byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_63();
// Retained Z80 helper: Leave the high byte of IY unchanged; this self-load still consumes
// instruction time. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_64();
// Retained Z80 helper: Copy the low byte of IY to the high byte of IY. The Z180 decoder traps
// this encoding (UM0050 pp. 68-72, 247-250).
void fd_65();
// Copy memory at IY plus the signed displacement to H. (UM0050 pp. 222-224).
void fd_66();
// Retained Z80 helper: Copy A to the high byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_67();
// Retained Z80 helper: Copy B to the low byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_68();
// Retained Z80 helper: Copy C to the low byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_69();
// Retained Z80 helper: Copy D to the low byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_6a();
// Retained Z80 helper: Copy E to the low byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_6b();
// Retained Z80 helper: Copy the high byte of IY to the low byte of IY. The Z180 decoder traps
// this encoding (UM0050 pp. 68-72, 247-250).
void fd_6c();
// Retained Z80 helper: Leave the low byte of IY unchanged; this self-load still consumes
// instruction time. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_6d();
// Copy memory at IY plus the signed displacement to L. (UM0050 pp. 222-224).
void fd_6e();
// Retained Z80 helper: Copy A to the low byte of IY. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_6f();
// Copy B to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
void fd_70();
// Copy C to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
void fd_71();
// Copy D to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
void fd_72();
// Copy E to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
void fd_73();
// Copy H to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
void fd_74();
// Copy L to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
void fd_75();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $76. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_76();
// Copy A to memory at IY plus the signed displacement. (UM0050 pp. 222-224).
void fd_77();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $78. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_78();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $79. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_79();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_7a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_7b();
// Retained Z80 helper: Copy the high byte of IY to A. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_7c();
// Retained Z80 helper: Copy the low byte of IY to A. The Z180 decoder traps this encoding
// (UM0050 pp. 68-72, 247-250).
void fd_7d();
// Copy memory at IY plus the signed displacement to A. (UM0050 pp. 222-224).
void fd_7e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $7F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_7f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $80. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_80();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $81. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_81();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $82. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_82();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $83. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_83();
// Retained Z80 helper: Add the high byte of IY to A. Store the wrapped result and update 8-bit
// arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_84();
// Retained Z80 helper: Add the low byte of IY to A. Store the wrapped result and update 8-bit
// arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_85();
// Add memory at IY plus the signed displacement to A. Store the wrapped result and update 8-bit
// arithmetic flags. (UM0050 pp. 211-212).
void fd_86();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $87. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_87();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $88. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_88();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $89. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_89();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_8a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_8b();
// Retained Z80 helper: Add the high byte of IY and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void fd_8c();
// Retained Z80 helper: Add the low byte of IY and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void fd_8d();
// Add memory at IY plus the signed displacement and carry to A. Store the wrapped result and
// update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void fd_8e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $8F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_8f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $90. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_90();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $91. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_91();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $92. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_92();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $93. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_93();
// Retained Z80 helper: Subtract the high byte of IY from A. Store the wrapped result and update
// 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_94();
// Retained Z80 helper: Subtract the low byte of IY from A. Store the wrapped result and update
// 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_95();
// Subtract memory at IY plus the signed displacement from A. Store the wrapped result and
// update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void fd_96();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $97. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_97();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $98. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_98();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $99. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_99();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9A. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_9a();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9B. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_9b();
// Retained Z80 helper: Subtract the high byte of IY and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void fd_9c();
// Retained Z80 helper: Subtract the low byte of IY and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void fd_9d();
// Subtract memory at IY plus the signed displacement and carry from A. Store the wrapped result
// and update 8-bit arithmetic flags. (UM0050 pp. 211-212).
void fd_9e();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $9F. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_9f();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_a0();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_a1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_a2();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_a3();
// Retained Z80 helper: Combine A with the high byte of IY using bitwise AND. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_a4();
// Retained Z80 helper: Combine A with the low byte of IY using bitwise AND. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_a5();
// Combine A with memory at IY plus the signed displacement using bitwise AND. Store A and set
// logical-result flags. (UM0050 p. 214).
void fd_a6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_a7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_a8();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $A9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_a9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_aa();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_ab();
// Retained Z80 helper: Combine A with the high byte of IY using bitwise exclusive OR. Store A
// and set logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void fd_ac();
// Retained Z80 helper: Combine A with the low byte of IY using bitwise exclusive OR. Store A
// and set logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72,
// 247-250).
void fd_ad();
// Combine A with memory at IY plus the signed displacement using bitwise exclusive OR. Store A
// and set logical-result flags. (UM0050 p. 214).
void fd_ae();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $AF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_af();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_b0();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_b1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_b2();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_b3();
// Retained Z80 helper: Combine A with the high byte of IY using bitwise OR. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_b4();
// Retained Z80 helper: Combine A with the low byte of IY using bitwise OR. Store A and set
// logical-result flags. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_b5();
// Combine A with memory at IY plus the signed displacement using bitwise OR. Store A and set
// logical-result flags. (UM0050 p. 214).
void fd_b6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_b7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_b8();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $B9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_b9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_ba();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_bb();
// Retained Z80 helper: Compare A with the high byte of IY by setting subtraction flags; leave A
// unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_bc();
// Retained Z80 helper: Compare A with the low byte of IY by setting subtraction flags; leave A
// unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void fd_bd();
// Compare A with memory at IY plus the signed displacement by setting subtraction flags; leave
// A unchanged. (UM0050 p. 213).
void fd_be();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $BF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_bf();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_c0();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_c1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_c2();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_c3();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_c4();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_c5();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_c6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_c7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_c8();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $C9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_c9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_ca();
// Read the signed index displacement, save its effective address, then fetch and dispatch the
// CB bit-operation byte (UM0050 pp. 182, 249, 267-271).
void fd_cb();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_cc();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_cd();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_ce();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $CF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_cf();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_d0();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_d1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_d2();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_d3();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_d4();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_d5();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_d6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_d7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_d8();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $D9. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_d9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_da();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_db();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_dc();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_dd();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_de();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $DF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_df();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_e0();
// Read the low then high stack bytes into IY and advance SP. (UM0050 p. 227).
void fd_e1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_e2();
// Exchange the word at SP and IY without changing arithmetic flags. (UM0050 p. 228).
void fd_e3();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_e4();
// Store IY on the descending stack, high byte first. (UM0050 p. 227).
void fd_e5();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_e6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_e7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $E8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_e8();
// Set PC to IY (the register value, without a memory read). (UM0050 pp. 229-230).
void fd_e9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_ea();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_eb();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_ec();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $ED. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_ed();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_ee();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $EF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_ef();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F0. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_f0();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F1. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_f1();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F2. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_f2();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F3. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_f3();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F4. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_f4();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F5. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_f5();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F6. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_f6();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F7. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_f7();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $F8. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_f8();
// Copy IY to SP. (UM0050 pp. 222-224).
void fd_f9();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FA. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_fa();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FB. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_fb();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FC. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_fc();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FD. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_fd();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FE. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_fe();
// Retained Z80 fallback: diagnose the prefix and call unprefixed opcode $FF. The Z180 decoder
// takes an undefined-instruction trap before this handler (UM0050 pp. 68-72, 247-250).
void fd_ff();
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to B. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_00();
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to C. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_01();
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to D. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_02();
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to E. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_03();
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to H. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_04();
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to L. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_05();
// Rotate memory at the saved indexed address left, wrapping bit 7 into bit 0. Set carry from
// the outgoing bit and flags from the result. (UM0050 pp. 216-220).
void xycb_06();
// Retained Z80 helper: Rotate memory at the saved indexed address left, wrapping bit 7 into bit
// 0. Set carry from the outgoing bit and flags from the result. Also copy the result to A. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_07();
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to B.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_08();
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to C.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_09();
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to D.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_0a();
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to E.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_0b();
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to H.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_0c();
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to L.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_0d();
// Rotate memory at the saved indexed address right, wrapping bit 0 into bit 7. Set carry from
// the outgoing bit and flags from the result. (UM0050 pp. 216-220).
void xycb_0e();
// Retained Z80 helper: Rotate memory at the saved indexed address right, wrapping bit 0 into
// bit 7. Set carry from the outgoing bit and flags from the result. Also copy the result to A.
// The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_0f();
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to B. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_10();
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to C. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_11();
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to D. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_12();
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to E. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_13();
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to H. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_14();
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to L. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_15();
// Rotate memory at the saved indexed address left through carry. Set carry from the outgoing
// bit and flags from the result. (UM0050 pp. 216-220).
void xycb_16();
// Retained Z80 helper: Rotate memory at the saved indexed address left through carry. Set carry
// from the outgoing bit and flags from the result. Also copy the result to A. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_17();
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to B. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_18();
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to C. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_19();
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to D. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_1a();
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to E. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_1b();
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to H. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_1c();
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to L. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_1d();
// Rotate memory at the saved indexed address right through carry. Set carry from the outgoing
// bit and flags from the result. (UM0050 pp. 216-220).
void xycb_1e();
// Retained Z80 helper: Rotate memory at the saved indexed address right through carry. Set
// carry from the outgoing bit and flags from the result. Also copy the result to A. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_1f();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to B. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_20();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to C. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_21();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to D. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_22();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to E. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_23();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to H. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_24();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to L. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_25();
// Shift memory at the saved indexed address left, inserting zero. Set carry from the outgoing
// bit and flags from the result. (UM0050 pp. 216-220).
void xycb_26();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to A. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_27();
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to B. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_28();
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to C. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_29();
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to D. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_2a();
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to E. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_2b();
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to H. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_2c();
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to L. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_2d();
// Shift memory at the saved indexed address right, retaining its sign bit. Set carry from the
// outgoing bit and flags from the result. (UM0050 pp. 216-220).
void xycb_2e();
// Retained Z80 helper: Shift memory at the saved indexed address right, retaining its sign bit.
// Set carry from the outgoing bit and flags from the result. Also copy the result to A. The
// Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_2f();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to B. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_30();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to C. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_31();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to D. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_32();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to E. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_33();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to H. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_34();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to L. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_35();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. The Z180 decoder traps this encoding (UM0050
// pp. 68-72, 247-250).
void xycb_36();
// Retained Z80 helper: Shift memory at the saved indexed address left, inserting one. Set carry
// from the outgoing bit and flags from the result. Also copy the result to A. The Z180 decoder
// traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_37();
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to B. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_38();
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to C. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_39();
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to D. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_3a();
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to E. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_3b();
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to H. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_3c();
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to L. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_3d();
// Shift memory at the saved indexed address right, inserting zero. Set carry from the outgoing
// bit and flags from the result. (UM0050 pp. 216-220).
void xycb_3e();
// Retained Z80 helper: Shift memory at the saved indexed address right, inserting zero. Set
// carry from the outgoing bit and flags from the result. Also copy the result to A. The Z180
// decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_3f();
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_40();
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_41();
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_42();
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_43();
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_44();
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_45();
// Test bit 0 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
void xycb_46();
// Retained Z80 helper: Test bit 0 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_47();
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_48();
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_49();
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_4a();
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_4b();
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_4c();
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_4d();
// Test bit 1 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
void xycb_4e();
// Retained Z80 helper: Test bit 1 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_4f();
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_50();
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_51();
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_52();
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_53();
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_54();
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_55();
// Test bit 2 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
void xycb_56();
// Retained Z80 helper: Test bit 2 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_57();
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_58();
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_59();
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_5a();
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_5b();
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_5c();
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_5d();
// Test bit 3 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
void xycb_5e();
// Retained Z80 helper: Test bit 3 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_5f();
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_60();
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_61();
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_62();
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_63();
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_64();
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_65();
// Test bit 4 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
void xycb_66();
// Retained Z80 helper: Test bit 4 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_67();
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_68();
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_69();
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_6a();
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_6b();
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_6c();
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_6d();
// Test bit 5 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
void xycb_6e();
// Retained Z80 helper: Test bit 5 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_6f();
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_70();
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_71();
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_72();
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_73();
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_74();
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_75();
// Test bit 6 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
void xycb_76();
// Retained Z80 helper: Test bit 6 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_77();
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_78();
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_79();
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_7a();
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_7b();
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_7c();
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_7d();
// Test bit 7 of memory at the saved indexed address in the flags; leave the operand unchanged.
// (UM0050 pp. 219-220).
void xycb_7e();
// Retained Z80 helper: Test bit 7 of memory at the saved indexed address in the flags; leave
// the operand unchanged. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_7f();
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_80();
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_81();
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_82();
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_83();
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_84();
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_85();
// Clear bit 0 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_86();
// Retained Z80 helper: Clear bit 0 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_87();
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_88();
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_89();
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_8a();
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_8b();
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_8c();
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_8d();
// Clear bit 1 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_8e();
// Retained Z80 helper: Clear bit 1 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_8f();
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_90();
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_91();
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_92();
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_93();
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_94();
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_95();
// Clear bit 2 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_96();
// Retained Z80 helper: Clear bit 2 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_97();
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_98();
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_99();
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_9a();
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_9b();
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_9c();
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_9d();
// Clear bit 3 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_9e();
// Retained Z80 helper: Clear bit 3 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_9f();
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_a0();
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_a1();
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_a2();
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_a3();
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_a4();
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_a5();
// Clear bit 4 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_a6();
// Retained Z80 helper: Clear bit 4 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_a7();
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_a8();
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_a9();
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_aa();
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_ab();
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_ac();
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_ad();
// Clear bit 5 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_ae();
// Retained Z80 helper: Clear bit 5 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_af();
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_b0();
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_b1();
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_b2();
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_b3();
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_b4();
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_b5();
// Clear bit 6 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_b6();
// Retained Z80 helper: Clear bit 6 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_b7();
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_b8();
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_b9();
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_ba();
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_bb();
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_bc();
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_bd();
// Clear bit 7 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_be();
// Retained Z80 helper: Clear bit 7 of memory at the saved indexed address; leave flags
// unchanged. Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp.
// 68-72, 247-250).
void xycb_bf();
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_c0();
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_c1();
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_c2();
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_c3();
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_c4();
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_c5();
// Set bit 0 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_c6();
// Retained Z80 helper: Set bit 0 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_c7();
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_c8();
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_c9();
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_ca();
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_cb();
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_cc();
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_cd();
// Set bit 1 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_ce();
// Retained Z80 helper: Set bit 1 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_cf();
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_d0();
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_d1();
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_d2();
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_d3();
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_d4();
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_d5();
// Set bit 2 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_d6();
// Retained Z80 helper: Set bit 2 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_d7();
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_d8();
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_d9();
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_da();
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_db();
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_dc();
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_dd();
// Set bit 3 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_de();
// Retained Z80 helper: Set bit 3 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_df();
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_e0();
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_e1();
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_e2();
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_e3();
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_e4();
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_e5();
// Set bit 4 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_e6();
// Retained Z80 helper: Set bit 4 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_e7();
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_e8();
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_e9();
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_ea();
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_eb();
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_ec();
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_ed();
// Set bit 5 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_ee();
// Retained Z80 helper: Set bit 5 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_ef();
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_f0();
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_f1();
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_f2();
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_f3();
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_f4();
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_f5();
// Set bit 6 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_f6();
// Retained Z80 helper: Set bit 6 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_f7();
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to B. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_f8();
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to C. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_f9();
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to D. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_fa();
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to E. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_fb();
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to H. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_fc();
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to L. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_fd();
// Set bit 7 of memory at the saved indexed address; leave flags unchanged. (UM0050 pp.
// 219-220).
void xycb_fe();
// Retained Z80 helper: Set bit 7 of memory at the saved indexed address; leave flags unchanged.
// Copy the changed byte to A. The Z180 decoder traps this encoding (UM0050 pp. 68-72, 247-250).
void xycb_ff();
