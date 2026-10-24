// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller

/*
 *   Instruction decoder and timing tables for Second Sight's Zilog Z8S180.
 *   Z180.cpp includes these tables and macros before the opcode bodies.
 *   The decoder validates an opcode, calls its handler, and returns nominal
 *   clock cost. The bus engine uses the same table to place internal clocks
 *   around memory/port cycles. Tables are immutable and shared; decoder
 *   methods use only their owning CPU's execution thread.
 *
 *   An opcode is an instruction byte. A prefix selects its family: op is
 *   unprefixed, cb bit operations, ed extensions, xy the IX/IY index-register
 *   families, and xycb indexed bit operations. PHI is the CPU clock; a T-state
 *   is one PHI. cc values count whole PHI, excluding inserted wait clocks.
 *   ex supplies conditional/repeat additions. Handler rows use dd/fd separately;
 *   timing rows share xy. A trap enters address zero for an undefined opcode.
 *
 *   Sources: Zilog UM005004-0918 pp. 237-246 (timing summary), pp. 247-250
 *   (valid opcodes), pp. 251-280 (bus sequence), pp. 68-72 (trap). The block-I/O
 *   parity tables retain inherited undocumented flag behavior; they do not
 *   represent additional registers.
 */

/* tmp1 value for ini/inir/outi/otir for [C.1-0][io.1-0] */
static const uint8_t irep_tmp1[4][4] = {{0, 0, 1, 0}, {0, 1, 0, 1}, {1, 0, 1, 1}, {0, 1, 1, 0}};

/* tmp1 value for ind/indr/outd/otdr for [C.1-0][io.1-0] */
static const uint8_t drep_tmp1[4][4] = {{0, 1, 0, 0}, {1, 0, 0, 1}, {0, 0, 1, 0}, {0, 1, 0, 1}};

/* tmp2 value for all in/out repeated opcodes for B.7-0 */
static const uint8_t breg_tmp2[256] = {
    0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1,
    0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0,
    1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1,
    1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1,
    0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1,
    0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1};

// Each 256-entry table is indexed by the final opcode byte. Zero means a
// prefix handler supplies the duration from another row, not a zero-time
// instruction. Low nibble runs across each logical group of sixteen entries.
static const uint8_t cc_op[0x100] = {
    /*-0 -1 -2 -3 -4 -5 -6 -7 -8 -9 -a -b -c -d -e -f */
    3, 9, 7,  4,  4, 4,  6, 3,  4, 7, 6,  4, 4, 4,  6, 3,  7, 9, 7,  4,  4,  4,  6, 3,  8, 7, 6,  4, 4, 4, 6, 3,
    6, 9, 16, 4,  4, 4,  6, 4,  6, 7, 15, 4, 4, 4,  6, 3,  6, 9, 13, 4,  10, 10, 9, 3,  6, 7, 12, 4, 4, 4, 6, 3,
    4, 4, 4,  4,  4, 4,  6, 4,  4, 4, 4,  4, 4, 4,  6, 4,  4, 4, 4,  4,  4,  4,  6, 4,  4, 4, 4,  4, 4, 4, 6, 4,
    4, 4, 4,  4,  4, 4,  6, 4,  4, 4, 4,  4, 4, 4,  6, 4,  7, 7, 7,  7,  7,  7,  3, 7,  4, 4, 4,  4, 4, 4, 6, 4,
    4, 4, 4,  4,  4, 4,  6, 4,  4, 4, 4,  4, 4, 4,  6, 4,  4, 4, 4,  4,  4,  4,  6, 4,  4, 4, 4,  4, 4, 4, 6, 4,
    4, 4, 4,  4,  4, 4,  6, 4,  4, 4, 4,  4, 4, 4,  6, 4,  4, 4, 4,  4,  4,  4,  6, 4,  4, 4, 4,  4, 4, 4, 6, 4,
    5, 9, 6,  9,  6, 11, 6, 11, 5, 9, 6,  0, 6, 16, 6, 11, 5, 9, 6,  10, 6,  11, 6, 11, 5, 3, 6,  9, 6, 0, 6, 11,
    5, 9, 6,  16, 6, 11, 6, 11, 5, 3, 6,  3, 6, 0,  6, 11, 5, 9, 6,  3,  6,  11, 6, 11, 5, 4, 6,  3, 6, 0, 6, 11};

// CB: rotate, shift and bit operations, including the prefix fetch.
static const uint8_t cc_cb[0x100] = {
    /*-0 -1 -2 -3 -4 -5 -6 -7 -8 -9 -a -b -c -d -e -f */
    7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7,
    7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7,
    6, 6, 6, 6, 6, 6, 9,  6, 6, 6, 6, 6, 6, 6, 9,  6, 6, 6, 6, 6, 6, 6, 9,  6, 6, 6, 6, 6, 6, 6, 9,  6,
    6, 6, 6, 6, 6, 6, 9,  6, 6, 6, 6, 6, 6, 6, 9,  6, 6, 6, 6, 6, 6, 6, 9,  6, 6, 6, 6, 6, 6, 6, 9,  6,
    7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7,
    7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7,
    7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7,
    7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7, 7, 7, 7, 7, 7, 7, 13, 7};

// ED: extended operations. Invalid encodings are intercepted by valid_ed.
static const uint8_t cc_ed[0x100] = {
    /*-0 -1 -2 -3 -4 -5 -6 -7 -8 -9 -a -b -c -d -e -f */
    12, 13, 6,  6,  7,  6,  6,  6,  12, 13, 6,  6,  7,  6,  6,  6,  12, 13, 6,  6,  7,  6,  6,  6,  12, 13, 6,  6,  7,
    6,  6,  6,  12, 13, 6,  6,  7,  6,  6,  6,  12, 13, 6,  6,  7,  6,  6,  6,  12, 13, 6,  6,  10, 6,  6,  6,  12, 13,
    6,  6,  7,  6,  6,  6,  9,  10, 10, 19, 6,  12, 6,  6,  9,  10, 10, 18, 17, 12, 6,  6,  9,  10, 10, 19, 6,  12, 6,
    6,  9,  10, 10, 18, 17, 12, 6,  6,  9,  10, 10, 19, 9,  12, 6,  16, 9,  10, 10, 18, 17, 12, 6,  16, 9,  10, 10, 19,
    12, 12, 8,  6,  9,  10, 10, 18, 17, 12, 6,  6,  6,  6,  6,  14, 6,  6,  6,  6,  6,  6,  6,  14, 6,  6,  6,  6,  6,
    6,  6,  14, 6,  6,  6,  6,  6,  6,  6,  14, 6,  6,  6,  6,  12, 12, 12, 12, 6,  6,  6,  6,  12, 12, 12, 12, 6,  6,
    6,  6,  12, 12, 12, 12, 6,  6,  6,  6,  12, 12, 12, 12, 6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6};

// DD/FD: shared timing for IX/IY instructions. A CB prefix delegates again.
static const uint8_t cc_xy[0x100] = {
    /*-0 -1 -2 -3 -4 -5 -6 -7 -8 -9 -a -b -c -d -e -f */
    4, 4,  4,  4,  4, 4,  4,  4, 4, 10, 4,  4, 4, 4, 4,  4, 4,  4,  4,  4,  4,  4,  4,  4,  4, 10, 4, 4, 4, 4, 4,  4,
    4, 12, 19, 7,  9, 9,  15, 4, 4, 10, 18, 7, 9, 9, 9,  4, 4,  4,  4,  4,  18, 18, 15, 4,  4, 10, 4, 4, 4, 4, 4,  4,
    4, 4,  4,  4,  9, 9,  14, 4, 4, 4,  4,  4, 9, 9, 14, 4, 4,  4,  4,  4,  9,  9,  14, 4,  4, 4,  4, 4, 9, 9, 14, 4,
    9, 9,  9,  9,  9, 9,  14, 9, 9, 9,  9,  9, 9, 9, 14, 9, 15, 15, 15, 15, 15, 15, 4,  15, 4, 4,  4, 4, 9, 9, 14, 4,
    4, 4,  4,  4,  9, 9,  14, 4, 4, 4,  4,  4, 9, 9, 14, 4, 4,  4,  4,  4,  9,  9,  14, 4,  4, 4,  4, 4, 9, 9, 14, 4,
    4, 4,  4,  4,  9, 9,  14, 4, 4, 4,  4,  4, 9, 9, 14, 4, 4,  4,  4,  4,  9,  9,  14, 4,  4, 4,  4, 4, 9, 9, 14, 4,
    4, 4,  4,  4,  4, 4,  4,  4, 4, 4,  4,  0, 4, 4, 4,  4, 4,  4,  4,  4,  4,  4,  4,  4,  4, 4,  4, 4, 4, 4, 4,  4,
    4, 12, 4,  19, 4, 14, 4,  4, 4, 6,  4,  4, 4, 4, 4,  4, 4,  4,  4,  4,  4,  4,  4,  4,  4, 7,  4, 4, 4, 4, 4,  4};

// Indexed CB: displacement and final opcode fetch are included in these totals.
static const uint8_t cc_xycb[0x100] = {
    /*-0 -1 -2 -3 -4 -5 -6 -7 -8 -9 -a -b -c -d -e -f */
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19};

/* extra cycles if jr/jp/call taken and 'interrupt latency' on rst 0-7 */
// Extra PHI for taken branches, repeated block steps and interrupt-related calls.
static const uint8_t cc_ex[0x100] = {
    /*-0 -1 -2 -3 -4 -5 -6 -7 -8 -9 -a -b -c -d -e -f */
    0, 0, 0, 0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 2, 0, 0, 0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, /* DJNZ */
    2, 0, 0, 0,  0,  0, 0, 0, 2, 0, 0, 0,  0,  0,  0, 0, /* JR NZ/JR Z */
    2, 0, 0, 0,  0,  0, 0, 0, 2, 0, 0, 0,  0,  0,  0, 0, /* JR NC/JR C */
    0, 0, 0, 0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  10, 0, 0, 0, 0, 0, 0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  10, 0, 0, 0,
    0, 0, 0, 0,  0,  0, 0, 0, 0, 0, 0, 0,  10, 0,  0, 0, 0, 0, 0, 0,  0,  0, 0, 0, 0, 0, 0, 0,  10, 0,  0, 0, 0,
    0, 0, 0, 0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0,
    0, 0, 0, 0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  2,  2, 2, 2, 0, 0, 0,  0,  2, 2, 2, 2, 0, 0, 0,  0, /* UM0050
                                                                                                      pp238,241-242:
                                                                                                      repeat adds two
                                                                                                      states */
    5, 0, 3, 0,  10, 0, 0, 2, 5, 0, 3, 0,  10, 0,  0, 2, 5, 0, 3, 0,  10, 0, 0, 2, 5, 0, 3, 0,  10, 0,  0, 2, 5,
    0, 3, 0, 10, 0,  0, 2, 5, 0, 3, 0, 10, 0,  0,  2, 5, 0, 3, 0, 10, 0,  0, 2, 5, 0, 3, 0, 10, 0,  0,  2};

static const uint8_t *const cc_default[6] = {cc_op, cc_cb, cc_ed, cc_xy, cc_xycb, cc_ex};

#define Z180_TABLE_dd Z180_TABLE_xy
#define Z180_TABLE_fd Z180_TABLE_xy

#define TABLE(prefix)                                                                                                  \
    {&Z180::prefix##_00, &Z180::prefix##_01, &Z180::prefix##_02, &Z180::prefix##_03, &Z180::prefix##_04,               \
     &Z180::prefix##_05, &Z180::prefix##_06, &Z180::prefix##_07, &Z180::prefix##_08, &Z180::prefix##_09,               \
     &Z180::prefix##_0a, &Z180::prefix##_0b, &Z180::prefix##_0c, &Z180::prefix##_0d, &Z180::prefix##_0e,               \
     &Z180::prefix##_0f, &Z180::prefix##_10, &Z180::prefix##_11, &Z180::prefix##_12, &Z180::prefix##_13,               \
     &Z180::prefix##_14, &Z180::prefix##_15, &Z180::prefix##_16, &Z180::prefix##_17, &Z180::prefix##_18,               \
     &Z180::prefix##_19, &Z180::prefix##_1a, &Z180::prefix##_1b, &Z180::prefix##_1c, &Z180::prefix##_1d,               \
     &Z180::prefix##_1e, &Z180::prefix##_1f, &Z180::prefix##_20, &Z180::prefix##_21, &Z180::prefix##_22,               \
     &Z180::prefix##_23, &Z180::prefix##_24, &Z180::prefix##_25, &Z180::prefix##_26, &Z180::prefix##_27,               \
     &Z180::prefix##_28, &Z180::prefix##_29, &Z180::prefix##_2a, &Z180::prefix##_2b, &Z180::prefix##_2c,               \
     &Z180::prefix##_2d, &Z180::prefix##_2e, &Z180::prefix##_2f, &Z180::prefix##_30, &Z180::prefix##_31,               \
     &Z180::prefix##_32, &Z180::prefix##_33, &Z180::prefix##_34, &Z180::prefix##_35, &Z180::prefix##_36,               \
     &Z180::prefix##_37, &Z180::prefix##_38, &Z180::prefix##_39, &Z180::prefix##_3a, &Z180::prefix##_3b,               \
     &Z180::prefix##_3c, &Z180::prefix##_3d, &Z180::prefix##_3e, &Z180::prefix##_3f, &Z180::prefix##_40,               \
     &Z180::prefix##_41, &Z180::prefix##_42, &Z180::prefix##_43, &Z180::prefix##_44, &Z180::prefix##_45,               \
     &Z180::prefix##_46, &Z180::prefix##_47, &Z180::prefix##_48, &Z180::prefix##_49, &Z180::prefix##_4a,               \
     &Z180::prefix##_4b, &Z180::prefix##_4c, &Z180::prefix##_4d, &Z180::prefix##_4e, &Z180::prefix##_4f,               \
     &Z180::prefix##_50, &Z180::prefix##_51, &Z180::prefix##_52, &Z180::prefix##_53, &Z180::prefix##_54,               \
     &Z180::prefix##_55, &Z180::prefix##_56, &Z180::prefix##_57, &Z180::prefix##_58, &Z180::prefix##_59,               \
     &Z180::prefix##_5a, &Z180::prefix##_5b, &Z180::prefix##_5c, &Z180::prefix##_5d, &Z180::prefix##_5e,               \
     &Z180::prefix##_5f, &Z180::prefix##_60, &Z180::prefix##_61, &Z180::prefix##_62, &Z180::prefix##_63,               \
     &Z180::prefix##_64, &Z180::prefix##_65, &Z180::prefix##_66, &Z180::prefix##_67, &Z180::prefix##_68,               \
     &Z180::prefix##_69, &Z180::prefix##_6a, &Z180::prefix##_6b, &Z180::prefix##_6c, &Z180::prefix##_6d,               \
     &Z180::prefix##_6e, &Z180::prefix##_6f, &Z180::prefix##_70, &Z180::prefix##_71, &Z180::prefix##_72,               \
     &Z180::prefix##_73, &Z180::prefix##_74, &Z180::prefix##_75, &Z180::prefix##_76, &Z180::prefix##_77,               \
     &Z180::prefix##_78, &Z180::prefix##_79, &Z180::prefix##_7a, &Z180::prefix##_7b, &Z180::prefix##_7c,               \
     &Z180::prefix##_7d, &Z180::prefix##_7e, &Z180::prefix##_7f, &Z180::prefix##_80, &Z180::prefix##_81,               \
     &Z180::prefix##_82, &Z180::prefix##_83, &Z180::prefix##_84, &Z180::prefix##_85, &Z180::prefix##_86,               \
     &Z180::prefix##_87, &Z180::prefix##_88, &Z180::prefix##_89, &Z180::prefix##_8a, &Z180::prefix##_8b,               \
     &Z180::prefix##_8c, &Z180::prefix##_8d, &Z180::prefix##_8e, &Z180::prefix##_8f, &Z180::prefix##_90,               \
     &Z180::prefix##_91, &Z180::prefix##_92, &Z180::prefix##_93, &Z180::prefix##_94, &Z180::prefix##_95,               \
     &Z180::prefix##_96, &Z180::prefix##_97, &Z180::prefix##_98, &Z180::prefix##_99, &Z180::prefix##_9a,               \
     &Z180::prefix##_9b, &Z180::prefix##_9c, &Z180::prefix##_9d, &Z180::prefix##_9e, &Z180::prefix##_9f,               \
     &Z180::prefix##_a0, &Z180::prefix##_a1, &Z180::prefix##_a2, &Z180::prefix##_a3, &Z180::prefix##_a4,               \
     &Z180::prefix##_a5, &Z180::prefix##_a6, &Z180::prefix##_a7, &Z180::prefix##_a8, &Z180::prefix##_a9,               \
     &Z180::prefix##_aa, &Z180::prefix##_ab, &Z180::prefix##_ac, &Z180::prefix##_ad, &Z180::prefix##_ae,               \
     &Z180::prefix##_af, &Z180::prefix##_b0, &Z180::prefix##_b1, &Z180::prefix##_b2, &Z180::prefix##_b3,               \
     &Z180::prefix##_b4, &Z180::prefix##_b5, &Z180::prefix##_b6, &Z180::prefix##_b7, &Z180::prefix##_b8,               \
     &Z180::prefix##_b9, &Z180::prefix##_ba, &Z180::prefix##_bb, &Z180::prefix##_bc, &Z180::prefix##_bd,               \
     &Z180::prefix##_be, &Z180::prefix##_bf, &Z180::prefix##_c0, &Z180::prefix##_c1, &Z180::prefix##_c2,               \
     &Z180::prefix##_c3, &Z180::prefix##_c4, &Z180::prefix##_c5, &Z180::prefix##_c6, &Z180::prefix##_c7,               \
     &Z180::prefix##_c8, &Z180::prefix##_c9, &Z180::prefix##_ca, &Z180::prefix##_cb, &Z180::prefix##_cc,               \
     &Z180::prefix##_cd, &Z180::prefix##_ce, &Z180::prefix##_cf, &Z180::prefix##_d0, &Z180::prefix##_d1,               \
     &Z180::prefix##_d2, &Z180::prefix##_d3, &Z180::prefix##_d4, &Z180::prefix##_d5, &Z180::prefix##_d6,               \
     &Z180::prefix##_d7, &Z180::prefix##_d8, &Z180::prefix##_d9, &Z180::prefix##_da, &Z180::prefix##_db,               \
     &Z180::prefix##_dc, &Z180::prefix##_dd, &Z180::prefix##_de, &Z180::prefix##_df, &Z180::prefix##_e0,               \
     &Z180::prefix##_e1, &Z180::prefix##_e2, &Z180::prefix##_e3, &Z180::prefix##_e4, &Z180::prefix##_e5,               \
     &Z180::prefix##_e6, &Z180::prefix##_e7, &Z180::prefix##_e8, &Z180::prefix##_e9, &Z180::prefix##_ea,               \
     &Z180::prefix##_eb, &Z180::prefix##_ec, &Z180::prefix##_ed, &Z180::prefix##_ee, &Z180::prefix##_ef,               \
     &Z180::prefix##_f0, &Z180::prefix##_f1, &Z180::prefix##_f2, &Z180::prefix##_f3, &Z180::prefix##_f4,               \
     &Z180::prefix##_f5, &Z180::prefix##_f6, &Z180::prefix##_f7, &Z180::prefix##_f8, &Z180::prefix##_f9,               \
     &Z180::prefix##_fa, &Z180::prefix##_fb, &Z180::prefix##_fc, &Z180::prefix##_fd, &Z180::prefix##_fe,               \
     &Z180::prefix##_ff}

const Z180::opcode_func Z180::opcodes[Z180_PREFIX_COUNT][0x100] = {TABLE(op), TABLE(cb), TABLE(dd),
                                                                   TABLE(ed), TABLE(fd), TABLE(xycb)};

/***************************************************************
 * define an opcode function
 ***************************************************************/
#define OP(prefix, opcode_arg) inline void Z180::prefix##_##opcode_arg()

/***************************************************************
 * adjust cycle count by n T-states
 ***************************************************************/
#define CC(prefix, opcode_arg) extra_cycles += cc[Z180_TABLE_##prefix][opcode_arg]

/***************************************************************
 * execute an opcode
 ***************************************************************/

// Defined opcode map, UM0050 Tables 48-50 (pp248-250). Z80 aliases,
// SLL, IXH/IXL/IYH/IYL and indexed-CB register destinations trap on Z180.
// Sixteen bitsets: high opcode nibble selects the word, low nibble selects
// its bit. One permits execution, zero enters the undefined-instruction trap.
static constexpr uint16_t valid_ed[16] = {0x1313, 0x1313, 0x1313, 0x1311, 0xbfff, 0xdfcf, 0x9f9f, 0x1f5d,
                                          0x0808, 0x0808, 0x0f0f, 0x0f0f, 0,      0,      0,      0};
static constexpr uint16_t valid_index[16] = {0x0200, 0x0200, 0x0e0e, 0x0270, 0x4040, 0x4040, 0x4040, 0x40bf,
                                             0x4040, 0x4040, 0x4040, 0x4040, 0x0800, 0,      0x022a, 0x0200};

#include "z180dispatch.hxx"

// Define a decoder returning nominal PHI for opcode_arg. valid is the
// silicon validity rule; third selects the indexed third-byte trap sequence.
// Both dispatch mechanisms call the same handler and use the same timing row.
#define EXEC_PROTOTYPE(prefix, valid, third)                                                                           \
    int Z180::exec##_##prefix(const uint8_t opcode_arg) {                                                              \
        if (cycle_executing)                                                                                           \
            cycle_decode(Z180_TABLE_##prefix, opcode_arg);                                                             \
        if (!(valid))                                                                                                  \
            return take_trap(third);                                                                                   \
        if (cycle_executing && !cycle_reference) {                                                                     \
            Z180_DIRECT_DISPATCH(prefix)                                                                               \
        } else                                                                                                         \
            (this->*opcodes[Z180_PREFIX_##prefix][opcode_arg])();                                                      \
        return cc[Z180_TABLE_##prefix][opcode_arg];                                                                    \
    }

// op: validate this family, execute it, and return its nominal timing.
EXEC_PROTOTYPE(op, true, false)
// cb: validate this family, execute it, and return its nominal timing.
EXEC_PROTOTYPE(cb, (opcode_arg & 0xf8) != 0x30, false)
// dd: validate this family, execute it, and return its nominal timing.
EXEC_PROTOTYPE(dd, (valid_index[opcode_arg >> 4] >> (opcode_arg & 15)) & 1, false)
// ed: validate this family, execute it, and return its nominal timing.
EXEC_PROTOTYPE(ed, (valid_ed[opcode_arg >> 4] >> (opcode_arg & 15)) & 1, false)
// fd: validate this family, execute it, and return its nominal timing.
EXEC_PROTOTYPE(fd, (valid_index[opcode_arg >> 4] >> (opcode_arg & 15)) & 1, false)
// xycb: validate this family, execute it, and return its nominal timing.
EXEC_PROTOTYPE(xycb, (opcode_arg & 7) == 6 && opcode_arg != 0x36, true)
