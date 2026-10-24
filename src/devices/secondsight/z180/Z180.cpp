// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller

/*
 *   The Zilog Z8S180 is Second Sight's own processor. It runs the card ROM
 *   and copies Apple video bytes into the Oak controller's video memory.
 *   SSCard supplies memory, port and pin callbacks. This file connects the
 *   instruction handlers to the on-chip timers, serial ports and bus scheduler.
 *   SSCard runs the CPU on its worker while a job is outstanding. Without
 *   a worker, including browser builds, the caller executes it directly.
 *   Callbacks run on that same thread and must not enter execution recursively.
 *
 *   PHI is the processor clock. One T-state is one PHI period; a half-PHI is
 *   one rising-to-falling or falling-to-rising interval. A machine cycle is a
 *   bus operation or internal step made of T-states. T1, T2 and T3 name its
 *   successive clocks; TW is an inserted wait clock and Ti an internal clock.
 *   M1 identifies an instruction-byte fetch. IORQ selects an input/output port.
 *   WAIT lengthens a transfer. BUSREQ asks the processor to release its bus;
 *   BUSACK confirms that it has done so. These names denote asserted signals,
 *   regardless of whether the electrical pin is active low.
 *
 *   The MMU (memory management unit) maps a 16-bit program address to a
 *   20-bit physical address. DMA (direct memory access) has two channels,
 *   each a source, destination and byte counter that can copy without an
 *   instruction for each byte. DREQ is a channel's external request input.
 *   INT0-2 are maskable interrupt inputs; NMI is a non-maskable interrupt.
 *   PRT means programmable reload timer. FRC is the free-running counter
 *   used to divide PHI. ASCI is asynchronous serial communication; CSI/O
 *   is serial communication with a separate clock wire.
 *
 *   Sources: Zilog UM005004-0918 (UM0050), printed pp. 15-30 for bus timing,
 *   pp. 55-64 for the MMU, pp. 65-115 for interrupts, refresh and DMA,
 *   pp. 116-164 for serial ports and timers, and pp. 251-280 for instruction
 *   bus sequences: https://www.zilog.com/docs/z180/um0050.pdf.
 *   The continuation journal and fast paths are emulator mechanisms, not
 *   extra processor registers. K/N revision differences use Zilog TN0053
 *   and the Z8S180 data sheets cited at the affected operation.
 */

#include "Z180.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <mutex>
#include <stdexcept>

// Illegal-opcode diagnostics evaluate bus reads even when logging is disabled.
template <class... Args> static void z180_diagnostic(const char *, Args...) {}

// Format an invariant failure for SSCard to retain and report after the worker
// stops. Arguments have printf meanings; this never returns to a bus operation.
template <class... Args> [[noreturn]] static void z180_error(const char *format, Args... args) {
    char message[256];
    std::snprintf(message, sizeof(message), format, args...);
    throw std::runtime_error(message);
}
// Internal priority order (Zilog UM0050, Interrupts); these are model indices.
#define Z180_INT_TRAP 0   /* Undefined opcode_arg */
#define Z180_INT_NMI 1    /* NMI */
#define Z180_INT_IRQ0 2   /* Execute IRQ0 */
#define Z180_INT_IRQ1 3   /* Execute IRQ1 */
#define Z180_INT_IRQ2 4   /* Execute IRQ2 */
#define Z180_INT_PRT0 5   /* Internal PRT channel 0 */
#define Z180_INT_PRT1 6   /* Internal PRT channel 1 */
#define Z180_INT_DMA0 7   /* Internal DMA channel 0 */
#define Z180_INT_DMA1 8   /* Internal DMA channel 1 */
#define Z180_INT_CSIO 9   /* Internal CSI/O */
#define Z180_INT_ASCI0 10 /* Internal ASCI channel 0 */
#define Z180_INT_ASCI1 11 /* Internal ASCI channel 1 */
#define Z180_INT_MAX Z180_INT_ASCI1

// F is the arithmetic-status byte (UM0050 pp. 177-179). S is the sign bit,
// Z means zero, H means a carry/borrow across bit 3, N marks subtraction,
// and C means carry/borrow across bit 7. P/V is parity for logical operations
// or signed overflow for arithmetic. X and Y copy result bits 3 and 5; these
// undocumented bits retain the inherited core's behavior.
#define CF 0x01
#define NF 0x02
#define PF 0x04
#define VF PF
#define XF 0x08
#define HF 0x10
#define YF 0x20
#define ZF 0x40
#define SF 0x80

/* I/O line status flags */
#define Z180_CKA0 0x00000001     /* I/O asynchronous clock 0 (active high) or DREQ0 (mux) */
#define Z180_CKA1 0x00000002     /* I/O asynchronous clock 1 (active high) or TEND1 (mux) */
#define Z180_CKS 0x00000004      /* I/O serial clock (active high) */
#define Z180_CTS0 0x00000100     /* I   clear to send 0 (active low) */
#define Z180_CTS1 0x00000200     /* I   clear to send 1 (active low) or RXS (mux) */
#define Z180_DCD0 0x00000400     /* I   data carrier detect (active low) */
#define Z180_DREQ0 0x00000800    /* I   data request DMA ch 0 (active low) or CKA0 (mux) */
#define Z180_DREQ1 0x00001000    /* I   data request DMA ch 1 (active low) */
#define Z180_RXA0 0x00002000     /* I   asynchronous receive data 0 (active high) */
#define Z180_RXA1 0x00004000     /* I   asynchronous receive data 1 (active high) */
#define Z180_RXS 0x00008000      /* I   clocked serial receive data (active high) or CTS1 (mux) */
#define Z180_RTS0 0x00010000     /*   O request to send (active low) */
#define Z180_TEND0 0x00020000    /*   O transfer end 0 (active low) or CKA1 (mux) */
#define Z180_TEND1 0x00040000    /*   O transfer end 1 (active low) */
#define Z180_A18_TOUT 0x00080000 /*   O transfer out (PRT channel, active low) or A18 (mux) */
#define Z180_TXA0 0x00100000     /*   O asynchronous transmit data 0 (active high) */
#define Z180_TXA1 0x00200000     /*   O asynchronous transmit data 1 (active high) */
#define Z180_TXS 0x00400000      /*   O clocked serial transmit data (active high) */

/*
 * NetBSD headers can define these short register names as macros.
 */
#undef b8
#undef c8
#undef l8

#define previous_pc PREPC.d /* previous program counter */

#define pc32 PC.d
#define pc16 PC.w.l

#define sp32 SP.d
#define sp16 SP.w.l

#define af32 AF.d
#define af16 AF.w.l
#define a8 AF.b.h
#define f8 AF.b.l

#define bc32 BC.d
#define bc16 BC.w.l
#define b8 BC.b.h
#define c8 BC.b.l

#define de32 DE.d
#define de16 DE.w.l
#define d8 DE.b.h
#define e8 DE.b.l

#define hl32 HL.d
#define hl16 HL.w.l
#define h8 HL.b.h
#define l8 HL.b.l

#define ix32 IX.d
#define ix16 IX.w.l
#define ixh IX.b.h
#define ixl IX.b.l

#define iy32 IY.d
#define iy16 IY.w.l
#define iyh IY.b.h
#define iyl IY.b.l

// Zilog UM0050 pp. 157-164: PRT registers and flag-clearing sequence.
/* 10 TIMER control register (TIF1 and TIF0 are read-only) */
// TIF0/1: timeout occurred; TIE0/1: enable timeout interrupt; TDE0/1:
// enable counting. TOC1:0 select A18 or timer output and its toggle rate.
#define Z180_TCR_TIF1 0x80
#define Z180_TCR_TIF0 0x40
#define Z180_TCR_TIE1 0x20
#define Z180_TCR_TIE0 0x10
#define Z180_TCR_TOC1 0x08
#define Z180_TCR_TOC0 0x04
#define Z180_TCR_TDE1 0x02
#define Z180_TCR_TDE0 0x01

/* 1e (Z8S180/Z8L180) clock multiplier */
// X2 enables the N-revision multiplier. LOW_NOISE changes output drive,
// which is stored but has no analogue electrical effect here (UM0050 pp. 49-52).
#define Z180_CMR_X2 0x80
#define Z180_CMR_LOW_NOISE 0x40

#define Z180_CMR_MASK 0xc0

/* 1f (Z8S180/Z8L180) chip control register */
// CLOCK_DIVIDE=1 bypasses divide-by-two. STDBY_IDLE1:0 choose the low-power
// mode; BREXT permits BUSREQ wake-up. LNPHI, LNIO, LNCPU_CTL and LNAD_DATA
// select reduced drive for clock, I/O, CPU-control and address/data pins.
// UM0050 pp. 37-41, 49-52; the K revision reserves STDBY_IDLE0.
#define Z180_CCR_CLOCK_DIVIDE 0x80
#define Z180_CCR_STDBY_IDLE1 0x40
#define Z180_CCR_BREXT 0x20
#define Z180_CCR_LNPHI 0x10
#define Z180_CCR_STDBY_IDLE0 0x08
#define Z180_CCR_LNIO 0x04
#define Z180_CCR_LNCPU_CTL 0x02
#define Z180_CCR_LNAD_DATA 0x01

// Zilog UM0050 pp. 95-109: DMA register fields and request modes.
/* 20-22 DMA source address register ch 0 L, H, B */
#define Z180_SAR0_MASK 0x0fffff

/* 23-25 DMA destination address register ch 0 L, H, B */
#define Z180_DAR0_MASK 0x0fffff

/* 28-2a DMA memory address register ch 1 L, H, B */
#define Z180_MAR1_MASK 0x0fffff

/* 2b-2d DMA I/O address register ch 1 L, H, (Z8S180/Z8L180) B */
// IAR1 high-byte fields: ATF is the automatic-transfer flag; ATC controls
// automatic transfer; TOUT_DREQ selects the request source; SS selects
// external I/O or an ASCI channel. These extension fields are retained even
// where no Second Sight connection uses them (Z8S180 DS006002, pp. 56-57).
#define Z180_IAR1_ATF 0x800000
#define Z180_IAR1_ATC 0x400000
#define Z180_IAR1_TOUT_DREQ 0x080000
#define Z180_IAR1_SS 0x070000

#define Z180_IAR1_MASK 0xcfffff

/* 30 DMA status register (DWE1 and DWE0 are write-only, DME is read-only) */
#define Z180_DSTAT_DE1 0x80  /* DMA enable ch 1 */
#define Z180_DSTAT_DE0 0x40  /* DMA enable ch 0 */
#define Z180_DSTAT_DWE1 0x20 /* DMA write enable ch 1 (active low) */
#define Z180_DSTAT_DWE0 0x10 /* DMA write enable ch 0 (active low) */
#define Z180_DSTAT_DIE1 0x08 /* DMA IRQ enable ch 1 */
#define Z180_DSTAT_DIE0 0x04 /* DMA IRQ enable ch 0 */
#define Z180_DSTAT_DME 0x01  /* DMA enable (read only) */

#define Z180_DSTAT_MASK 0xfd

/* 31 DMA mode register */
#define Z180_DMODE_DM 0x30   /* DMA ch 0 destination addressing mode */
#define Z180_DMODE_SM 0x0c   /* DMA ch 0 source addressing mode */
#define Z180_DMODE_MMOD 0x02 /* DMA cycle steal/burst mode select */

#define Z180_DMODE_MASK 0x3e

/* 32 DMA/WAIT control register */
// MWI1:0 insert 0-3 memory waits. IWI1:0 select 1-4 external I/O waits.
// DMS0/1 select edge (1) or level (0) DREQ sensing for each channel.
// DIM1 selects DMA1 I/O-to-memory (1) or memory-to-I/O (0); DIM0 chooses
// decreasing (1) or increasing (0) memory addresses (UM0050 pp. 29-30, 101-103).
#define Z180_DCNTL_MWI1 0x80
#define Z180_DCNTL_MWI0 0x40
#define Z180_DCNTL_IWI1 0x20
#define Z180_DCNTL_IWI0 0x10
#define Z180_DCNTL_DMS1 0x08
#define Z180_DCNTL_DMS0 0x04
#define Z180_DCNTL_DIM1 0x02
#define Z180_DCNTL_DIM0 0x01

/* 33 INT vector low register */
#define Z180_IL_IL 0xe0

#define Z180_IL_MASK 0xe0

/* 34 INT/TRAP control register (UFO is read-only) */
// TRAP records an undefined instruction. UFO distinguishes a third-byte
// undefined opcode from a second-byte one. ITE0-2 enable INT0-2 separately
// from the CPU interrupt-enable flag (UM0050 pp. 69-72).
#define Z180_ITC_TRAP 0x80
#define Z180_ITC_UFO 0x40
#define Z180_ITC_ITE2 0x04
#define Z180_ITC_ITE1 0x02
#define Z180_ITC_ITE0 0x01

#define Z180_ITC_MASK 0xc7

// Zilog UM0050, Refresh Control: enable, extra wait and interval select.
/* 36 refresh control register */
// REFE enables dynamic-RAM refresh; REFW adds a refresh wait; CYC selects
// the interval. Second Sight SRAM needs no refresh (UM0050 pp. 87-90).
#define Z180_RCR_REFE 0x80
#define Z180_RCR_REFW 0x40
#define Z180_RCR_CYC 0x03

#define Z180_RCR_MASK 0xc3

// Zilog UM0050, Memory Management Unit: CBAR splits common/banked areas.
/* 3a MMU common/bank area register */
// CA and BA are 4-KiB boundaries: common area 1 starts at CA and the
// movable bank starts at BA. Common area 0 stays untranslated (pp. 58-64).
#define Z180_CBAR_CA 0xf0
#define Z180_CBAR_BA 0x0f

/* 3e operation mode control register (M1TE is write-only) */
// M1E selects M1-compatible timing; M1TE temporarily enables M1 output.
// IOC selects the IORQ/RD/WR timing convention (UM0050 pp. 15-17).
#define Z180_OMCR_M1E 0x80
#define Z180_OMCR_M1TE 0x40
#define Z180_OMCR_IOC 0x20

#define Z180_OMCR_MASK 0xe0

/* 3f I/O control register */
// IOSTP stops the internal I/O clocks. IOCR bits 7:6 relocate the 64-byte
// internal register window; port high bits must be zero (UM0050 pp. 42-48).
#define Z180_IOCR_IOSTP 0x20

/***************************************************************************
    CPU PREFIXES

    Indices select the handler rows in z180tbl.hpp: unprefixed, CB bit
    operations, DD/FD index-register operations, ED extensions, and DD/FD-CB.
***************************************************************************/

#define Z180_PREFIX_op 0
#define Z180_PREFIX_cb 1
#define Z180_PREFIX_dd 2
#define Z180_PREFIX_ed 3
#define Z180_PREFIX_fd 4
#define Z180_PREFIX_xycb 5

#define Z180_PREFIX_COUNT (Z180_PREFIX_xycb + 1)

static uint8_t SZ[256];       /* zero and sign flags */
static uint8_t SZ_BIT[256];   /* zero, sign and parity/overflow (=zero) flags for BIT opcode_arg */
static uint8_t SZP[256];      /* zero, sign and parity flags */
static uint8_t SZHV_inc[256]; /* zero, sign, half carry and overflow flags INC r8 */
static uint8_t SZHV_dec[256]; /* zero, sign, half carry and overflow flags DEC r8 */

// Derive ADD/ADC flags from the input/result pair without a large lookup table.
static uint8_t add_flags(uint8_t before, uint8_t result, unsigned carry = 0) {
    const int operand = int(result) - before - carry;
    return SZ[result] | (((result & 15) < (before & 15) + carry) ? HF : 0) |
           ((unsigned(result) < unsigned(before) + carry) ? CF : 0) |
           (((operand ^ before ^ 0x80) & (operand ^ result) & 0x80) ? VF : 0);
}

// Derive SUB/SBC flags with the same nibble carry and signed-overflow rules.
static uint8_t sub_flags(uint8_t before, uint8_t result, unsigned carry = 0) {
    const int operand = int(before) - result - carry;
    return SZ[result] | NF | (((result & 15) + carry > (before & 15)) ? HF : 0) |
           ((unsigned(result) + carry > before) ? CF : 0) |
           (((operand ^ before) & (before ^ result) & 0x80) ? VF : 0);
}

#include "z180ops.hpp"
#include "z180tbl.hpp"

#include "z180cb.hxx"
#include "z180xy.hxx"
#include "z180dd.hxx"
#include "z180fd.hxx"
#include "z180ed.hxx"
#include "z180op.hxx"

// Build byte-result tables for sign, zero, parity and INC/DEC flag effects.
static void init_flags() {
    int i, p;
    for (i = 0; i < 256; i++) {
        p = 0;
        if (i & 0x01)
            ++p;
        if (i & 0x02)
            ++p;
        if (i & 0x04)
            ++p;
        if (i & 0x08)
            ++p;
        if (i & 0x10)
            ++p;
        if (i & 0x20)
            ++p;
        if (i & 0x40)
            ++p;
        if (i & 0x80)
            ++p;
        SZ[i] = i ? i & SF : ZF;
        SZ[i] |= (i & (YF | XF)); /* undocumented flag bits 5+3 */
        SZ_BIT[i] = i ? i & SF : ZF | PF;
        SZ_BIT[i] |= (i & (YF | XF)); /* undocumented flag bits 5+3 */
        SZP[i] = SZ[i] | ((p & 1) ? 0 : PF);
        SZHV_inc[i] = SZ[i];
        if (i == 0x80)
            SZHV_inc[i] |= VF;
        if ((i & 0x0f) == 0x00)
            SZHV_inc[i] |= HF;
        SZHV_dec[i] = SZ[i] | NF;
        if (i == 0x7f)
            SZHV_dec[i] |= VF;
        if ((i & 0x0f) == 0x0f)
            SZHV_dec[i] |= HF;
    }
}
// Select internal I/O after IOCR relocation; the low six port bits identify
// the register (Zilog UM0050, internal I/O map and IOCR).
uint8_t Z180::z180_readcontrol(uint32_t port) {
    instruction_fast = false;
    // normal external readport (ignore the data)
    (void)read_io(port);

    // read the internal register
    return z180_internal_port_read(port & (0x3f));
}

// Read peripheral registers with their latch/acknowledgement side effects.
// Reserved bits and debugger peeks must not create interrupt acknowledgements.
// Offsets within the relocated internal-port window (UM0050 pp. 293-302):
// 00/01 CNTLA: serial control A; 02/03 CNTLB: serial control B;
// 04/05 STAT: serial status; 06/07 TDR: transmit byte; 08/09 RDR: receive byte;
// 0A CNTR: clocked-serial control/status; 0B TRDR: its shared data byte;
// 0C/0D TMDR0: timer 0 low/high count; 0E/0F RLDR0: its low/high reload;
// 10 TCR: timer control; 12/13 ASEXT: enhanced serial extension controls;
// 14/15 TMDR1 and 16/17 RLDR1: timer 1 count/reload low/high bytes;
// 18 FRC: free-running counter; 1A-1D ASTC: enhanced serial time constants;
// 1E CMR: clock multiplier; 1F CCR: clock/power/drive control;
// 20-22 SAR0: DMA0 source low/high/bank; 23-25 DAR0: destination low/high/bank;
// 26/27 BCR0: DMA0 count low/high; 28-2A MAR1: DMA1 memory low/high/bank;
// 2B-2D IAR1: DMA1 port low/high/extensions; 2E/2F BCR1: count low/high;
// 30 DSTAT: DMA status/enables; 31 DMODE: DMA0 addressing/burst modes;
// 32 DCNTL: wait insertion and DMA1/request modes; 33 IL: vector low bits;
// 34 ITC: interrupt/trap control; 36 RCR: refresh control;
// 38 CBR: MMU common offset; 39 BBR: bank offset; 3A CBAR: area boundaries;
// 3E OMCR: bus operation mode; 3F IOCR: port-window base and I/O clock stop.
// Other offsets read 0xff and ignore writes. Masks above describe bit meanings.
uint8_t Z180::z180_internal_port_read(uint8_t port) {
    uint8_t data = 0xff;

    // TN0053 pp1-2: these enhancements are absent from original K silicon.
    if (silicon == Silicon::K && (port == 0x12 || port == 0x13 || (port >= 0x1a && port <= 0x1e)))
        return 0xff;
    switch (port) {
    case 0x12:
    case 0x13:
        data = asci[port & 1].asext_r();
        break;

    case 0x1a:
    case 0x1c:
        data = asci[(port - 0x1a) >> 1].astcl_r();
        break;

    case 0x1b:
    case 0x1d:
        data = asci[(port - 0x1a) >> 1].astch_r();
        break;

    case 0x1e:
        data = cmr | ~Z180_CMR_MASK;

        break;

    case 0x1f:
        data = ccr;

        break;

    case 0x00:
    case 0x01:
        data = asci[port & 1].cntla_r();
        break;

    case 0x02:
    case 0x03:
        data = asci[port & 1].cntlb_r();
        break;

    case 0x04:
    case 0x05:
        data = asci[port & 1].stat_r();
        break;

    case 0x06:
    case 0x07:
        data = asci[port & 1].tdr_r();
        break;

    case 0x08:
    case 0x09:
        data = asci[port & 1].rdr_r();
        break;

    case 0x0a:
        data = csio.cntr_r();
        break;

    case 0x0b:
        data = csio.trdr_r();
        break;

    case 0x0c:
        data = tmdr_value[0] & 0x00ff;

        /* Hold the high byte while reading low so a later high read cannot tear across a decrement (p. 159). */
        if (tcr & Z180_TCR_TDE0) {
            tmdr_latch |= 1;
            tmdrh[0] = (tmdr_value[0] & 0xff00) >> 8;
        }

        // UM0050 p162: acknowledge by reading both TCR and TMDR.
        if (read_tcr_tmdr[0] & 1) {
            tcr &= ~Z180_TCR_TIF0;
            read_tcr_tmdr[0] = 0;
        } else
            read_tcr_tmdr[0] = 2;
        break;

    case 0x0d:
        /* Consume a previously held high byte; otherwise read the live counter (p. 159). */
        if (tmdr_latch & 1) {
            tmdr_latch &= ~1;
            data = tmdrh[0];
        } else {
            data = (tmdr_value[0] & 0xff00) >> 8;
        }

        // UM0050 p162: acknowledge by reading both TCR and TMDR.
        if (read_tcr_tmdr[0] & 1) {
            tcr &= ~Z180_TCR_TIF0;
            read_tcr_tmdr[0] = 0;
        } else
            read_tcr_tmdr[0] = 2;

        break;

    case 0x0e:
        data = rldr[0].b.l;

        break;

    case 0x0f:
        data = rldr[0].b.h;

        break;

    case 0x10:
        data = tcr;

        // UM0050 p162: acknowledge by reading both TCR and TMDR.
        if (read_tcr_tmdr[0] & 2) {
            tcr &= ~Z180_TCR_TIF0;
            read_tcr_tmdr[0] = 0;
        } else
            read_tcr_tmdr[0] = 1;

        // UM0050 p162: acknowledge by reading both TCR and TMDR.
        if (read_tcr_tmdr[1] & 2) {
            tcr &= ~Z180_TCR_TIF1;
            read_tcr_tmdr[1] = 0;
        } else
            read_tcr_tmdr[1] = 1;

        break;

    case 0x14:
        data = tmdr_value[1];

        /* Hold the high byte while reading low so a later high read cannot tear across a decrement (p. 159). */
        if (tcr & Z180_TCR_TDE1) {
            tmdr_latch |= 2;
            tmdrh[1] = (tmdr_value[1] & 0xff00) >> 8;
        }

        // UM0050 p162: acknowledge by reading both TCR and TMDR.
        if (read_tcr_tmdr[1] & 1) {
            tcr &= ~Z180_TCR_TIF1;
            read_tcr_tmdr[1] = 0;
        } else
            read_tcr_tmdr[1] = 2;
        break;

    case 0x15:
        /* Consume a previously held high byte; otherwise read the live counter (p. 159). */
        if (tmdr_latch & 2) {
            tmdr_latch &= ~2;
            data = tmdrh[1];
        } else {
            data = (tmdr_value[1] & 0xff00) >> 8;
        }

        // UM0050 p162: acknowledge by reading both TCR and TMDR.
        if (read_tcr_tmdr[1] & 1) {
            tcr &= ~Z180_TCR_TIF1;
            read_tcr_tmdr[1] = 0;
        } else
            read_tcr_tmdr[1] = 2;

        break;

    case 0x16:
        data = rldr[1].b.l;

        break;

    case 0x17:
        data = rldr[1].b.h;

        break;

    case 0x18:
        data = frc;

        break;

    case 0x19:
        data = 0xff;

        break;

    case 0x20:
        data = dma_sar0.b.l;

        break;

    case 0x21:
        data = dma_sar0.b.h;

        break;

    case 0x22:
        data = dma_sar0.b.h2 & (Z180_SAR0_MASK >> 16);

        break;

    case 0x23:
        data = dma_dar0.b.l;

        break;

    case 0x24:
        data = dma_dar0.b.h;

        break;

    case 0x25:
        data = dma_dar0.b.h2 & (Z180_DAR0_MASK >> 16);

        break;

    case 0x26:
        data = dma_bcr[0].b.l;

        break;

    case 0x27:
        data = dma_bcr[0].b.h;

        break;

    case 0x28:
        data = dma_mar1.b.l;

        break;

    case 0x29:
        data = dma_mar1.b.h;

        break;

    case 0x2a:
        data = dma_mar1.b.h2 & (Z180_MAR1_MASK >> 16);

        break;

    case 0x2b:
        data = dma_iar1.b.l;

        break;

    case 0x2c:
        data = dma_iar1.b.h;

        break;

    case 0x2d:
        data = dma_iar1.b.h2 & (Z180_IAR1_MASK >> 16);

        break;

    case 0x2e:
        data = dma_bcr[1].b.l;

        break;

    case 0x2f:
        data = dma_bcr[1].b.h;

        break;

    case 0x30:
        data = dstat | ~Z180_DSTAT_MASK | Z180_DSTAT_DWE0 | Z180_DSTAT_DWE1;

        break;

    case 0x31:
        data = dmode | ~Z180_DMODE_MASK;

        break;

    case 0x32:
        data = dcntl;

        break;

    case 0x33:
        data = il & Z180_IL_MASK;

        break;

    case 0x34:
        data = itc | ~Z180_ITC_MASK;

        break;

    case 0x36:
        data = rcr | ~Z180_RCR_MASK;

        break;

    case 0x38:
        data = mmu_cbr;

        break;

    case 0x39:
        data = mmu_bbr;

        break;

    case 0x3a:
        data = mmu_cbar;

        break;

    case 0x3e:
        data = omcr | Z180_OMCR_M1TE | ~Z180_OMCR_MASK;

        break;

    case 0x3f:
        data = iocr | ~(0xe0);

        break;

    default:
        data = 0xff;

        break;
    }

    return data;
}

// Route relocated internal-register writes through the same peripheral decode.
void Z180::z180_writecontrol(uint32_t port, uint8_t data) {
    instruction_fast = false;
    // normal external write port
    write_io(port, data);

    // store the data in the internal register
    z180_internal_port_write(port & (0x3f), data);
}

// Apply register-specific masks and side effects, including DMA admission,
// clock changes and MMU mapping (Zilog UM0050, internal I/O registers).
// port is the six-bit internal offset; data is the sampled CPU bus byte.
// Use the map above the read decoder. Register writes can change clocks and
// memory views, so affected cases notify the scheduler before it runs again.
void Z180::z180_internal_port_write(uint8_t port, uint8_t data) {
    // TN0053 pp1-2: these enhancements are absent from original K silicon.
    if (silicon == Silicon::K && (port == 0x12 || port == 0x13 || (port >= 0x1a && port <= 0x1e)))
        return;
    switch (port) {
    case 0x12:
    case 0x13:
        asci[port & 1].asext_w(data);
        break;

    case 0x1a:
    case 0x1c:
        asci[(port - 0x1a) >> 1].astcl_w(data);
        break;

    case 0x1b:
    case 0x1d:
        asci[(port - 0x1a) >> 1].astch_w(data);
        break;

    case 0x1e:

        prepare_deferred_clock_change();
        cmr = data & Z180_CMR_MASK;

        notify_clock_changed();
        break;

    case 0x1f:

        prepare_deferred_clock_change();
        ccr = silicon == Silicon::K ? data & ~0x08 : data;

        notify_clock_changed();
        break;

    case 0x00:
    case 0x01:
        asci[port & 1].cntla_w(data);
        break;

    case 0x02:
    case 0x03:
        asci[port & 1].cntlb_w(data);
        break;

    case 0x04:
    case 0x05:
        asci[port & 1].stat_w(data);
        break;

    case 0x06:
    case 0x07:
        asci[port & 1].tdr_w(data);
        break;

    case 0x08:
    case 0x09:
        asci[port & 1].rdr_w(data);
        break;

    case 0x0a:
        csio.cntr_w(data);
        break;

    case 0x0b:
        csio.trdr_w(data);
        break;

    case 0x0c:

        tmdr[0].b.l = data;
        tmdr_value[0] = (tmdr_value[0] & 0xff00) | tmdr[0].b.l;
        break;

    case 0x0d:

        tmdr[0].b.h = data;
        tmdr_value[0] = (tmdr_value[0] & 0x00ff) | (tmdr[0].b.h << 8);
        break;

    case 0x0e:

        rldr[0].b.l = data;
        break;

    case 0x0f:

        rldr[0].b.h = data;
        break;

    case 0x10:

        // UM0050 pp158,163: enabling counting preserves a programmed TMDR.
        tcr = (tcr & (Z180_TCR_TIF1 | Z180_TCR_TIF0)) | (data & ~(Z180_TCR_TIF1 | Z180_TCR_TIF0));
        // TOUT is the timer output shared with address pin A18 (UM0050 p. 164).
        // The model retains timeout parity across pin selection: before the
        // first timeout it is high. Forced levels are applied by clock_timers
        // at a timeout, rather than immediately by this write.
        tout_level = !prt1_timeout_odd;
        cycle_update_direct_reads();
        break;

    case 0x14:

        tmdr[1].b.l = data;
        tmdr_value[1] = (tmdr_value[1] & 0xff00) | tmdr[1].b.l;
        break;

    case 0x15:

        tmdr[1].b.h = data;
        tmdr_value[1] = (tmdr_value[1] & 0x00ff) | (tmdr[1].b.h << 8);
        break;

    case 0x16:

        rldr[1].b.l = data;
        break;

    case 0x17:

        rldr[1].b.h = data;
        break;

    case 0x18:

        // FRC is read-only
        break;

    case 0x20:

        dma_sar0.b.l = data;
        break;

    case 0x21:

        dma_sar0.b.h = data;
        break;

    case 0x22:

        dma_sar0.b.h2 = data & (Z180_SAR0_MASK >> 16);
        break;

    case 0x23:

        dma_dar0.b.l = data;
        break;

    case 0x24:

        dma_dar0.b.h = data;
        break;

    case 0x25:

        dma_dar0.b.h2 = data & (Z180_DAR0_MASK >> 16);
        break;

    case 0x26:

        dma_bcr[0].b.l = data;
        break;

    case 0x27:

        dma_bcr[0].b.h = data;
        break;

    case 0x28:

        dma_mar1.b.l = data;
        break;

    case 0x29:

        dma_mar1.b.h = data;
        break;

    case 0x2a:

        dma_mar1.b.h2 = data & (Z180_MAR1_MASK >> 16);
        break;

    case 0x2b:

        dma_iar1.b.l = data;
        break;

    case 0x2c:

        dma_iar1.b.h = data;
        break;

    case 0x2d:

        dma_iar1.b.h2 = data & (Z180_IAR1_MASK >> 16);
        break;

    case 0x2e:

        dma_bcr[1].b.l = data;
        break;

    case 0x2f:

        dma_bcr[1].b.h = data;
        break;

    case 0x30:

        // UM0050 pp95-97: DWE=1 protects DE, and reads back as 1.
        {
            uint8_t const previous = dstat;
            uint8_t const writable = Z180_DSTAT_DIE0 | Z180_DSTAT_DIE1 |
                                     ((data & Z180_DSTAT_DWE0) ? 0 : Z180_DSTAT_DE0) |
                                     ((data & Z180_DSTAT_DWE1) ? 0 : Z180_DSTAT_DE1);
            dstat = (dstat & ~writable) | (data & writable) | Z180_DSTAT_DWE0 | Z180_DSTAT_DWE1;
            if (data & writable & (Z180_DSTAT_DE0 | Z180_DSTAT_DE1))
                dstat |= Z180_DSTAT_DME;
            // UM0050 pp107-109: after enabling a channel, edge-sense
            // transfers are requested by DREQ falling edges. Events from
            // before that enable are not a queue of pending DMA requests.
            if (cycle_engine)
                for (unsigned ch = 0; ch < 2; ++ch)
                    if ((dstat & (0x40 << ch)) && (!(previous & (0x40 << ch)) || !(previous & Z180_DSTAT_DME)))
                        dma_edge_sampled[ch] = dma_edge_accepted[ch] = dma_edge_generation[ch];
            // UM0050 p106: a memory-to-memory start follows one further CPU
            // MC. The register-write MC itself does not satisfy that delay.
            if (cycle_engine && (data & writable & Z180_DSTAT_DE0) && ((dmode >> 2) & 3) < 2 &&
                ((dmode >> 4) & 3) < 2) {
                dma_allowed = false;
                dma_start_pending = cycle_executing;
            }
        }
        break;

    case 0x31:

        dmode = data & Z180_DMODE_MASK;
        break;

    case 0x32:

        // Switching to edge-sensed DREQ starts with a fresh edge history;
        // a previously asserted level is not a new request (UM0050 pp. 107-110).
        if (cycle_engine)
            for (unsigned ch = 0; ch < 2; ++ch)
                if ((data & ~dcntl) & (4 << ch))
                    dma_edge_sampled[ch] = dma_edge_accepted[ch] = dma_edge_generation[ch];
        dcntl = data;
        break;

    case 0x33:

        il = data & Z180_IL_MASK;
        break;

    case 0x34:

        // UFO is read-only; TRAP can be cleared but not set by a write.
        // Low three bits independently gate INT0-2 (UM0050 pp. 69-72).
        itc = (itc & Z180_ITC_UFO) | (itc & data & Z180_ITC_TRAP) | (data & 7);
        break;

    case 0x36:

        rcr = data & Z180_RCR_MASK;
        break;

    case 0x38:

        mmu_cbr = data;
        z180_mmu();
        break;

    case 0x39:

        mmu_bbr = data;
        z180_mmu();
        break;

    case 0x3a:

        mmu_cbar = data;
        z180_mmu();
        break;

    case 0x3e:

        omcr = data & Z180_OMCR_MASK;
        break;

    case 0x3f:

        iocr = data & (0xe0);
        for (auto &a : asci)
            a.set_iostop(iocr & 0x20);
        csio.set_iostop(iocr & 0x20);
        break;

    default:

        break;
    }
}

// Merge the supplied input-bit image into iol. Output bits belong to their
// peripheral callbacks and are ignored here. This helper does not dispatch
// serial edges; their pin methods do that (UM0050 pp. 7-13).
void Z180::z180_write_iolines(uint32_t data) {
    uint32_t changes = iol ^ data;

    /* I/O asynchronous clock 0 (active high) or DREQ0 (mux) */
    if (changes & Z180_CKA0) {

        iol = (iol & ~Z180_CKA0) | (data & Z180_CKA0);
    }

    /* I/O asynchronous clock 1 (active high) or TEND1 (mux) */
    if (changes & Z180_CKA1) {

        iol = (iol & ~Z180_CKA1) | (data & Z180_CKA1);
    }

    /* I/O serial clock (active high) */
    if (changes & Z180_CKS) {

        iol = (iol & ~Z180_CKS) | (data & Z180_CKS);
    }

    /* I   clear to send 0 (active low) */
    if (changes & Z180_CTS0) {

        iol = (iol & ~Z180_CTS0) | (data & Z180_CTS0);
    }

    /* I   clear to send 1 (active low) or RXS (mux) */
    if (changes & Z180_CTS1) {

        iol = (iol & ~Z180_CTS1) | (data & Z180_CTS1);
    }

    /* I   data carrier detect (active low) */
    if (changes & Z180_DCD0) {

        iol = (iol & ~Z180_DCD0) | (data & Z180_DCD0);
    }

    /* I   data request DMA ch 0 (active low) or CKA0 (mux) */
    if (changes & Z180_DREQ0) {

        iol = (iol & ~Z180_DREQ0) | (data & Z180_DREQ0);
    }

    /* I   data request DMA ch 1 (active low) */
    if (changes & Z180_DREQ1) {

        iol = (iol & ~Z180_DREQ1) | (data & Z180_DREQ1);
    }

    /* I   asynchronous receive data 0 (active high) */
    if (changes & Z180_RXA0) {

        iol = (iol & ~Z180_RXA0) | (data & Z180_RXA0);
    }

    /* I   asynchronous receive data 1 (active high) */
    if (changes & Z180_RXA1) {

        iol = (iol & ~Z180_RXA1) | (data & Z180_RXA1);
    }

    /* I   clocked serial receive data (active high) or CTS1 (mux) */
    if (changes & Z180_RXS) {

        iol = (iol & ~Z180_RXS) | (data & Z180_RXS);
    }

    /*   O request to send (active low) */
    if (changes & Z180_RTS0) {
    }

    /*   O transfer end 0 (active low) or CKA1 (mux) */
    if (changes & Z180_TEND0) {
    }

    /*   O transfer end 1 (active low) */
    if (changes & Z180_TEND1) {
    }

    /*   O transfer out (PRT channel, active low) or A18 (mux) */
    if (changes & Z180_A18_TOUT) {
    }

    /*   O asynchronous transmit data 0 (active high) */
    if (changes & Z180_TXA0) {
    }

    /*   O asynchronous transmit data 1 (active high) */
    if (changes & Z180_TXA1) {
    }

    /*   O clocked serial transmit data (active high) */
    if (changes & Z180_TXS) {
    }
}

// Restore CPU and peripheral reset state while keeping external bus callbacks.
// Zilog UM0050, Reset and register reset-value descriptions.
void Z180::reset_core() {
    previous_pc = 0;
    pc32 = 0;
    sp32 = 0;
    af32 = 0;
    bc32 = 0;
    de32 = 0;
    hl32 = 0;
    ix32 = 0;
    iy32 = 0;
    AF2.d = 0;
    BC2.d = 0;
    DE2.d = 0;
    HL2.d = 0;
    R = 0;
    R2 = 0;
    IFF1 = 0;
    IFF2 = 0;
    HALT = 0;
    IM = 0;
    I = 0;
    tmdr_latch = 0;
    read_tcr_tmdr[0] = 0;
    read_tcr_tmdr[1] = 0;
    iol = 0;
    memset(mmu, 0, sizeof(mmu));
    tmdrh[0] = 0;
    tmdrh[1] = 0;
    tmdr_value[0] = 0xffff;
    tmdr_value[1] = 0xffff;
    tmdr[0].w = tmdr[1].w = 0xffff;
    rldr[0].w = rldr[1].w = 0xffff; // UM0050 pp. 157, 159, 164: timer reset values.
    nmi_state = 0;
    nmi_pending = 0;
    irq_state[0] = 0;
    irq_state[1] = 0;
    irq_state[2] = 0;
    after_EI = 0;
    ea = 0;

    std::copy(std::begin(cc_default), std::end(cc_default), cc);
    ix16 = iy16 = 0xffff; /* IX and IY are FFFF after a reset! */
    f8 = 0;               // UM0050 p178: all defined F bits reset to zero.

    for (int i = 0; i <= Z180_INT_MAX; i++) {
        int_pending[i] = 0;
    }

    frc = 0xff;
    frc_prescale = 0;
    bus_released = false;

    /* reset io registers */
    tcr = 0x00;
    prt1_timeout_odd = false;
    tout_level = true;
    dma_iar1.b.h2 = 0x00;
    dstat = Z180_DSTAT_DWE1 | Z180_DSTAT_DWE0;
    dmode = 0x00;
    dcntl = 0xf0; // maximum number of memory and I/O wait states
    il = 0x00;
    itc = Z180_ITC_ITE0;
    rcr = Z180_RCR_REFE | Z180_RCR_REFW;
    mmu_cbr = 0x00;
    mmu_bbr = 0x00;
    mmu_cbar = 0xf0;
    omcr = Z180_OMCR_M1E | Z180_OMCR_M1TE | Z180_OMCR_IOC;
    iocr = 0x00;

    z180_mmu();
    cycle_reset();
}

// Advance the PRT divide-by-20 clock, reload counters and latch timer IRQs.
// Zilog UM0050 pp. 157-164, especially the zero/reload sequence in Fig. 64.
void Z180::clock_timers() {
    // UM0050 figure64: decrement through zero, reload on the next tick.
    for (unsigned ch = 0; ch != 2; ++ch) {
        if (!(tcr & (1 << ch)))
            continue;
        if (tmdr_value[ch] == 0)
            tmdr_value[ch] = rldr[ch].w;
        else
            --tmdr_value[ch];
        if (tmdr_value[ch] == 0) {
            tcr |= Z180_TCR_TIF0 << ch;
            if (ch == 1) {
                prt1_timeout_odd = !prt1_timeout_odd;
                bool const old_tout = tout_level;
                switch (tcr & (Z180_TCR_TOC1 | Z180_TCR_TOC0)) {
                case 0:
                    tout_level = !prt1_timeout_odd;
                    break;
                case Z180_TCR_TOC0:
                    tout_level = !tout_level;
                    break;
                case Z180_TCR_TOC1:
                    tout_level = false;
                    break;
                case Z180_TCR_TOC1 | Z180_TCR_TOC0:
                    tout_level = true;
                    break;
                }
                if (old_tout != tout_level && (tcr & 0x0c))
                    cycle_update_direct_reads();
            }
        }
    }
}

// Advance the FRC divider and PRT counters by cycles whole PHI. Serial
// deadlines are dispatched separately by the scheduler (UM0050 pp. 157-164).
void Z180::handle_io_timers(int cycles) {
    if (cycles <= 0)
        return;
    // The FRC falls every ten clocks; a falling edge to an even FRC clocks
    // both PRTs. Retain the timeout parity and the final multiplexed TOUT
    // level; instruction/DMA boundaries observe those outputs.
    const unsigned frc_ticks = (frc_prescale + cycles) / 10;
    frc_prescale = (frc_prescale + cycles) % 10;
    const unsigned ticks = (frc_ticks + (frc & 1)) / 2;
    frc -= frc_ticks;
    if (!ticks || (iocr & 0x20))
        return;
    for (unsigned ch = 0; ch != 2; ++ch) {
        if (!(tcr & (1 << ch)))
            continue;
        const unsigned value = tmdr_value[ch];
        const unsigned period = unsigned(rldr[ch].w) + 1;
        // Zero is a real count state: reload occurs on its following tick.
        const unsigned first = value ? value : period;
        const unsigned timeouts = ticks < first ? 0 : 1 + (ticks - first) / period;
        if (timeouts) {
            tcr |= Z180_TCR_TIF0 << ch;
            if (ch == 1) {
                prt1_timeout_odd ^= bool(timeouts & 1);
                bool const old_tout = tout_level;
                switch (tcr & (Z180_TCR_TOC1 | Z180_TCR_TOC0)) {
                case 0:
                    tout_level = !prt1_timeout_odd;
                    break;
                case Z180_TCR_TOC0:
                    tout_level ^= bool(timeouts & 1);
                    break;
                case Z180_TCR_TOC1:
                    tout_level = false;
                    break;
                case Z180_TCR_TOC1 | Z180_TCR_TOC0:
                    tout_level = true;
                    break;
                }
                if (old_tout != tout_level && (tcr & 0x0c))
                    cycle_update_direct_reads();
            }
        }
        if (ticks <= value)
            tmdr_value[ch] = value - ticks;
        else {
            const unsigned remainder = (ticks - value) % period;
            tmdr_value[ch] = remainder ? period - remainder : 0;
        }
    }
}

// Select the machine-cycle engine or the test-only instruction-boundary oracle.
void Z180::execute_run() {
    if (cycle_engine)
        execute_machine_cycles();
#ifdef GS2_SS_TESTING
    else
        execute_legacy();
#endif
}

// Latch external interrupt/DMA pin changes; NMI edge memory is distinct
// from level-sensitive inputs (UM0050, Interrupts and DMA chapters).
void Z180::execute_set_input(int irqline, int state) {
    instruction_fast = false;
    if (irqline == INPUT_LINE_NMI) {
        /* A new logical assertion latches NMI; the physical active-low pin falls (UM0050 p. 73). */
        if (nmi_state == 0 && state != 0)
            nmi_pending = 1;
        nmi_state = state;
    } else {

        if (irqline == Z180_INPUT_LINE_IRQ0 || irqline == Z180_INPUT_LINE_IRQ1 || irqline == Z180_INPUT_LINE_IRQ2) {
            /* update the IRQ state */
            irq_state[irqline] = state;

            /* the main execute loop will take the interrupt */
        } else if (irqline == Z180_INPUT_LINE_DREQ0) {
            if (state == 1 && !(iol & Z180_DREQ0))
                ++dma_edge_generation[0];
            uint32_t iol_arg = iol & ~Z180_DREQ0;
            if (state == 1)
                iol_arg |= Z180_DREQ0;
            z180_write_iolines(iol_arg);
        } else if (irqline == Z180_INPUT_LINE_DREQ1) {
            if (state == 1 && !(iol & Z180_DREQ1))
                ++dma_edge_generation[1];
            uint32_t iol_arg = iol & ~Z180_DREQ1;
            if (state == 1)
                iol_arg |= Z180_DREQ1;
            z180_write_iolines(iol_arg);
        }
    }
}

// Z180 machine-cycle execution, UM0050 Tables 51-52 and Figs 9-18,32-49.
// Instruction handlers remain the only implementation of ISA arithmetic.
// A suspended handler replays completed reads from a bounded journal; completed
// writes and peripheral reads are never issued twice. DMA/peripherals are not
// rolled back. The uninterrupted path calls the handler just once.

// As in the 65816 continuation path, these calls cross only trivial CPU
// frames after callbacks return. The host signal mask never changes here;
// Darwin's plain context form avoids a system call at every bus deadline.
#if defined(__APPLE__)
#define Z180_CONTINUATION_SAVE(B) _setjmp(B)
#define Z180_CONTINUATION_RESTORE(B) _longjmp(B, 1)
#else
#define Z180_CONTINUATION_SAVE(B) setjmp(B)
#define Z180_CONTINUATION_RESTORE(B) std::longjmp(B, 1)
#endif

// Rebuild certified logical read pages after MMU or board-overlay changes;
// uncertified pages continue through side-effecting memory callbacks.
void Z180::cycle_update_direct_reads() {
    instruction_fast = false;
    if (!direct_read_bus) {
        direct_logical.fill(nullptr);
        return;
    }
    for (unsigned page = 0; page < 16; ++page) {
        uint32_t const physical = a18_tout_address(mmu[page]);
        auto const base = direct_read[(physical >> 16) & 15];
        direct_logical[page] = base ? base + (physical & 0xffff) : nullptr;
    }
}

// Save instruction-entry registers for replay after a mid-instruction deadline.
void Z180::cycle_save_state() {
    Z180Register const *r[] = {&PC, &SP, &AF, &BC, &DE, &HL, &IX, &IY, &AF2, &BC2, &DE2, &HL2};
    for (unsigned n = 0; n < 12; ++n)
        instruction_start.regs[n] = r[n]->d;
    uint8_t const c[] = {R, R2, IFF1, IFF2, HALT, IM, I, after_EI, 0};
    std::copy(std::begin(c), std::end(c), instruction_start.control);
}

// Restore the instruction-entry registers while retaining the bus journal;
// replay consumes recorded reads and must not repeat completed writes.
void Z180::cycle_restore_state() {
    Z180Register *r[] = {&PC, &SP, &AF, &BC, &DE, &HL, &IX, &IY, &AF2, &BC2, &DE2, &HL2};
    for (unsigned n = 0; n < 12; ++n)
        r[n]->d = instruction_start.regs[n];
    uint8_t *c[] = {&R, &R2, &IFF1, &IFF2, &HALT, &IM, &I, &after_EI};
    for (unsigned n = 0; n < 8; ++n)
        *c[n] = instruction_start.control[n];
    previous_pc = instruction_start.regs[0];
    cursor = bus_number = nominal = 0;
    family = Z180_TABLE_op;
    opcode = 0;
    extra_cycles = 0;
    prelude_schedule = 0;
    trapping = third_trap = false;
}

// Clear the resumable bus-cycle state at processor reset.
void Z180::cycle_reset() {
    bus_release_at = UINT64_MAX;
    cycle = {};
    instruction_fast = instruction_whole = instruction_active = cycle_executing = false;
    fast_irq_valid = false;
    prelude_schedule = 0;
    history_count = 0;
    cursor = bus_number = nominal = 0;
    wait_line = bus_ack = sampled_bus_request = false;
    boundary_allowed = true;
    dma_allowed = dma_start_pending = false;
    refresh_pending = refresh_after_sleep = false;
    refresh_phase = phi_half = advance_left = refresh_address = 0;
    half_phi = retired = fast_instructions = resumptions = 0;
    power_mode = 0;
    wake_remaining = 0;
    quiet_phi = 0;
    quiet_record = 0;
    quiet_no_arbitration = false;
    dma_channel = dma_continue = 0xff;
    dma_stage = dma_byte = 0;
    dma_edge[0] = dma_edge[1] = false;
    sampled_iol = 0;
    for (unsigned n = 0; n < 2; ++n)
        dma_edge_generation[n] = dma_edge_accepted[n] = dma_edge_sampled[n] = 0;
    instruction_interrupt = sampled_interrupt = -1;
    ack_vector = -1;
    interrupt_recovery = dma_nmi = false;
}

// Publish the selected bus edge to the board and optional test observer.
// IORQ assertion/release is distinct from the data sample (UM0050 pp. 16-17).
inline void Z180::cycle_emit(uint8_t edge) {
    // UM0050 p108 Fig49: TEND is low from T1 falling through T3
    // falling of the last write cycle, including all inserted waits.
    if (cycle.owner == cycle_owner::dma && dma_stage && dma_bcr[dma_channel].w == 1 && (edge == 1 || edge == 4)) {
        unsigned const ch = dma_channel;
        if (edge == 1)
            iol |= 0x20000 << ch;
        else
            iol &= ~(0x20000 << ch);
        tend(ch, edge == 1 ? 1 : 0);
    }
    // UM0050 pp16-17 Figs7/8: IOC=1 selects at T1 falling;
    // IOC=0 selects at T2 rising. Internal I/O also asserts IORQ (UM p44).
    if (io_strobe && cycle.io_select_edge && (edge == cycle.io_select_edge || edge == 4))
        io_strobe(cycle.address, cycle.kind == cycle_kind::output, cycle.data, edge != 4);
    if (cycle_observer)
        cycle_observer(half_phi, uint8_t(cycle.kind) + 16 * uint8_t(cycle.owner), edge, cycle.address, cycle.data);
}

// Return to the board at an exact half-PHI boundary. Only trivial locals
// may cross this nonlocal return; bus callbacks have already returned.
void Z180::cycle_suspend() {
    cycle_flush_quiet();
    if (scheduler && !cycle_reference) {
        if (bus_release_at <= half_phi) {
            bus_release_at = UINT64_MAX;
            bus_release_complete();
            fast_irq_valid = false;
            instruction_fast = false;
            // Only the fixed grant ended. The external horizon still proves
            // that no other board or serial event occurred in this interval.
            if (!yield_requested && scheduler_limit > half_phi) {
                icount = int(scheduler_limit - half_phi);
                return;
            }
        }
        if (refill_budget())
            return;
    }
    // Retain exception unwinding in the independent forced-reference path.
    // Normal suspension crosses only scalar/reference ISA and engine frames;
    // memory handlers and callbacks have returned before either yield site.
    // No destructor or hardware edge is bypassed by this control transfer.
#ifdef GS2_SS_TESTING
    if (cycle_reference)
        throw cycle_yield{};
#endif
    Z180_CONTINUATION_RESTORE(cycle_resume);
}

// Settle deferred timer/refresh clocks before any observable peripheral edge.
inline void Z180::cycle_flush_quiet() {
    if (!quiet_phi)
        return;
    // No observer has run between these complete CPU cycles. Their journal
    // already records every read and Ti; materialize the final completed bus
    // job once, before a callback or partial phase can inspect it. MMU and
    // pin selection cannot change before this flush.
    auto const &h = history[quiet_record];
    bool const internal = h.kind == cycle_kind::internal;
    cycle = {};
    cycle.kind = h.kind;
    cycle.data = h.data;
    cycle.address = internal ? 0 : MMU_REMAP_ADDR(h.address);
    cycle.nominal = internal ? 1 : 3;
    cycle.phase = internal ? 1 : 9;
    cycle.elapsed = internal ? 2 : 6;
    cycle.sampled = !internal;
    cycle.no_arbitration = quiet_no_arbitration;
    // Every composed span contains an even number of half-PHI ticks, so
    // phi_half is unchanged. No register/callback can change these divider
    // inputs until after this flush. Refresh is disabled, but its phase still
    // runs and must be retained for a later RCR write.
    assert(!(tcr & 3) && !(rcr & 0x80));
    {
        unsigned const prescale = frc_prescale + quiet_phi;
        frc -= prescale / 10;
        frc_prescale = prescale % 10;
    }
    refresh_phase = (refresh_phase + 2 * quiet_phi) % 160;
    quiet_phi = 0;
}

// Advance a certified interval with no intervening external event; stop
// deferred clocks from crossing the next scheduled event.
inline void Z180::cycle_advance_quiet(unsigned ticks) {
    assert(!quiet_phi && !(tcr & 3) && !advance_left && icount >= int(ticks));
    icount -= ticks;
    half_phi += ticks;
    cycle.elapsed += ticks;
    unsigned const phi = (phi_half + ticks) / 2;
    phi_half = (phi_half + ticks) & 1;
    {
        unsigned const prescale = frc_prescale + phi;
        frc -= prescale / 10;
        frc_prescale = prescale % 10;
    }
    unsigned const period = 20U << (rcr & 3);
    if ((rcr & 0x80) && (refresh_phase % period + ticks >= period) && !bus_ack)
        refresh_pending = true;
    refresh_phase = (refresh_phase + ticks) % 160;
}

// Consume half-PHI clocks and dispatch peripheral events in emulated-time order.
void Z180::cycle_advance(unsigned ticks) {
    assert(!quiet_phi);
    if (!advance_left)
        advance_left = ticks;
    if (!cycle_reference) {
        // No callback occurs inside this span. FRC/PRT already implement
        // their divider and reload recurrences arithmetically; update them
        // once at the next externally observable edge or scheduler boundary.
        // Keep the one-half-PHI implementation below as the reference.
        while (advance_left) {
            if (icount <= 0)
                cycle_suspend();
            unsigned const n = std::min<unsigned>(advance_left, icount);
            icount -= n;
            advance_left -= n;
            half_phi += n;
            cycle.elapsed += n;
            unsigned const phi = (phi_half + n) / 2;
            phi_half = (phi_half + n) & 1;
            {
                if (tcr & 3)
                    handle_io_timers(phi);
                else {
                    unsigned const prescale = frc_prescale + phi;
                    frc -= prescale / 10;
                    frc_prescale = prescale % 10;
                }
            }
            unsigned const period = 20U << (rcr & 3);
            if ((rcr & 0x80) && (refresh_phase % period + n >= period) && !bus_ack)
                refresh_pending = true;
            refresh_phase = (refresh_phase + n) % 160;
            if (advance_left)
                cycle_suspend();
        }
        return;
    }
    while (advance_left) {
        if (icount <= 0)
            cycle_suspend();
        unsigned n = std::min<unsigned>(advance_left, cycle_reference ? 1 : icount);
        n = std::min(n, 2 * (10U - frc_prescale) - phi_half);
        icount -= n;
        advance_left -= n;
        half_phi += n;
        cycle.elapsed += n;
        unsigned const phi = (phi_half + n) / 2;
        phi_half = (phi_half + n) & 1;
        handle_io_timers(phi);
        unsigned const period = 20U << (rcr & 3);
        if ((rcr & 0x80) && (refresh_phase % period + n >= period) && !bus_ack)
            refresh_pending = true;
        refresh_phase = (refresh_phase + n) % 160;
    }
}

// Create a bus cycle with its initial strobes, synchronization and wait count.
// Zilog UM0050 pp. 16-30 describes the external memory/I/O phases.
void Z180::cycle_begin(cycle_owner owner, cycle_kind kind, uint32_t address, uint8_t data, bool no_arbitration) {
    cycle_flush_quiet();
    assert(!cycle.active);
    cycle = {};
    cycle.owner = owner;
    cycle.kind = kind;
    cycle.address = address;
    cycle.data = data;
    cycle.active = true;
    cycle.no_arbitration = no_arbitration;
    bool const io = kind == cycle_kind::input || kind == cycle_kind::output;
    if (io)
        cycle.io_select_edge = (omcr & 0x20) ? 1 : 2;
    bool const write = kind == cycle_kind::write || kind == cycle_kind::output;
    bool const internal = kind == cycle_kind::internal || kind == cycle_kind::release;
    if (internal)
        cycle.nominal = 1;
    else if (kind == cycle_kind::refresh)
        cycle.nominal = (rcr & 0x40) ? 3 : 2;
    else if (kind == cycle_kind::acknowledge) {
        cycle.nominal = 5;
        cycle.waits = owner == cycle_owner::cpu && instruction_interrupt == 2
                          ? (((dcntl >> 4) & 3) ? ((dcntl >> 4) & 3) + 3 : 2)
                          : 2;
    } else if (kind == cycle_kind::nmi)
        cycle.waits = 0;
    else {
        cycle.waits = io ? (is_internal_io_address(address) ? 0 : io_wait_states()) : memory_wait_states();
        if (io && is_internal_io_address(address)) {
            unsigned const reg = address & 0x3f;
            bool const data_register = (write ? reg == 6 || reg == 7 : reg == 8 || reg == 9) || reg == 0x0b ||
                                       (reg >= 0x0c && reg <= 0x0f) || (reg >= 0x14 && reg <= 0x17);
            // Model the UM0050 p. 30 synchronization bound with a shared five-PHI phase.
            // cycle_begin has flushed deferred clocks. T3's candidate rising
            // edge is four half-PHI ahead; derive q from the shared FRC phase.
            // This is the same future phase as evaluation at T2 falling.
            if (data_register) {
                unsigned const q = (frc_prescale + (phi_half + 4) / 2) % 5;
                cycle.waits = (5 - q) % 5;
            }
        }
        if (bus_wait && !(io && is_internal_io_address(address)) && (io || write || !cycle_direct_read(address)))
            cycle.waits = std::max<unsigned>(cycle.waits, bus_wait(address, write, io));
    }
    cycle_emit(0);
}

// Sample a fetched opcode once, preserving its value for instruction replay.
void Z180::cycle_sample_opcode() {
    if (cycle.kind == cycle_kind::fetch || cycle.kind == cycle_kind::halt || cycle.kind == cycle_kind::nmi) {
        if (auto const p = cycle_direct_read(cycle.address))
            cycle.data = drive_direct_read(*p);
        else
            cycle.data = read_memory(cycle.address);
        cycle.sampled = true;
        cycle_emit(4);
    } else if (cycle.kind == cycle_kind::acknowledge) {
        if (instruction_interrupt == 2 && ack_vector < 0) {
            ack_vector = bus.irq_ack ? bus.irq_ack() : 0xff;
        }
        cycle.data = ack_vector < 0 ? 0 : uint8_t(ack_vector);
        cycle.sampled = true;
        cycle_emit(4);
    }
}

// Perform the read/write callback at the bus data edge, not at instruction
// retirement; the journal preserves completed side effects across a yield.
void Z180::cycle_sample_data() {
    if (!cycle.sampled) {
        // Set the guard before a handler that can abort this timeslice.
        cycle.sampled = true;
        auto const direct = (cycle.kind == cycle_kind::operand || cycle.kind == cycle_kind::read)
                                ? cycle_direct_read(cycle.address)
                                : nullptr;
        if (direct)
            cycle.data = drive_direct_read(*direct);
        else
            switch (cycle.kind) {
            case cycle_kind::operand:
                cycle.data = read_memory(cycle.address);
                break;
            case cycle_kind::read:
                cycle.data = read_memory(cycle.address);
                break;
            case cycle_kind::write:
                write_memory(cycle.address, cycle.data);
                break;
            case cycle_kind::input:
                cycle.data =
                    is_internal_io_address(cycle.address) ? z180_readcontrol(cycle.address) : read_io(cycle.address);
                break;
            case cycle_kind::output:
                if (is_internal_io_address(cycle.address))
                    z180_writecontrol(cycle.address, cycle.data);
                else
                    write_io(cycle.address, cycle.data);
                break;
            default:
                break;
            }
        cycle_emit(4);
        // I/O can alter the clock, timers, DMA or pins. Re-evaluate
        // arbitration for the rest of this instruction after that edge.
        if (cycle.kind == cycle_kind::input || cycle.kind == cycle_kind::output)
            instruction_fast = false;
    }
}

// Finish a cycle in one span only when no pin observer, wait or deadline
// can distinguish it from edge-by-edge execution.
bool Z180::cycle_run_unobserved() {
    // This is the same six-edge memory cycle, evaluated at its observable
    // sample and end instead of dispatching six one-half-PHI phases. The
    // scheduler budget excludes external input changes; without a PRT tick
    // source, sampled pins/interrupts cannot change before the memory handler.
    // IORQ uses its own observable-edge path below. TEND, trace callbacks,
    // asynchronous WAIT and partial cycles retain the explicit phase path.
    // A memory callback can shorten the budget: in
    // that case leave its exact saved phase for the ordinary continuation.
    if (cycle_reference || cycle_observer || cycle.phase || advance_left || cycle.ttp || cycle.trailing || wait_line ||
        (tcr & 3) || (cycle.owner == cycle_owner::dma && dma_stage && dma_bcr[dma_channel].w == 1))
        return false;
    bool const internal = cycle.kind == cycle_kind::internal || cycle.kind == cycle_kind::release;
    if (!internal && (cycle.nominal != 3 || cycle.kind >= cycle_kind::internal))
        return false;
    unsigned const waits = 2U * cycle.waits;
    // The continuation's span counter is 16 bits. Very long externally
    // supplied waits remain a sequence of individual TW cycles.
    if (waits > 65530)
        return false;
    if (icount < int(internal ? 2 : 6 + waits))
        return false;
    if (cycle.io_select_edge)
        return cycle_run_io_unobserved();
    bool const fetch = cycle.kind == cycle_kind::fetch;
    sampled_iol = iol;
    std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
    sampled_bus_request = bus_released;
    if (cycle.owner == cycle_owner::cpu && cycle.kind != cycle_kind::release) {
        if (instruction_fast && fast_irq_valid)
            sampled_interrupt = fast_interrupt;
        else {
            cycle_sample_interrupts();
            fast_interrupt = sampled_interrupt;
            fast_irq_valid = instruction_fast;
        }
    } else if (cycle.owner == cycle_owner::dma && dma_stage && nmi_pending)
        dma_nmi = true;
    if (internal) {
        cycle_advance_quiet(2);
        cycle.phase = 1;
        cycle.active = false;
        return true;
    }
    cycle_advance_quiet((fetch ? 4 : 5) + waits);
    cycle.waits = 0;
    cycle.phase = fetch ? 5 : 6;
    if (fetch)
        cycle_sample_opcode();
    else
        cycle_sample_data();
    if (nmi_pending || bus_released || wait_line)
        instruction_fast = false;
    unsigned const tail = fetch ? 2 : 1;
    if (icount < int(tail))
        return false;
    if (tcr & 3)
        cycle_advance(tail);
    else
        cycle_advance_quiet(tail);
    cycle.phase = 9;
    cycle.active = false;
    return true;
}

// Collapse an I/O cycle only when its select/data/release edges can all
// finish before the next board event (UM0050 Figs. 7-8).
bool Z180::cycle_run_io_unobserved() {
    // Same external I/O cycle as the phase loop: IOC selects IORQ at T1
    // falling or T2 rising (UM0050 Figs 7/8); data and IORQ release remain
    // at T3 falling. No timed source changes inside the admitted span.
    // The select callback can shorten it or change WAIT/PRT. Publish its
    // phase first, then fall back without repeating the already issued edge.
    auto sample_dma = [this]() {
        sampled_iol = iol;
        std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
    };
    auto admitted = [this](unsigned remaining) {
        return icount >= int(remaining) && !wait_line && !(tcr & 3) && !cycle_observer && !cycle_reference;
    };
    unsigned const waits = 2U * cycle.waits;
    if (cycle.io_select_edge == 1) {
        cycle_advance_quiet(1);
        cycle.phase = 1;
        cycle_emit(1);
        if (!admitted(5 + waits))
            return false;
        cycle_advance_quiet(1);
        cycle.phase = 2;
        sample_dma();
    } else {
        cycle_advance_quiet(2);
        cycle.phase = 2;
        sample_dma();
        cycle_emit(2);
        if (!admitted(4 + waits))
            return false;
    }
    // DREQ is sampled at T2 rising and every TW rising. All TW samples
    // here see the same inputs; retain the final sample before T3.
    cycle_advance_quiet(1 + waits);
    cycle.waits = 0;
    cycle.phase = 4;
    if (waits)
        sample_dma();
    sampled_bus_request = bus_released;
    if (cycle.owner == cycle_owner::cpu)
        cycle_sample_interrupts();
    else if (cycle.owner == cycle_owner::dma && dma_stage && nmi_pending)
        dma_nmi = true;
    cycle_advance_quiet(2);
    cycle.phase = 6;
    cycle_sample_data();
    if (!admitted(1))
        return false;
    cycle_advance_quiet(1);
    cycle.phase = 9;
    cycle.active = false;
    return true;
}

// Permit scheduled edge execution only when the active cycle has a stable
// set of waits and observable edges.
bool Z180::cycle_timeline_ready() const {
    return scheduled_timeline && scheduler && !cycle_reference && !cycle_observer && instruction_interrupt < 0 &&
           !trapping && !(dstat & 0xc0) && !(rcr & 0x80) && !(tcr & 3) && !wait_line && !nmi_pending &&
           !(IFF1 && (irq_state[0] || irq_state[1] || irq_state[2])) && dma_channel == 0xff && dma_continue == 0xff;
}

// Visit the remaining select, data and release edges in order while retaining
// the cycle cursor if a board deadline interrupts the interval.
bool Z180::cycle_timeline() {
    // Compose a CPU memory/Ti cycle across board deadlines. The only edges
    // inside the span are DMA/IRQ samples and the data edge; fixed TWs are
    // arithmetic. A copy may change BUSREQ at a deadline without discarding
    // this instruction's timeline. I/O, IRQ, DMA and live peripheral clocks
    // retain the phase engine. Materialize its exact continuation on exit.
    if (!cycle_timeline_ready() || cycle.owner != cycle_owner::cpu || cycle.phase || advance_left || cycle.ttp ||
        cycle.trailing || cycle.io_select_edge)
        return false;
    bool const internal = cycle.kind == cycle_kind::internal || cycle.kind == cycle_kind::release;
    if (!internal && (cycle.kind > cycle_kind::write || cycle.nominal != 3 || cycle.waits > 32765))
        return false;
    if (internal && cycle.nominal != 1)
        return false;
    cycle_flush_quiet();
    auto sample_dma = [this]() {
        sampled_iol = iol;
        std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
    };
    auto sample_irq = [this]() {
        sampled_bus_request = bus_released;
        if (cycle.kind != cycle_kind::release)
            cycle_sample_interrupts();
    };
    unsigned const waits = cycle.waits;
    bool const fetch = cycle.kind == cycle_kind::fetch;
    unsigned const irq = internal ? 1 : 3 + 2 * waits;
    unsigned const data = internal ? 2 : irq + (fetch ? 1 : 2);
    unsigned const end = internal ? 2 : 6 + 2 * waits;
    if (internal)
        sample_dma();
    unsigned position = 0;
    // Most cycles fit before the next board event. Construct their
    // data-edge continuation directly; the partial timeline below is needed
    // only when that event interrupts the cycle.
    if (icount >= int(data)) {
        cycle_advance_quiet(data);
        if (!internal)
            sample_dma();
        sample_irq();
        cycle.waits = 0;
        cycle.phase = internal ? 1 : fetch ? 5 : 6;
        if (internal) {
            cycle.active = false;
            return true;
        }
        if (fetch)
            cycle_sample_opcode();
        else
            cycle_sample_data();
        if (!cycle_timeline_ready())
            return false;
        unsigned const tail = end - data;
        if (icount >= int(tail)) {
            cycle_advance_quiet(tail);
            cycle.phase = 9;
            cycle.active = false;
            return true;
        }
        position = data;
    }
    while (position < end) {
        if (icount <= 0) {
            advance_left = 1;
            cycle_suspend();
            advance_left = 0;
            if (!cycle_timeline_ready())
                return false;
        }
        unsigned const stop = position < data ? data : end;
        unsigned const next = position + std::min<unsigned>(icount, stop - position);
        cycle_advance_quiet(next - position);
        // T2 rising is +2; fixed TW rising edges are +4,+6,...,+irq-1.
        // Inputs are constant inside this scheduler grant, so retain the last
        // sample crossed by the span, before servicing an event at the same instant.
        if (!internal && ((position < 2 && next >= 2) ||
                          (waits && next >= 4 && position < irq - 1 && (std::min(next, irq - 1) & ~1U) > position)))
            sample_dma();
        if (position < irq && next >= irq)
            sample_irq();
        position = next;
        if (internal)
            cycle.phase = position ? 1 : 0;
        else if (position < 3)
            cycle.phase = position;
        else if (position < irq) {
            cycle.waits = waits - (position - 3) / 2;
            cycle.phase = (position - 3) & 1 ? 8 : 7;
        } else {
            cycle.waits = 0;
            cycle.phase = position == end ? 9 : 4 + position - irq;
        }
        if (!internal && position == data) {
            if (fetch)
                cycle_sample_opcode();
            else
                cycle_sample_data();
            if (!cycle_timeline_ready())
                return false;
        }
    }
    cycle.active = false;
    return true;
}

// Resume a cycle at its saved edge; incomplete data phases cannot be
// committed just because the caller supplied another execution budget.
bool Z180::cycle_run_partial() {
    if (cycle_reference || cycle_observer || icount <= 0 || advance_left > 1 || cycle.nominal != 3 ||
        cycle.kind >= cycle_kind::internal || cycle.ttp || cycle.trailing || cycle.io_select_edge || wait_line ||
        (tcr & 3) || cycle.waits > 32765 ||
        (cycle.owner == cycle_owner::dma && dma_stage && dma_bcr[dma_channel].w == 1))
        return false;
    // A deadline can split any half-PHI. Advance directly to the next pin
    // sample or data edge, preserving the exact phase that the reference
    // suspends with, including its one-half-PHI pending advance.
    advance_left = 0;
    auto sample_dma = [this]() {
        sampled_iol = iol;
        std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
    };
    auto sample_irq = [this]() {
        sampled_bus_request = bus_released;
        if (cycle.owner == cycle_owner::cpu)
            cycle_sample_interrupts();
        else if (cycle.owner == cycle_owner::dma && dma_stage && nmi_pending)
            dma_nmi = true;
    };
    // Compose T1/T2 and fixed TWs up to the memory edge or this grant's
    // deadline. Samples cannot change inside this admitted span. Preserve
    // the exact pending TW and sample state when the grant cuts it short.
    bool const fetch = cycle.kind == cycle_kind::fetch;
    if (cycle.waits && cycle.phase < 3 && cycle.elapsed == cycle.phase) {
        unsigned const start = cycle.phase, irq = 3 + 2U * cycle.waits;
        unsigned const sample = irq + (fetch ? 1 : 2);
        unsigned const end = start + std::min<unsigned>(icount, sample - start);
        cycle_advance_quiet(end - start);
        if ((start < 2 && end >= 2) || end >= 4)
            sample_dma();
        if (end < 3)
            cycle.phase = end;
        else if (end < irq) {
            cycle.waits -= (end - 3) / 2;
            cycle.phase = (end - 3) & 1 ? 8 : 7;
        } else {
            cycle.waits = 0;
            sample_irq();
            cycle.phase = 4 + end - irq;
            if (end == sample) {
                if (fetch)
                    cycle_sample_opcode();
                else
                    cycle_sample_data();
                if (nmi_pending || bus_released || wait_line)
                    instruction_fast = false;
                if (tcr & 3)
                    return false;
            }
        }
    }
    // Zero-wait continuations can also compose from a later T2/T3 phase.
    static constexpr uint8_t position[] = {0, 1, 2, 3, 3, 4};
    static constexpr uint8_t phase_at[] = {0, 1, 2, 4, 5, 6};
    unsigned const phase = cycle.phase;
    if (!cycle.waits && phase < 6 && !(fetch && phase == 5) && cycle.elapsed == position[phase]) {
        unsigned const start = position[phase], sample = fetch ? 4 : 5;
        unsigned const n = std::min<unsigned>(icount, sample - start), end = start + n;
        cycle_advance_quiet(n);
        if (phase < 2 && end >= 2)
            sample_dma();
        if (phase < 4 && end >= 3)
            sample_irq();
        cycle.phase = phase_at[end];
        if (end == sample) {
            if (fetch)
                cycle_sample_opcode();
            else
                cycle_sample_data();
            if (nmi_pending || bus_released || wait_line)
                instruction_fast = false;
            if (tcr & 3)
                return false;
        }
    }
    for (;;) {
        if (cycle.phase == 9) {
            cycle.active = false;
            return true;
        }
        if (cycle.phase == 3) {
            if (cycle.waits) {
                cycle.phase = 7;
                continue;
            }
            sample_irq();
            cycle.phase = 4;
            continue;
        }
        if (icount <= 0) {
            advance_left = 1;
            cycle_suspend();
            if (wait_line || (tcr & 3))
                return false;
            advance_left = 0;
        }
        switch (cycle.phase) {
        case 0:
        case 1: {
            unsigned const n = std::min<unsigned>(icount, 2 - cycle.phase);
            cycle_advance_quiet(n);
            cycle.phase += n;
            if (cycle.phase == 2)
                sample_dma();
            break;
        }
        case 2:
            cycle_advance_quiet(1);
            cycle.phase = 3;
            break;
        case 7: {
            unsigned const n = std::min<unsigned>(icount, 2U * std::max<unsigned>(1, cycle.waits));
            cycle_advance_quiet(n);
            sample_dma();
            cycle.waits -= std::min<unsigned>(cycle.waits, n / 2);
            cycle.phase = (n & 1) ? 8 : 3;
            break;
        }
        case 8:
            cycle_advance_quiet(1);
            if (cycle.waits)
                --cycle.waits;
            cycle.phase = 3;
            break;
        case 4:
        case 5: {
            bool const fetch = cycle.kind == cycle_kind::fetch;
            unsigned const sample = fetch ? 5 : 6;
            if (cycle.phase < sample) {
                unsigned const n = std::min<unsigned>(icount, sample - cycle.phase);
                cycle_advance_quiet(n);
                cycle.phase += n;
                if (cycle.phase == sample) {
                    if (fetch)
                        cycle_sample_opcode();
                    else
                        cycle_sample_data();
                    if (nmi_pending || bus_released || wait_line)
                        instruction_fast = false;
                    if (tcr & 3)
                        return false;
                }
                break;
            }
            // A fetched opcode is already sampled; T3's remaining halves
            // have no other callback until the cycle ends.
            unsigned const n = std::min<unsigned>(icount, 2);
            cycle_advance_quiet(n);
            cycle.phase = n == 2 ? 9 : 6;
            break;
        }
        case 6:
            cycle_advance_quiet(1);
            cycle.phase = 9;
            break;
        default:
            z180_error("Z180 unexpected partial phase %u", cycle.phase);
        }
    }
}

// Execute machine-cycle edges, sampling WAIT at T2/TW falling. Internal I/O
// retains strobes but ignores external wait insertion (UM0050 pp. 19, 27, 44).
// phase is a continuation label, not a T-state number. 0 starts T1; 1 is
// T1 falling; 2 is T2 rising; 3 is T2 falling and the WAIT decision. 7/8
// perform the two halves of each TW. 4 reaches T3; 5 resumes after an
// opcode sample and 6 after an ordinary data sample. 9 completes the cycle;
// 10 handles TTP, the extra trap-read clock. Internal/release cycles use
// phases 0/1 for their last falling/rising edges. A deadline may stop any phase.
// Data callbacks occur only at the sample phase, so resuming cannot write twice.
void Z180::cycle_run() {
    cycle_flush_quiet();
    // Reject impossible shortcuts before entering their host stack frames.
    // Partial scheduler grants commonly resume at a nonzero bus phase.
    if (!cycle_reference && !cycle_observer) {
        if (!cycle.phase && !advance_left && cycle_run_unobserved())
            return;
        if (icount > 0 && cycle.nominal == 3 && cycle.kind < cycle_kind::internal && !cycle.io_select_edge &&
            cycle_run_partial())
            return;
    }
    // Phase numbers distinguish T1 rise/fall, T2 rise/fall and T3 rise/fall.
    // WAIT is sampled at T2/TW falling (UM0050 pp19,27); no memory or I/O
    // operation is committed until its documented sample/WR release edge.
    while (cycle.active) {
        // The no-event path invokes the ISA handler once and unrolls its
        // ordinary bus cycles. It has exactly the same edges and sample
        // functions as the resumable path below. A callback may still bring
        // a scheduler deadline forward: phase is saved before every callback.
        if (instruction_fast && cycle.owner == cycle_owner::cpu && cycle.phase == 0 && cycle.nominal == 3 &&
            !cycle.ttp && !cycle.waits && !wait_line && icount >= 6) {
            cycle_advance(1);
            cycle.phase = 1;
            cycle_emit(1);
            cycle_advance(1);
            cycle.phase = 2;
            sampled_iol = iol;
            std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
            cycle_emit(2);
            cycle_advance(1);
            cycle.phase = 3;
            if (wait_line)
                continue;
            sampled_bus_request = bus_released;
            cycle_sample_interrupts();
            cycle.phase = 4;
            cycle_advance(1);
            cycle.phase = 5;
            cycle_sample_opcode();
            cycle_advance(1);
            cycle.phase = 6;
            cycle_sample_data();
            cycle_advance(1);
            cycle.phase = 9;
            cycle.active = false;
            cycle_emit(5);
            return;
        }
        bool const internal = cycle.kind == cycle_kind::internal || cycle.kind == cycle_kind::release;
        if (internal || cycle.kind == cycle_kind::refresh) {
            if (!cycle.phase) {
                if (!advance_left && internal) {
                    sampled_iol = iol;
                    std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
                }
                cycle_advance(2 * cycle.nominal - 1);
                cycle.phase = 1;
                if (internal) {
                    sampled_bus_request = bus_released;
                    if (cycle.kind != cycle_kind::release)
                        cycle_sample_interrupts();
                }
                cycle_emit(1);
            }
            cycle_advance(1);
            cycle.active = false;
            cycle_emit(5);
            return;
        }
        switch (cycle.phase) {
        case 0:
            cycle_advance(1);
            cycle.phase = 1;
            cycle_emit(1);
            break;
        case 1:
            cycle_advance(1);
            cycle.phase = 2;
            sampled_iol = iol;
            std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
            cycle_emit(2);
            break;
        case 2:
            cycle_advance(1);
            cycle.phase = 3;
            break;
        case 3:
            // UM0050 p44: internal I/O duplicates the external strobes but
            // ignores external WAIT and DCNTL.IWI. Data synchronization is
            // independent and selected from the shared five-PHI service phase.
            if (cycle.waits || (wait_line && !((cycle.kind == cycle_kind::input || cycle.kind == cycle_kind::output) &&
                                               is_internal_io_address(cycle.address)))) {
                cycle.phase = 7;
                break;
            }
            sampled_bus_request = bus_released;
            if (cycle.owner == cycle_owner::cpu)
                cycle_sample_interrupts();
            else if (cycle.owner == cycle_owner::dma && dma_stage && nmi_pending)
                dma_nmi = true;
            cycle.phase = cycle.ttp ? 10 : 4;
            break;
        case 10:
            cycle_advance(2);
            cycle.phase = 4;
            break; // UM0050 p72: TTP is not a bus boundary.
        case 7:
            cycle_advance(1);
            cycle.phase = 8;
            sampled_iol = iol;
            std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
            break;
        case 8:
            cycle_advance(1);
            if (cycle.waits)
                --cycle.waits;
            cycle_emit(3);
            cycle.phase = 3;
            break;
        case 4:
            cycle_advance(1);
            cycle.phase = 5;
            cycle_sample_opcode();
            break;
        case 5:
            cycle_advance(1);
            cycle.phase = 6;
            cycle_sample_data();
            break;
        case 6:
            cycle_advance(1);
            cycle.phase = 9;
            break;
        case 9:
            if (cycle.trailing)
                cycle_advance(2 * cycle.trailing);
            cycle.active = false;
            cycle_emit(5);
            return;
        }
    }
}

// Apply opcode-prefix semantics and preserve the decoded family for replay.
// family_arg selects a timing-table row; opcode_arg is its final instruction
// byte. The packed schedule stores three bits per bus position: how many Ti
// clocks must precede it. The result is private scheduling state, not a return value.
void Z180::cycle_decode(unsigned family_arg, unsigned opcode_arg) {
    family = family_arg;
    opcode = opcode_arg;
    // Table 51's Ti positions are invariant for an opcode. Pack the eight
    // possible transaction positions once, instead of decoding the opcode
    // afresh on every operand/read/write. TRAP and RETI's M1 option below
    // retain their runtime choices.
    static constexpr auto schedules = [] {
        std::array<std::array<uint32_t, 256>, 5> t{};
        for (unsigned op = 0; op < 256; ++op) {
            auto set = [&](unsigned f, unsigned n, unsigned ti) { t[f][op] |= ti << (3 * n); };
            if (op == 0x10 || op == 0x02 || op == 0x12 || (op >= 0x70 && op <= 0x77 && op != 0x76))
                set(Z180_TABLE_op, 1, 1);
            if (op == 0x22 || op == 0x32 || op == 0xcd || (op & 0xc7) == 0xc4 || op == 0xe3)
                set(Z180_TABLE_op, 3, 1);
            if (op == 0x34 || op == 0x35 || op == 0xd3)
                set(Z180_TABLE_op, 2, 1);
            if ((op & 0xcf) == 0xc5 || (op & 0xc7) == 0xc7)
                set(Z180_TABLE_op, 1, 2);
            if ((op & 0xc7) == 0xc0)
                set(Z180_TABLE_op, 1, 1);
            if (op == 0x22 || op == 0xe3 || op == 0x34 || op == 0x35)
                set(Z180_TABLE_xy, 4, 1);
            if (op == 0xe5)
                set(Z180_TABLE_xy, 2, 2);
            if (op == 0x34 || op == 0x35 || (op >= 0x40 && op < 0xc0 && (op & 7) == 6))
                set(Z180_TABLE_xy, 3, 2);
            if (op >= 0x70 && op <= 0x77 && op != 0x76)
                set(Z180_TABLE_xy, 3, 3);
            if (op < 0x40 && (op & 7) == 1)
                set(Z180_TABLE_ed, 3, 1);
            if ((op >= 0x40 && op < 0x80 && (op & 7) == 1) || op == 0x34 || op == 0x83 || op == 0x8b || op == 0x93 ||
                op == 0x9b)
                set(Z180_TABLE_ed, 2, 1);
            if ((op & 0xcf) == 0x43)
                set(Z180_TABLE_ed, 4, 1);
            if (op == 0x67 || op == 0x6f)
                set(Z180_TABLE_ed, 3, 4);
            set(Z180_TABLE_cb, 3, 1);
            set(Z180_TABLE_xycb, 5, 1);
        }
        return t;
    }();
    prelude_schedule = schedules[family_arg][opcode_arg];
    // Admission is made once when the opcode is known, from its maximum
    // nominal duration and the current external-event horizon. Prefixes
    // admit at their final opcode. I/O and pin changes revoke the proof;
    // an address-dependent external wait can still require a continuation.
    unsigned const bound = cc[family_arg][opcode_arg] + cc[Z180_TABLE_ex][opcode_arg];
    instruction_whole = instruction_fast && !cycle_observer && cc[family_arg][opcode_arg] &&
                        icount >= int(2 * (bound > nominal ? bound - nominal : 0));
}

// Return Ti clocks before a bus access using the explicit timing-table rules.
// Zilog UM0050 Tables 51-52 specify the machine-cycle sequence.
unsigned Z180::cycle_prelude_reference(cycle_kind kind) const {
    unsigned const n = bus_number, op = opcode;
    if (trapping) {
        if (kind == cycle_kind::write && n == (third_trap ? 5U : 2U))
            return third_trap ? 4 : 5;
        return 0;
    }
    if (family == Z180_TABLE_op) {
        if (op == 0x10 && n == 1)
            return 1; // UM p256: its following boundary is inhibited.
        if ((op == 0x02 || op == 0x12 || (op >= 0x70 && op <= 0x77 && op != 0x76)) && n == 1)
            return 1;
        if ((op == 0x22 || op == 0x32 || op == 0xcd || (op & 0xc7) == 0xc4) && n == 3)
            return 1;
        if ((op == 0x34 || op == 0x35) && n == 2)
            return 1;
        if (((op & 0xcf) == 0xc5 || (op & 0xc7) == 0xc7) && n == 1)
            return 2;
        if ((op & 0xc7) == 0xc0 && n == 1)
            return 1;
        if (op == 0xe3 && n == 3)
            return 1;
        if (op == 0xd3 && n == 2)
            return 1;
    } else if (family == Z180_TABLE_xy) {
        if (op == 0x22 && n == 4)
            return 1;
        if (op == 0xe3 && n == 4)
            return 1;
        if (op == 0xe5 && n == 2)
            return 2;
        if ((op == 0x34 || op == 0x35) && n == 4)
            return 1;
        if (n == 3 && (op == 0x34 || op == 0x35 || (op >= 0x40 && op < 0xc0 && (op & 7) == 6)))
            return 2;
        if (n == 3 && op >= 0x70 && op <= 0x77 && op != 0x76)
            return 3;
    } else if (family == Z180_TABLE_ed) {
        if (op < 0x40 && (op & 7) == 1 && n == 3)
            return 1;
        if (op >= 0x40 && op < 0x80 && (op & 7) == 1 && n == 2)
            return 1;
        if ((op == 0x34 || op == 0x83 || op == 0x8b || op == 0x93 || op == 0x9b) && n == 2)
            return 1;
        if ((op & 0xcf) == 0x43 && n == 4)
            return 1;
        if ((op == 0x67 || op == 0x6f) && n == 3)
            return 4;
        if (op == 0x4d && !(omcr & 0x80)) {
            if (n == 2)
                return 3;
            if (n == 3)
                return 1;
        }
    } else if (family == Z180_TABLE_cb && kind == cycle_kind::write && n == 3)
        return 1;
    else if (family == Z180_TABLE_xycb && kind == cycle_kind::write && n == 5)
        return 1;
    return 0;
}

// Enter the fully observable bus path when a direct access cannot be certified.
uint8_t Z180::cycle_access_slow(cycle_kind kind, uint32_t address, uint8_t data, unsigned pre) {
    for (unsigned n = 0; n < pre; ++n)
        cycle_transaction(cycle_kind::internal, 0, 0, family == Z180_TABLE_op && opcode == 0x10);
    ++bus_number;
    return cycle_transaction(kind, address, data);
}

// Advance internal instruction clocks while preserving arbitration boundaries.
void Z180::cycle_whole_internal(unsigned count, bool no_arbitration) {
    assert(count && cursor == history_count && !cycle.active);
    if (history_count + count > std::size(history))
        z180_error("Z180 instruction journal overflow at %04x", previous_pc);
    // These Ti boundaries have identical sampled inputs and no requester.
    // The exact divider recurrence covers the span; preserve every journal
    // entry in case a later memory callback introduces a continuation.
    if (!fast_irq_valid)
        cycle_sample_stable_inputs();
    cycle_defer_clocks(count);
    for (unsigned n = 0; n < count; ++n)
        history[history_count++] = {cycle_kind::internal, 0, 0, 1};
    quiet_record = history_count - 1;
    quiet_no_arbitration = no_arbitration;
    cursor += count;
    nominal += count;
    boundary_allowed = !no_arbitration;
    dma_allowed = true;
    dma_start_pending = false;
    refresh_after_sleep = false;
}

// Use certified physical memory only if the entire access fits the grant;
// otherwise retain the exact partial-cycle state for the normal bus engine.
// address is retained for replay; physical is the MMU-translated bus address.
// The wait query may revoke the fast path. In that case cycle.phase preserves
// the work already performed and the ordinary engine finishes the same access.
uint8_t Z180::cycle_mapped_access(cycle_kind kind, uint32_t address, uint32_t physical, uint8_t data) {
    cycle_flush_quiet();
    cycle = {};
    cycle.kind = kind;
    cycle.address = physical;
    cycle.data = data;
    cycle.active = true;
    cycle.waits = memory_wait_states();
    if (bus_wait)
        cycle.waits = std::max<unsigned>(cycle.waits, bus_wait(cycle.address, kind == cycle_kind::write, false));
    unsigned const waits = 2U * cycle.waits;
    if (!instruction_fast || icount < int(6 + waits) || waits > 65530) {
        instruction_whole = false;
        if (!cycle_timeline())
            cycle_run();
    } else {
        if (!fast_irq_valid)
            cycle_sample_stable_inputs();
        bool const fetch = kind == cycle_kind::fetch;
        cycle_advance_quiet((fetch ? 4 : 5) + waits);
        cycle.waits = 0;
        cycle.phase = fetch ? 5 : 6;
        cycle.sampled = true;
        auto const direct = kind != cycle_kind::write ? cycle_direct_read(cycle.address) : nullptr;
        if (direct)
            cycle.data = drive_direct_read(*direct);
        else
            switch (kind) {
            case cycle_kind::fetch:
                cycle.data = read_memory(cycle.address);
                break;
            case cycle_kind::operand:
                cycle.data = read_memory(cycle.address);
                break;
            case cycle_kind::read:
                cycle.data = read_memory(cycle.address);
                break;
            case cycle_kind::write:
                write_memory(cycle.address, data);
                break;
            default:
                break;
            }
        if (nmi_pending || bus_released || wait_line)
            instruction_fast = false;
        unsigned const tail = fetch ? 2 : 1;
        if (!instruction_fast || icount < int(tail)) {
            instruction_whole = false;
            cycle_run();
        } else {
            cycle_advance_quiet(tail);
            cycle.phase = 9;
            cycle.active = false;
        }
    }
    if (history_count == std::size(history))
        z180_error("Z180 instruction journal overflow at %04x", previous_pc);
    history[history_count++] = {kind, address, cycle.data, 3};
    ++cursor;
    nominal += 3;
    boundary_allowed = true;
    dma_allowed = !dma_start_pending;
    dma_start_pending = false;
    refresh_after_sleep = false;
    if (!instruction_fast)
        cycle_resume_admission();
    return cycle.data;
}

// Restore pending arbitration before resuming an instruction access.
void Z180::cycle_resume_admission() {
    // Replay and the actual pending cycle are finished. Re-prove the event
    // horizon for the instruction's remainder instead of keeping it slow
    // solely because an earlier scheduler grant ended. Invalidate, but do
    // not take a new pin sample here: a late IRQ after T2 must wait for the
    // next real Ti/memory cycle, if this instruction has one at all.
    if (icount >= 2 && !cycle_reference && !cycle_observer && !trapping && instruction_interrupt < 0 && !cycle.active &&
        cursor == history_count && boundary_allowed && !(dstat & 0xc0) && !(rcr & 0x80) && !(tcr & 3) &&
        !(dcntl & 0xf0) && !wait_line && !bus_released && !sampled_bus_request && !bus_ack && !nmi_pending &&
        !refresh_pending && dma_channel == 0xff && dma_continue == 0xff) {
        instruction_fast = true;
        fast_irq_valid = false;
    }
}

// Replay a completed bus operation from the journal or perform it once;
// this lets an ordinary ISA handler resume without duplicate device writes.
// address is the logical CPU address (ports remain 16-bit); data is the
// outgoing byte and the return is the sampled byte. no_arbitration suppresses
// a bus hand-off at this boundary where the instruction timing requires it.
// The journal contains completed accesses. Restoring CPU registers does not
// restore devices: replay returns the recorded byte without calling them.
uint8_t Z180::cycle_transaction(cycle_kind kind, uint32_t address, uint8_t data, bool no_arbitration) {
    if (cursor < history_count) {
        auto const &h = history[cursor++];
        if (h.kind != kind || h.address != address)
            z180_error("Z180 continuation diverged at %04x transaction %u", previous_pc, cursor - 1);
        nominal += h.nominal;
        return h.data;
    }
    if (!cycle.active || cycle.owner != cycle_owner::cpu || cycle.kind == cycle_kind::release) {
        // The instruction's stable-input path has no DMA/refresh requester.
        // Every I/O or external input mutation invalidates that proof. Keep
        // explicit arbitration when observing edges or replaying a suspension.
        if (!instruction_fast || cycle_observer || sampled_bus_request || refresh_pending)
            cycle_arbitrate();
        if (icount <= 0)
            cycle_suspend();
        uint32_t physical = address;
        if (kind == cycle_kind::fetch || kind == cycle_kind::operand || kind == cycle_kind::read ||
            kind == cycle_kind::write || kind == cycle_kind::nmi || kind == cycle_kind::halt)
            physical = MMU_REMAP_ADDR(address);
        cycle_begin(cycle_owner::cpu, kind, physical, data, no_arbitration);
        if (trapping && third_trap && kind == cycle_kind::read && bus_number == 5) {
            cycle.ttp = true;
            ++cycle.nominal;
        }
    }
    if (!cycle_timeline())
        cycle_run();
    if (history_count == std::size(history))
        z180_error("Z180 instruction journal overflow at %04x", previous_pc);
    history[history_count++] = {kind, address, cycle.data, cycle.nominal};
    ++cursor;
    nominal += cycle.nominal;
    boundary_allowed = !cycle.no_arbitration;
    dma_allowed = !dma_start_pending;
    dma_start_pending = false;
    refresh_after_sleep = false;
    uint8_t const result = cycle.data;
    if (!instruction_fast && kind <= cycle_kind::write)
        cycle_resume_admission();
#ifdef GS2_SS_TESTING
    if (cycle_reference)
        throw cycle_yield{};
#endif
    return result;
}

// Account for internal clocks left after explicit bus cycles, then retire
// the instruction at its interrupt/arbitration boundary.
void Z180::cycle_finish_cpu(int nominal_arg) {
    if (nominal > unsigned(nominal_arg))
        z180_error("Z180 cycle table overrun at %04x: %u > %u", previous_pc, nominal, nominal_arg);
    bool const admitted =
        instruction_whole || (!cycle_observer && !cycle.active && cursor == history_count && !trapping);
    if (admitted && instruction_fast && nominal < unsigned(nominal_arg) && icount >= int(2 * (nominal_arg - nominal)))
        cycle_whole_internal(nominal_arg - nominal);
    while (nominal < unsigned(nominal_arg))
        cycle_transaction(cycle_kind::internal, 0, 0);
    if (cursor != history_count)
        z180_error("Z180 unused continuation at %04x", previous_pc);
}

// Notify the board once for RETI (return from interrupt), even if its handler
// is replayed. A peripheral interrupt chain may release its in-service state.
void Z180::cycle_reti() {
    cycle_flush_quiet();
    if (!cycle_executing || !reti_notified) {
        if (bus.reti)
            bus.reti();
        reti_notified = true;
    }
}

// Check enabled/requested DMA admission without consuming the request edge.
// Zilog UM0050 pp. 95-109 distinguishes edge and level request modes.
bool Z180::cycle_dma_ready(unsigned ch) const {
    if (HALT == 2 || !(dstat & 1) || !(dstat & (0x40 << ch)))
        return false;
    unsigned route = 0;
    bool receive = false;
    if (!ch) {
        unsigned sm = (dmode >> 2) & 3, dm = (dmode >> 4) & 3;
        if (sm >= 2 && dm >= 2)
            return false;
        if (sm < 2 && dm < 2)
            return true;
        if (sm == 3) {
            route = (dma_sar0.d >> 16) & 3;
            receive = true;
        } else if (dm == 3)
            route = (dma_dar0.d >> 16) & 3;
    } else {
        route = (dma_iar1.d >> 16) & 7;
        receive = (dcntl & 2) != 0;
    }
    if (route == 1 || route == 2)
        return (asci[route - 1].dma_request(receive));
    if (route)
        return false; // DS006002 p57: selectors 3-7 are reserved.
    return (dcntl & (4 << ch)) ? (dma_edge_sampled[ch] != dma_edge_accepted[ch]) : bool(sampled_iol & (0x800 << ch));
}

// Latch a DMA byte transfer and its memory/I/O directions before taking
// bus ownership (UM0050 DMA modes, pp. 95-109).
void Z180::cycle_dma_begin(unsigned ch) {
    dma_channel = ch;
    dma_stage = 0;
    if (!ch) {
        dma_source_mode = (dmode >> 2) & 3;
        dma_destination_mode = (dmode >> 4) & 3;
        dma_source = dma_sar0.d;
        dma_destination = dma_dar0.d;
        dma_source_io = dma_source_mode == 3;
        dma_destination_io = dma_destination_mode == 3;
    } else {
        bool receive = (dcntl & 2) != 0;
        dma_source = receive ? dma_iar1.d : dma_mar1.d;
        dma_destination = receive ? dma_mar1.d : dma_iar1.d;
        dma_source_io = receive;
        dma_destination_io = !receive;
        dma_source_mode = receive ? 3 : (dcntl & 1);
        dma_destination_mode = receive ? (dcntl & 1) : 3;
    }
    unsigned const req = ch;
    if (dcntl & (4 << req))
        dma_edge_accepted[req] = dma_edge_sampled[req];
}

// Execute DMA read/write phases and update addresses/count after the byte.
// BUSREQ or refresh may suspend a burst without ending it (UM0050 pp. 105, 113).
// dma_stage=0 reads one byte; stage=1 writes that retained byte. Only the
// write completion decrements the byte count. Address arithmetic wraps at
// 20 bits; crossing a 64-KiB boundary costs the carry clock (UM0050 pp. 104-105).
void Z180::cycle_dma_run() {
    unsigned const ch = dma_channel;
    bool const writing = dma_stage != 0;
    bool const io = writing ? dma_destination_io : dma_source_io;
    uint32_t const address = writing ? dma_destination : dma_source;
    unsigned const mode = writing ? dma_destination_mode : dma_source_mode;
    if (!cycle.active) {
        cycle_begin(cycle_owner::dma,
                    io ? (writing ? cycle_kind::output : cycle_kind::input)
                       : (writing ? cycle_kind::write : cycle_kind::read),
                    io ? uint16_t(address) : address, dma_byte);
        if (mode < 2 && ((mode == 0 && (address & 0xffff) == 0xffff) || (mode == 1 && !(address & 0xffff)))) {
            cycle.trailing = 1;
            ++cycle.nominal;
        }
    }
    cycle_run();
    if (!writing)
        dma_byte = cycle.data;
    uint32_t const next = mode < 2 ? (address + (mode ? -1 : 1)) & 0xfffff : address;
    if (!ch) {
        if (writing)
            dma_dar0.d = next;
        else
            dma_sar0.d = next;
    } else if (!io)
        dma_mar1.d = next;
    boundary_allowed = true;
    if (!writing) {
        dma_stage = 1;
        return;
    }
    // A programmed zero count means 65536 bytes: the first decrement wraps
    // to 65535, and only the final decrement disables the channel (p. 96).
    --dma_bcr[ch].w;
    if (!dma_bcr[ch].w) {
        dstat &= ~(0x40 << ch);
    }
    // Memory-to-memory cycle steal returns one MC to the CPU. I/O DMA
    // continues while the sampled level/next edge requests another byte
    // (UM0050 pp105-108); DMODE.MM applies only to memory-to-memory.
    if (dma_nmi) {
        dstat &= ~1;
        dma_nmi = false;
        // Preserve the CPU's interrupt sampling point across DMA. A newly
        // latched NMI resumes and completes the interrupted CPU instruction
        // (Zilog AN0066 pp. 6-6, 6-8).
    }
    bool const memory = dma_source_mode < 2 && dma_destination_mode < 2;
    dma_continue = (!memory || (dmode & 2)) && cycle_dma_ready(ch) ? ch : 0xff;
    dma_channel = 0xff;
    dma_allowed = false;
}

// Choose external grant, refresh, DMA or CPU at a legal bus boundary.
// A suspended burst retains priority over CPU execution (UM0050 pp. 281-282).
// The loop returns only when the CPU may own the next cycle. Each competing
// owner either consumes clock budget, retains a partial cycle for suspension,
// or releases its request. It must not execute a CPU access inside a DMA burst.
void Z180::cycle_arbitrate() {
    cycle_flush_quiet();
    if (!boundary_allowed)
        return;
    for (;;) {
        if (cycle.active) {
            if (cycle.owner == cycle_owner::dma) {
                cycle_dma_run();
                continue;
            }
            if (cycle.owner == cycle_owner::refresh) {
                cycle_run();
                ++refresh_address;
                // Refresh takes the bus without consuming the suspended DMA
                // grant or the CPU breakpoint (UM0050 pp86-90,281-282).
                continue;
            }
        }
        if (sampled_bus_request || bus_ack) {
            if (!bus_ack) {
                bus_ack = true;
                refresh_pending = false;
                if (busack_cb)
                    busack_cb(true);
            }
            if (!cycle.active && !cycle_reference && !cycle_observer && bus_released && icount >= 2) {
                // BUSREQ is constant until this scheduler grant ends. Compose
                // its complete one-PHI release cycles with the exact divider
                // recurrences; leave an odd final half-PHI to the phase engine.
                // No IRQ is accepted or refresh cycle issued while BUSACK is
                // asserted. The next loop still constructs the pending cycle
                // at a zero budget, preserving its continuation state.
                unsigned const ticks = unsigned(icount) & ~1U;
                sampled_iol = iol;
                std::copy(std::begin(dma_edge_generation), std::end(dma_edge_generation), dma_edge_sampled);
                sampled_bus_request = true;
                icount -= ticks;
                half_phi += ticks;
                unsigned const phi = (phi_half + ticks) / 2;
                phi_half = (phi_half + ticks) & 1;
                handle_io_timers(phi);
                refresh_phase = (refresh_phase + ticks) % 160;
                cycle = {};
                cycle.kind = cycle_kind::release;
                cycle.nominal = 1;
                cycle.elapsed = 2;
                cycle.phase = 1;
            } else {
                if (!cycle.active)
                    cycle_begin(cycle_owner::cpu, cycle_kind::release, 0);
                if (!cycle_timeline())
                    cycle_run();
            }
            // UM0050 pp. 105, 113: BUSREQ suspends DMA.
            // Resume its grant after release. Clearing it here allows CPU
            // cycles inside a burst, including writes to the live DMA registers.
            if (!sampled_bus_request) {
                bus_ack = false;
                if (busack_cb)
                    busack_cb(false);
            }
            continue;
        }
        if (refresh_pending && (rcr & 0x80) && HALT != 2 && !refresh_after_sleep) {
            refresh_pending = false;
            cycle_begin(cycle_owner::refresh, cycle_kind::refresh, refresh_address);
            continue;
        }
        if (dma_channel != 0xff) {
            cycle_dma_run();
            continue;
        }
        if (dma_continue != 0xff) {
            unsigned const ch = dma_continue;
            dma_continue = 0xff;
            if (cycle_dma_ready(ch)) {
                cycle_dma_begin(ch);
                continue;
            }
        }
        if (dma_allowed) {
            if (cycle_dma_ready(0)) {
                cycle_dma_begin(0);
                continue;
            }
            if (cycle_dma_ready(1)) {
                cycle_dma_begin(1);
                continue;
            }
        }
        return;
    }
}

// Latch interrupt inputs at the documented final-cycle sampling edge,
// separately from DMA stopping on NMI (UM0050 pp. 73, 75, 81).
void Z180::cycle_sample_interrupts() {
    // UM0050 pp73,75,81: falling edge before T3/T1 of the final MC.
    // Sampling each CPU MC and retaining the last sample implements that
    // rule without predicting whether a conditional instruction will stop.
    sampled_interrupt = -1;
    if (nmi_pending) {
        sampled_interrupt = 1;
        return;
    }
    // Priority encoder: lower-priority sources need not be evaluated once
    // a higher-priority asserted source fixes the result.
    if (irq_state[0] && (itc & 1))
        sampled_interrupt = 2;
    else if (irq_state[1] && (itc & 2))
        sampled_interrupt = 3;
    else if (irq_state[2] && (itc & 4))
        sampled_interrupt = 4;
    else if (!(iocr & 0x20) && (tcr & 0x50) == 0x50)
        sampled_interrupt = 5;
    else if (!(iocr & 0x20) && (tcr & 0xa0) == 0xa0)
        sampled_interrupt = 6;
    else if ((dstat & 0x44) == 4)
        sampled_interrupt = 7;
    else if ((dstat & 0x88) == 8)
        sampled_interrupt = 8;
    else if (!(iocr & 0x20) && csio.check_interrupt())
        sampled_interrupt = 9;
    else if (!(iocr & 0x20) && asci[0].check_interrupt())
        sampled_interrupt = 10;
    else if (!(iocr & 0x20) && asci[1].check_interrupt())
        sampled_interrupt = 11;
}

// Resolve enabled interrupt sources in the Z180 priority order.
// Return a priority index, or -1 if no interrupt may start. EI defers acceptance
// through the following instruction. SLP can wake from a masked interrupt
// without taking its handler (UM0050 pp. 34-36, 68-69).
int Z180::cycle_choose_interrupt() {
    if (after_EI || interrupt_recovery)
        return -1;
    int const irq = sampled_interrupt;
    if (irq >= 0 && HALT == 2) {
        refresh_after_sleep = true;
        LEAVE_HALT();
    }
    return irq == 1 || IFF1 ? irq : -1;
}

// Perform acknowledge/vector and stack cycles for the selected interrupt.
// Zilog UM0050 Tables 51-52 distinguishes IM0, IM1, IM2 and NMI sequences.
int Z180::cycle_interrupt() {
    int const irq = instruction_interrupt;
    LEAVE_HALT();
    if (irq == 1) {
        IFF2 = IFF1;
        IFF1 = 0;
        if (!interrupt_started) {
            interrupt_started = true;
            dstat &= ~1;
        }
        cycle_transaction(cycle_kind::nmi, pc16, 0);
        cycle_transaction(cycle_kind::internal, 0, 0);
        cycle_transaction(cycle_kind::internal, 0, 0);
        PUSH(PC);
        pc16 = 0x66;
        return 11;
    }
    IFF1 = IFF2 = 0;
    uint8_t const vector = cycle_transaction(cycle_kind::acknowledge, pc16, 0);
    if (irq == 2 && IM == 0) {
        // UM0050 Table52 p279: subsequent operands are memory reads at PC,
        // PC+1; an inserted CALL stacks PC+2. Only the initial byte is ACK.
        ++R;
        ++bus_number;
        int const nominal_arg = exec_op(vector);
        return nominal_arg + extra_cycles + 2;
    }
    if (irq == 2 && IM == 1) {
        PUSH(PC);
        pc16 = 0x38;
        return 11;
    }
    cycle_transaction(cycle_kind::internal, 0, 0);
    unsigned const low = irq == 2 ? vector : ((il & 0xe0) + (irq - 3) * 2) & 0xff;
    PUSH(PC);
    RM16((I << 8) | low, &PC);
    return 18;
}

// Execute register-only instruction spans only while pins, mapping and
// peripheral deadlines remain stable; otherwise use the ordinary bus engine.
inline int Z180::cycle_direct_instruction() {
    if (!instruction_fast || history_count || !direct_reads_enabled || cycle_reference || cycle_observer || icount < 8)
        return 0;
    uint16_t pc = pc16;
    auto const page = direct_logical[pc >> 12];
    if (!page)
        return 0;
    unsigned op = page[pc & 4095];
    // These address-building instructions have no I/O or control changes.
    // Admit all fetch/data/Ti edges together, using the original handlers,
    // cycle table and journal. A mapped byte or short horizon falls back.
    auto registers = [](unsigned op) {
        return op == 0x29 || op == 0x6f || op == 0xd9 || op == 0x13 || op == 0x23 || op == 0x0d || op == 0x2d;
    };
    if (!registers(op) && op != 0x26 && op != 0x0a)
        return 0;
    unsigned duration = cc[Z180_TABLE_op][op];
    if (icount < int(2 * duration))
        return 0;
    if (op == 0x26 && !direct_logical[uint16_t(pc + 1) >> 12])
        return 0;
    if (op == 0x0a && !direct_logical[bc16 >> 12])
        return 0;
    opcode = op;
    instruction_whole = true;
    ++R;
    ++pc16;
    bus_number = 1;
    if (registers(op)) {
        // The certified fetch and trailing Ti span have constant sampled
        // inputs and no callback. Compose their clocks and journal once;
        // the original handler remains the only register/flag arithmetic.
        if (!fast_irq_valid)
            cycle_sample_stable_inputs();
        unsigned longest = duration, span = duration, count = 1;
        for (;;) {
            // No register-only handler changes pins, IFF or the memory map.
            // Look ahead only within the certified external-event horizon.
            uint16_t const next_pc = pc + 1;
            auto const next_page = direct_logical[next_pc >> 12];
            unsigned const next_op = next_page ? next_page[next_pc & 4095] : 0;
            unsigned const next_duration = cc[Z180_TABLE_op][next_op];
            bool const more = batch_registers && !trace && !sampled_bus_request && !refresh_pending && !bus_ack &&
                              !(sampled_interrupt >= 0 && (sampled_interrupt == 1 || IFF1)) && next_page &&
                              registers(next_op) && icount >= int(2 * (span + next_duration));
            if (!more && count > 1) {
                // Intermediate snapshots cannot be observed inside this span.
                // Retain exactly the last instruction's pre-fetch registers.
                cycle_save_state();
                instruction_start.regs[0] = (PC.d & 0xffff0000U) | pc;
                instruction_start.control[0] = uint8_t(R + count - 2);
                previous_pc = instruction_start.regs[0];
            }
            switch (op) {
            case 0x29:
                op_29();
                break;
            case 0x6f:
                op_6f();
                break;
            case 0xd9:
                op_d9();
                break;
            case 0x13:
                op_13();
                break;
            case 0x23:
                op_23();
                break;
            case 0x0d:
                op_0d();
                break;
            case 0x2d:
                op_2d();
                break;
            }
            if (!more)
                break;
            ++count;
            pc = next_pc;
            op = next_op;
            duration = next_duration;
            span += duration;
            longest = std::max(longest, duration);
        }
        cycle_defer_clocks(span);
        retired += count - 1;
        fast_instructions += count - 1;
        pc16 = pc + 1;
        R += count - 1;
        opcode = op;
        dma_allowed = count > 1 || duration > 3 || !dma_start_pending;
        dma_start_pending = false;
        // Only the last completed instruction's journal is observable. Keep
        // earlier Ti entries beyond its length too: checkpoints retain them.
        drive_direct_read(op);
        history[0] = {cycle_kind::fetch, pc, uint8_t(op), 3};
        for (unsigned n = 3; n < longest; ++n)
            history[n - 2] = {cycle_kind::internal, 0, 0, 1};
        history_count = duration - 2;
        quiet_record = history_count - 1;
        quiet_no_arbitration = false;
        cursor = history_count;
        nominal = duration;
        boundary_allowed = true;
        refresh_after_sleep = false;
        return duration;
    }
    cycle_certified_read(cycle_kind::fetch, pc, page + (pc & 4095));
    switch (op) {
    case 0x26:
        op_26();
        break;
    case 0x0a:
        op_0a();
        break;
    }
    if (nominal < duration)
        cycle_whole_internal(duration - nominal);
    return duration;
}

// Certify the stable memory and timing conditions needed to use the LDD shortcut.
inline bool Z180::cycle_ldd_ready() const {
    if (cycle_reference || cycle_observer)
        return false;
    uint16_t const pc = pc16;
    if (history_count >= 2)
        return history[0].kind == cycle_kind::fetch && history[0].address == pc && history[0].data == 0xed &&
               history[1].kind == cycle_kind::fetch && history[1].address == uint16_t(pc + 1) &&
               history[1].data == 0xa8;
    if (history_count || !instruction_fast || !direct_reads_enabled || icount < 12 || (pc & 4095) == 4095)
        return false;
    auto const page = direct_logical[pc >> 12];
    return page && page[pc & 4095] == 0xed && page[(pc & 4095) + 1] == 0xa8;
}

// Advance a backward-copy instruction only through the next observable
// event; instruction flags and register wrapping match individual LDD steps.
void Z180::cycle_ldd() {
    uint16_t const pc = pc16;
    if (!history_count) {
        // Certified consecutive M1s have no intervening observable event.
        // Compose their clocks but retain both entries for a later yield.
        if (!fast_irq_valid)
            cycle_sample_stable_inputs();
        drive_direct_read(0xa8);
        cycle_defer_clocks(6);
        history[0] = {cycle_kind::fetch, pc, 0xed, 3};
        history[1] = {cycle_kind::fetch, uint16_t(pc + 1), 0xa8, 3};
        history_count = 2;
        quiet_record = 1;
        quiet_no_arbitration = false;
        boundary_allowed = true;
        dma_allowed = true;
        dma_start_pending = false;
        refresh_after_sleep = false;
    }
    // On continuation, both M1s are already in the journal. Recreate their
    // register/decode effects without redispatching two replayed accesses.
    pc16 += 2;
    R += 2;
    cursor = bus_number = 2;
    nominal = 6;
    cycle_decode(Z180_TABLE_ed, 0xa8);
    auto access = [this](cycle_kind kind, uint16_t address, uint8_t data = 0) {
        // LDD has no Ti prelude. The actual data cycles still use the common
        // wait query, callback edge and resumable arbitration path.
        if (instruction_fast && (instruction_whole || (!cycle.active && cursor == history_count)) && icount >= 6)
            return cycle_whole_access(kind, address, data, 0);
        ++bus_number;
        return cycle_transaction(kind, address, data);
    };
    uint8_t const io = access(cycle_kind::read, hl16);
    access(cycle_kind::write, de16, io);
    ldd_registers(io);
    extra_cycles += cc[Z180_TABLE_ed][0xa8];
}

// Run resumable instructions, low-power states and DMA within the supplied
// half-PHI budget; meet the board before crossing any external deadline.
// The saved jump target belongs to this call. Opcode helpers use only trivial
// stack locals across a suspension; persistent registers, sampled data and
// partial-cycle state live in Z180. A resumed handler starts at its beginning
// with the saved registers and replays its completed journal entries.
// Each iteration retires an instruction, advances an idle/bus cycle, or leaves
// when the half-PHI budget is exhausted. No external deadline may be crossed.
void Z180::execute_machine_cycles() {
    // One target per scheduler invocation, rather than C++ exception-table
    // searches on every half-PHI suspension. Resume uses the same journal.
    if (Z180_CONTINUATION_SAVE(cycle_resume))
        cycle_executing = false;
    // Debugger/non-mutating execution keeps the address-space handlers.
    // This context belongs to the scheduler grant, not a hardware edge.
    direct_reads_enabled = !side_effects_disabled;
    // Peripheral timers and inputs may have run since the previous slice,
    // including while HALT/SLP had no active instruction to restart.
    fast_irq_valid = false;
    instruction_fast = false;
    while (icount > 0) {
#ifdef GS2_SS_TESTING
        try {
#endif
            bool const resuming = instruction_active;
            if (!instruction_active) {
                if (run_low_power())
                    continue;
                // Finish an idle HALT/SLP cycle before testing interrupts.
                if (cycle.active && cycle.owner == cycle_owner::cpu && cycle.kind != cycle_kind::release) {
                    cycle_run();
                    boundary_allowed = true;
                    dma_allowed = HALT != 2;
                }
                if (!instruction_fast || cycle_observer || sampled_bus_request || refresh_pending)
                    cycle_arbitrate();
                if (icount <= 0) {
                    cycle_flush_quiet();
                    return;
                }
                instruction_interrupt = cycle_choose_interrupt();
                if (instruction_interrupt == 1)
                    nmi_pending = 0;
                if (instruction_interrupt < 0 && HALT) {
                    if (skip_machine_idle())
                        continue;
                    if (HALT == 2) {
                        refresh_after_sleep = true;
                        if (!cycle.active)
                            cycle_begin(cycle_owner::cpu, cycle_kind::internal, 0);
                    } else if (!cycle.active)
                        cycle_begin(cycle_owner::cpu, cycle_kind::halt, MMU_REMAP_ADDR(uint16_t(pc16 + 1)));
                    cycle_run();
                    boundary_allowed = true;
                    dma_allowed = HALT != 2;
                    continue;
                }
                if (instruction_interrupt < 0 && skip_machine_poll())
                    continue;
                after_EI = 0;
                cycle_save_state();
                instruction_active = true;

                history_count = 0;
                ack_vector = -1;
                reti_notified = trap_started = interrupt_started = false;
                if (instruction_interrupt < 0 && (trace != nullptr)) {
                    cycle_flush_quiet();
                    if (trace)
                        trace(get_state());
                    // Debugger state entries can change divider/pin registers
                    // without passing through the CPU's external-input API.
                    instruction_fast = false;
                }
                // The scheduler budget is the horizon for external pin/timer
                // events. Keep the handler running once through that horizon;
                // cycle_run checks the remaining budget before each unrolled
                // MC. A deadline or handler-triggered event yields a journaled
                // continuation rather than speculating past the event.
                if (instruction_fast && !cycle_observer) {
                    // No callback or timed source has changed these inputs
                    // since the last admitted instruction. Only the budget and
                    // instruction-boundary interrupt decision are new.
                    instruction_fast = instruction_interrupt < 0 && icount >= 6;
                } else {
                    fast_irq_valid = false;
                    instruction_fast = !cycle_reference && instruction_interrupt < 0 && icount >= 6 &&
                                       !(dstat & 0xc0) && !(rcr & 0x80) && !(tcr & 3) && !(dcntl & 0xf0) &&
                                       !wait_line && !bus_released && !nmi_pending;
                }
                if (instruction_fast)
                    ++fast_instructions;
            } else {
                ++resumptions;

                instruction_fast = false;
            }
            instruction_whole = false;
            if (resuming || (trace != nullptr))
                cycle_restore_state();
            else {
                previous_pc = instruction_start.regs[0];
                cursor = bus_number = nominal = 0;
                family = Z180_TABLE_op;
                opcode = 0;
                extra_cycles = 0;
                prelude_schedule = 0;
                trapping = third_trap = false;
            }
            cycle_executing = true;
            int nominal_arg;
            if (instruction_interrupt >= 0)
                nominal_arg = cycle_interrupt();
            else if ((nominal_arg = cycle_direct_instruction()) != 0) {
                // The admitted instruction completed within this scheduler grant.
            } else if (cycle_ldd_ready()) {
                cycle_ldd();
                nominal_arg = extra_cycles;
            } else {
                uint8_t opcode_arg;
                if (instruction_fast && !cycle_observer) {
                    ++R;
                    uint16_t const pc = pc16++;
                    opcode_arg = cycle_whole_access(cycle_kind::fetch, pc, 0, 0);
                } else
                    opcode_arg = ROP();
                nominal_arg = exec_op(opcode_arg);
                nominal_arg += extra_cycles;
            }
            // When both counts already match, finish_cpu only rechecks them.
            // Keep its internal-cycle and error paths for every other case.
            if (nominal != unsigned(nominal_arg) || cursor != history_count)
                cycle_finish_cpu(nominal_arg);
            cycle_executing = false;
            instruction_active = false;
            ++retired;
            interrupt_recovery = instruction_interrupt >= 0;
            if (cycle_observer)
                cycle_observer(half_phi, 0, 6, pc16, 0);

#ifdef GS2_SS_TESTING
        } catch (cycle_yield const &) {
            cycle_executing = false;
        }
#endif
    }
    cycle_flush_quiet();
}

#undef Z180_CONTINUATION_SAVE
#undef Z180_CONTINUATION_RESTORE

// The card supplies scheduler deadlines; local serial timers shorten each grant.
Z180::Z180(Bus callbacks, uint32_t extal, Silicon revision)
    : bus(std::move(callbacks)), extal_hz(extal), silicon(revision) {
    // Flag tables are process-wide and immutable after initialization. Several
    // card instances may be created on different threads; call_once publishes
    // the finished tables before any instance executes an instruction.
    static std::once_flag initialized;
    std::call_once(initialized, init_flags);
    auto now = [this] { return time(); };
    for (auto &a : asci) {
        a.ext = silicon == Silicon::N;
        a.bind_time(now);
    }
    csio.bind_time(now);
    if (bus.extra_wait)
        set_bus_wait([this](uint32_t a, bool write, bool io) {
            return bus.extra_wait(io ? (write ? Access::IOWrite : Access::IORead)
                                     : (write ? Access::MemoryWrite : Access::MemoryRead),
                                  a);
        });
#ifdef GS2_SS_TESTING
    cycle_engine = true;
#endif
    reset();
}

// Return the active PHI frequency in Hz, after CMR/CCR clock selection
// (UM0050 pp. 49-52). EXTAL is the external oscillator input frequency.
uint32_t Z180::clock_hz() const {
    return clock_multiplier_enabled() ? extal_hz * 2 : (ccr & 0x80) ? extal_hz : extal_hz / 2;
}
// Convert EXTAL clocks to the active execution clock using CMR/CCR divisors.
uint64_t Z180::clocks_to_cycles(uint64_t clocks) const {
    return cycle_engine                 ? clock_multiplier_enabled() ? clocks * 4
                                          : (ccr & 0x80)             ? clocks * 2
                                                                     : clocks
           : clock_multiplier_enabled() ? clocks * 2
           : (ccr & 0x80)               ? clocks
                                        : (clocks + 1) / 2;
}
// Invert the active clock-divider conversion without changing its epoch.
uint64_t Z180::cycles_to_clocks(uint64_t ticks) const {
    return cycle_engine                 ? clock_multiplier_enabled() ? (ticks + 3) / 4
                                          : (ccr & 0x80)             ? (ticks + 1) / 2
                                                                     : ticks
           : clock_multiplier_enabled() ? (ticks + 1) / 2
           : (ccr & 0x80)               ? ticks
                                        : ticks * 2;
}
// Invalidate derived time after restoring counters or changing clock rate.
void Z180::reset_time_cache() {
    time_base = epoch_time;
    time_half = epoch_half;
    half_period = epoch_rate ? Z180Time::units_per_second / epoch_rate : 0;
}
// Derive elapsed time from the current oscillator epoch without accumulating
// rounding error per instruction.
Z180Time Z180::update_time_cache() const {
    if (!epoch_rate)
        return Z180Time::never;
    uint64_t const elapsed = half_phi - epoch_half, seconds = elapsed / epoch_rate;
    time_half = epoch_half + seconds * epoch_rate;
    time_base = epoch_time + Z180Time(seconds);
    return time_base + Z180Time(0, (half_phi - time_half) * half_period);
}
// Convert execution ticks to an elapsed emulated duration. Ticks are half-PHI
// in the cycle engine and whole PHI in the optional instruction engine.
Z180Time Z180::cycles_to_time(uint64_t n) const { return Z180Time::from_ticks(n, clocks_to_cycles(extal_hz)); }
// Return complete execution ticks contained in duration t; round down so
// the caller cannot grant execution beyond its deadline.
uint64_t Z180::time_to_cycles(Z180Time t) const { return t.floor_ticks(clocks_to_cycles(extal_hz)); }
// Save time under the old divider before a register write changes clock rate.
void Z180::prepare_deferred_clock_change() {
    epoch_time = time();
    epoch_half = half_phi;
    reset_time_cache();
}
// Rebase the CPU time epoch and serial clocks, then tell the board to
// recompute waits that depend on PHI.
void Z180::notify_clock_changed() {
    bus_release_at = UINT64_MAX;
    epoch_rate = clock_hz() * 2;
    reset_time_cache();
    attoseconds_per_cycle = Z180Time::units_per_second / clocks_to_cycles(extal_hz);
    for (auto &a : asci)
        a.set_clock(clock_hz());
    csio.set_clock(clock_hz());
    if (bus.clock_changed)
        bus.clock_changed(clock_hz());
    if (executing)
        request_yield();
}
// Reset CPU execution and peripheral clocks together; board-retained latches
// and SRAM are owned by SSCard rather than this processor reset.
void Z180::reset() {
    Z180Time const now = epoch_rate ? time() : Z180Time::zero;
    reset_core();
    power_mode = 0;
    wake_remaining = 0;
    epoch_time = now;
    epoch_half = 0;
    epoch_rate = extal_hz;
    reset_time_cache();
    cmr = ccr = 0;
    for (auto &a : asci)
        a.reset();
    csio.reset();
    notify_clock_changed();
}
// Run to an absolute half-PHI budget, retaining an unfinished instruction
// when a board event cuts through its machine cycle.
void Z180::run_half_until(uint64_t target) {
    assert(!executing);
    executing = true;
    run_target = target;
    yield_requested = false;
    while (half_phi < target && !yield_requested) {
        if (scheduler) {
            if (!refill_budget())
                break;
            execute_run();
            continue;
        }
        auto next = std::min({asci[0].next_event(), asci[1].next_event(), csio.next_event()});
        uint64_t span = std::min<uint64_t>(target - half_phi, 0x3fffffff);
        if (!next.is_never()) {
            auto now = time();
            if (next <= now) {
                for (auto &a : asci)
                    a.dispatch(now);
                csio.dispatch(now);
                continue;
            }
            auto delta = next - now;
            uint64_t ticks = delta.ceil_ticks(epoch_rate);
            span = std::min(span, ticks);
        }
        icount = int(span);
        uint64_t before = half_phi;
        execute_run();
        if (!cycle_engine)
            half_phi += 2 * (int(span) - icount);
        assert(half_phi > before || yield_requested);
    }
    executing = false;
}

// Meet the board scheduler at the current deadline and accept its next
// grant without recursively entering a suspended instruction handler.
bool Z180::refill_budget() {
    cycle_flush_quiet();
    fast_irq_valid = false;
    instruction_fast = false;
    yield_requested = false;
    uint64_t limit = std::min(run_target, scheduler());
    if (limit <= half_phi) {
        yield_requested = true;
        icount = 0;
        return false;
    }
    // A future returned horizon admits the callback's input changes. An
    // explicit zero-length horizon above remains the scheduler's exit rule.
    yield_requested = false;
    auto next = std::min({asci[0].next_event(), asci[1].next_event(), csio.next_event()});
    if (!next.is_never()) {
        auto now = time();
        if (next <= now) {
            for (auto &a : asci)
                a.dispatch(now);
            csio.dispatch(now);
            next = std::min({asci[0].next_event(), asci[1].next_event(), csio.next_event()});
        }
        if (!next.is_never())
            limit = std::min(limit, half_phi + (next - now).ceil_ticks(epoch_rate));
    }
    scheduler_limit = half_phi + std::min<uint64_t>(limit - half_phi, 0x3fffffff);
    icount = int(std::min(scheduler_limit, bus_release_at) - half_phi);
    return icount > 0;
}

// Arrange the copy controller BUSREQ release inside a granted interval
// so a batch need not return at every idle half-PHI.
bool Z180::schedule_bus_release(uint64_t half_deadline) {
    if (!scheduled_timeline || !scheduler || !executing || cycle_reference || cycle_observer || power_mode ||
        !bus_ack || !bus_release_complete || half_deadline <= half_phi)
        return false;
    bus_release_at = half_deadline;
    if (half_deadline - half_phi < uint64_t(std::max(icount, 0)))
        icount = int(half_deadline - half_phi);
    return true;
}
// Execute one instruction for debugging/reference use, respecting active bus state.
void Z180::step() {
    uint64_t const goal = retired + 1, stop = half_phi + 2'000'000;
    do {
        run_half_until(half_phi + 1);
        if (!instruction_active && HALT)
            break;
        if (bus_ack)
            break;
    } while (retired < goal && half_phi < stop);
}
// Translate the public pin name to the internal input index. asserted is the
// logical active state, not voltage; a new NMI assertion is latched once.
void Z180::set_input(Input input, bool asserted) {
    constexpr int lines[] = {Z180_INPUT_LINE_IRQ0, Z180_INPUT_LINE_IRQ1,  Z180_INPUT_LINE_IRQ2,
                             INPUT_LINE_NMI,       Z180_INPUT_LINE_DREQ0, Z180_INPUT_LINE_DREQ1};
    execute_set_input(lines[unsigned(input)], asserted);
}
// Export architectural registers separately from the internal continuation journal.
Z180::State Z180::get_state() const {
    return {PC.w.l,
            SP.w.l,
            AF.w.l,
            BC.w.l,
            DE.w.l,
            HL.w.l,
            IX.w.l,
            IY.w.l,
            AF2.w.l,
            BC2.w.l,
            DE2.w.l,
            HL2.w.l,
            uint8_t((R & 0x7f) | (R2 & 0x80)),
            I,
            IM,
            IFF1,
            IFF2,
            HALT,
            cycles()};
}
// Import debugger registers and invalidate execution assumptions tied to the
// previous PC, interrupt state or address mapping.
void Z180::set_state(const State &s) {
    PC.w.l = s.pc;
    SP.w.l = s.sp;
    AF.w.l = s.af;
    BC.w.l = s.bc;
    DE.w.l = s.de;
    HL.w.l = s.hl;
    IX.w.l = s.ix;
    IY.w.l = s.iy;
    AF2.w.l = s.af2;
    BC2.w.l = s.bc2;
    DE2.w.l = s.de2;
    HL2.w.l = s.hl2;
    R = s.r & 0x7f;
    R2 = s.r & 0x80;
    I = s.i;
    IM = s.im;
    IFF1 = s.iff1;
    IFF2 = s.iff2;
    HALT = s.halt;
    if (cycle_engine && instruction_active) {
        instruction_start.control[2] = IFF1;
        instruction_start.control[3] = IFF2;
        instruction_start.control[4] = HALT;
    }
    instruction_fast = false;
}

#include <istream>
#include <ostream>
// Serialize CPU/peripheral state and continuation for same-build checkpoints.
// Callbacks and host pointers belong to the embedding board, not the stream.
// Return 0 on success or 1 on stream failure. Field order and native scalar
// representation form a same-build format, not a portable hardware image.
int Z180::save(std::ostream &stream) {
    cycle_flush_quiet();
    uint32_t const format = 0x5a313802, revision = uint32_t(silicon);
    stream.write(reinterpret_cast<const char *>(&format), sizeof(format));
    stream.write(reinterpret_cast<const char *>(&revision), sizeof(revision));
    stream.write(reinterpret_cast<const char *>(&power_mode), sizeof(power_mode));
    stream.write(reinterpret_cast<const char *>(&wake_remaining), sizeof(wake_remaining));
    stream.write(reinterpret_cast<const char *>(&cmr), sizeof(cmr));
    stream.write(reinterpret_cast<const char *>(&ccr), sizeof(ccr));
    epoch_time.save(stream);
    stream.write(reinterpret_cast<const char *>(&epoch_half), sizeof(epoch_half));
    stream.write(reinterpret_cast<const char *>(&epoch_rate), sizeof(epoch_rate));
    stream.write(reinterpret_cast<const char *>(&attoseconds_per_cycle), sizeof(attoseconds_per_cycle));
    // Architectural registers and interrupt inputs precede on-chip peripherals.
    stream.write(reinterpret_cast<const char *>(&AF.w.l), sizeof(AF.w.l));
    stream.write(reinterpret_cast<const char *>(&BC.w.l), sizeof(BC.w.l));
    stream.write(reinterpret_cast<const char *>(&DE.w.l), sizeof(DE.w.l));
    stream.write(reinterpret_cast<const char *>(&HL.w.l), sizeof(HL.w.l));
    stream.write(reinterpret_cast<const char *>(&IX.w.l), sizeof(IX.w.l));
    stream.write(reinterpret_cast<const char *>(&IY.w.l), sizeof(IY.w.l));
    stream.write(reinterpret_cast<const char *>(&PC.w.l), sizeof(PC.w.l));
    stream.write(reinterpret_cast<const char *>(&SP.w.l), sizeof(SP.w.l));
    stream.write(reinterpret_cast<const char *>(&AF2.w.l), sizeof(AF2.w.l));
    stream.write(reinterpret_cast<const char *>(&BC2.w.l), sizeof(BC2.w.l));
    stream.write(reinterpret_cast<const char *>(&DE2.w.l), sizeof(DE2.w.l));
    stream.write(reinterpret_cast<const char *>(&HL2.w.l), sizeof(HL2.w.l));
    stream.write(reinterpret_cast<const char *>(&R), sizeof(R));
    stream.write(reinterpret_cast<const char *>(&R2), sizeof(R2));
    stream.write(reinterpret_cast<const char *>(&IFF1), sizeof(IFF1));
    stream.write(reinterpret_cast<const char *>(&IFF2), sizeof(IFF2));
    stream.write(reinterpret_cast<const char *>(&HALT), sizeof(HALT));
    stream.write(reinterpret_cast<const char *>(&IM), sizeof(IM));
    stream.write(reinterpret_cast<const char *>(&I), sizeof(I));
    stream.write(reinterpret_cast<const char *>(&nmi_state), sizeof(nmi_state));
    stream.write(reinterpret_cast<const char *>(&nmi_pending), sizeof(nmi_pending));
    stream.write(reinterpret_cast<const char *>(&irq_state), sizeof(irq_state));
    stream.write(reinterpret_cast<const char *>(&int_pending), sizeof(int_pending));
    stream.write(reinterpret_cast<const char *>(&frc_prescale), sizeof(frc_prescale));
    stream.write(reinterpret_cast<const char *>(&bus_released), sizeof(bus_released));
    stream.write(reinterpret_cast<const char *>(&after_EI), sizeof(after_EI));
    stream.write(reinterpret_cast<const char *>(&read_tcr_tmdr), sizeof(read_tcr_tmdr));
    stream.write(reinterpret_cast<const char *>(&tmdr_value), sizeof(tmdr_value));
    stream.write(reinterpret_cast<const char *>(&tmdrh), sizeof(tmdrh));
    stream.write(reinterpret_cast<const char *>(&tmdr_latch), sizeof(tmdr_latch));
    stream.write(reinterpret_cast<const char *>(&prt1_timeout_odd), sizeof(prt1_timeout_odd));
    stream.write(reinterpret_cast<const char *>(&tout_level), sizeof(tout_level));
    stream.write(reinterpret_cast<const char *>(&tmdr[0].w), sizeof(tmdr[0].w));
    stream.write(reinterpret_cast<const char *>(&tmdr[1].w), sizeof(tmdr[1].w));
    stream.write(reinterpret_cast<const char *>(&rldr[0].w), sizeof(rldr[0].w));
    stream.write(reinterpret_cast<const char *>(&rldr[1].w), sizeof(rldr[1].w));
    stream.write(reinterpret_cast<const char *>(&tcr), sizeof(tcr));
    stream.write(reinterpret_cast<const char *>(&frc), sizeof(frc));
    stream.write(reinterpret_cast<const char *>(&dma_sar0.d), sizeof(dma_sar0.d));
    stream.write(reinterpret_cast<const char *>(&dma_dar0.d), sizeof(dma_dar0.d));
    stream.write(reinterpret_cast<const char *>(&dma_bcr[0].w), sizeof(dma_bcr[0].w));
    stream.write(reinterpret_cast<const char *>(&dma_bcr[1].w), sizeof(dma_bcr[1].w));
    stream.write(reinterpret_cast<const char *>(&dma_mar1.d), sizeof(dma_mar1.d));
    stream.write(reinterpret_cast<const char *>(&dma_iar1.d), sizeof(dma_iar1.d));
    stream.write(reinterpret_cast<const char *>(&dstat), sizeof(dstat));
    stream.write(reinterpret_cast<const char *>(&dmode), sizeof(dmode));
    stream.write(reinterpret_cast<const char *>(&dcntl), sizeof(dcntl));
    stream.write(reinterpret_cast<const char *>(&il), sizeof(il));
    stream.write(reinterpret_cast<const char *>(&itc), sizeof(itc));
    stream.write(reinterpret_cast<const char *>(&rcr), sizeof(rcr));
    stream.write(reinterpret_cast<const char *>(&mmu_cbr), sizeof(mmu_cbr));
    stream.write(reinterpret_cast<const char *>(&mmu_bbr), sizeof(mmu_bbr));
    stream.write(reinterpret_cast<const char *>(&mmu_cbar), sizeof(mmu_cbar));
    stream.write(reinterpret_cast<const char *>(&omcr), sizeof(omcr));
    stream.write(reinterpret_cast<const char *>(&iocr), sizeof(iocr));
    stream.write(reinterpret_cast<const char *>(&iol), sizeof(iol));
    stream.write(reinterpret_cast<const char *>(&ioltemp), sizeof(ioltemp));
    stream.write(reinterpret_cast<const char *>(&mmu), sizeof(mmu));
    // Preserve the partially completed bus cycle and its replay journal.
    // These fields allow a checkpoint between PHI edges, not just instructions.
    stream.write(reinterpret_cast<const char *>(&cycle_reference), sizeof(cycle_reference));
    stream.write(reinterpret_cast<const char *>(&cycle_executing), sizeof(cycle_executing));
    stream.write(reinterpret_cast<const char *>(&instruction_fast), sizeof(instruction_fast));
    stream.write(reinterpret_cast<const char *>(&instruction_whole), sizeof(instruction_whole));
    stream.write(reinterpret_cast<const char *>(&fast_irq_valid), sizeof(fast_irq_valid));
    stream.write(reinterpret_cast<const char *>(&fast_interrupt), sizeof(fast_interrupt));
    stream.write(reinterpret_cast<const char *>(&instruction_active), sizeof(instruction_active));
    stream.write(reinterpret_cast<const char *>(&wait_line), sizeof(wait_line));
    stream.write(reinterpret_cast<const char *>(&bus_ack), sizeof(bus_ack));
    stream.write(reinterpret_cast<const char *>(&sampled_bus_request), sizeof(sampled_bus_request));
    stream.write(reinterpret_cast<const char *>(&boundary_allowed), sizeof(boundary_allowed));
    stream.write(reinterpret_cast<const char *>(&dma_allowed), sizeof(dma_allowed));
    stream.write(reinterpret_cast<const char *>(&dma_start_pending), sizeof(dma_start_pending));
    stream.write(reinterpret_cast<const char *>(&refresh_pending), sizeof(refresh_pending));
    stream.write(reinterpret_cast<const char *>(&refresh_after_sleep), sizeof(refresh_after_sleep));
    stream.write(reinterpret_cast<const char *>(&dma_edge), sizeof(dma_edge));
    stream.write(reinterpret_cast<const char *>(&sampled_iol), sizeof(sampled_iol));
    stream.write(reinterpret_cast<const char *>(&dma_edge_generation), sizeof(dma_edge_generation));
    stream.write(reinterpret_cast<const char *>(&dma_edge_accepted), sizeof(dma_edge_accepted));
    stream.write(reinterpret_cast<const char *>(&dma_edge_sampled), sizeof(dma_edge_sampled));
    stream.write(reinterpret_cast<const char *>(&phi_half), sizeof(phi_half));
    stream.write(reinterpret_cast<const char *>(&refresh_address), sizeof(refresh_address));
    stream.write(reinterpret_cast<const char *>(&refresh_phase), sizeof(refresh_phase));
    stream.write(reinterpret_cast<const char *>(&advance_left), sizeof(advance_left));
    stream.write(reinterpret_cast<const char *>(&half_phi), sizeof(half_phi));
    stream.write(reinterpret_cast<const char *>(&quiet_phi), sizeof(quiet_phi));
    stream.write(reinterpret_cast<const char *>(&quiet_record), sizeof(quiet_record));
    stream.write(reinterpret_cast<const char *>(&quiet_no_arbitration), sizeof(quiet_no_arbitration));
    stream.write(reinterpret_cast<const char *>(&retired), sizeof(retired));
    stream.write(reinterpret_cast<const char *>(&fast_instructions), sizeof(fast_instructions));
    stream.write(reinterpret_cast<const char *>(&resumptions), sizeof(resumptions));
    checkpoint_continuation([&](auto &field) { stream.write(reinterpret_cast<const char *>(&field), sizeof(field)); });
    stream.write(reinterpret_cast<const char *>(&history_count), sizeof(history_count));
    stream.write(reinterpret_cast<const char *>(&cursor), sizeof(cursor));
    stream.write(reinterpret_cast<const char *>(&bus_number), sizeof(bus_number));
    stream.write(reinterpret_cast<const char *>(&nominal), sizeof(nominal));
    stream.write(reinterpret_cast<const char *>(&family), sizeof(family));
    stream.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
    stream.write(reinterpret_cast<const char *>(&prelude_schedule), sizeof(prelude_schedule));
    stream.write(reinterpret_cast<const char *>(&trapping), sizeof(trapping));
    stream.write(reinterpret_cast<const char *>(&third_trap), sizeof(third_trap));
    stream.write(reinterpret_cast<const char *>(&reti_notified), sizeof(reti_notified));
    stream.write(reinterpret_cast<const char *>(&trap_started), sizeof(trap_started));
    stream.write(reinterpret_cast<const char *>(&interrupt_started), sizeof(interrupt_started));
    stream.write(reinterpret_cast<const char *>(&sampled_interrupt), sizeof(sampled_interrupt));
    stream.write(reinterpret_cast<const char *>(&interrupt_recovery), sizeof(interrupt_recovery));
    stream.write(reinterpret_cast<const char *>(&dma_nmi), sizeof(dma_nmi));
    stream.write(reinterpret_cast<const char *>(&instruction_interrupt), sizeof(instruction_interrupt));
    stream.write(reinterpret_cast<const char *>(&ack_vector), sizeof(ack_vector));
    stream.write(reinterpret_cast<const char *>(&dma_channel), sizeof(dma_channel));
    stream.write(reinterpret_cast<const char *>(&dma_stage), sizeof(dma_stage));
    stream.write(reinterpret_cast<const char *>(&dma_continue), sizeof(dma_continue));
    stream.write(reinterpret_cast<const char *>(&dma_byte), sizeof(dma_byte));
    stream.write(reinterpret_cast<const char *>(&dma_source), sizeof(dma_source));
    stream.write(reinterpret_cast<const char *>(&dma_destination), sizeof(dma_destination));
    stream.write(reinterpret_cast<const char *>(&dma_source_io), sizeof(dma_source_io));
    stream.write(reinterpret_cast<const char *>(&dma_destination_io), sizeof(dma_destination_io));
    stream.write(reinterpret_cast<const char *>(&dma_source_mode), sizeof(dma_source_mode));
    stream.write(reinterpret_cast<const char *>(&dma_destination_mode), sizeof(dma_destination_mode));
    // Serial shift registers, queued bytes and deadlines must agree with
    // the CPU clock epoch when execution resumes.
    stream.write(reinterpret_cast<const char *>(&asci[0].asci_cntla), sizeof(asci[0].asci_cntla));
    stream.write(reinterpret_cast<const char *>(&asci[0].asci_cntlb), sizeof(asci[0].asci_cntlb));
    stream.write(reinterpret_cast<const char *>(&asci[0].asci_stat), sizeof(asci[0].asci_stat));
    stream.write(reinterpret_cast<const char *>(&asci[0].asci_tdr), sizeof(asci[0].asci_tdr));
    stream.write(reinterpret_cast<const char *>(&asci[0].asci_rdr), sizeof(asci[0].asci_rdr));
    stream.write(reinterpret_cast<const char *>(&asci[0].asci_ext), sizeof(asci[0].asci_ext));
    stream.write(reinterpret_cast<const char *>(&asci[0].asci_tc.w), sizeof(asci[0].asci_tc.w));
    stream.write(reinterpret_cast<const char *>(&asci[0].tdr_empty), sizeof(asci[0].tdr_empty));
    stream.write(reinterpret_cast<const char *>(&asci[0].tsr), sizeof(asci[0].tsr));
    stream.write(reinterpret_cast<const char *>(&asci[0].rsr), sizeof(asci[0].rsr));
    stream.write(reinterpret_cast<const char *>(&asci[0].data_fifo), sizeof(asci[0].data_fifo));
    stream.write(reinterpret_cast<const char *>(&asci[0].error_fifo), sizeof(asci[0].error_fifo));
    stream.write(reinterpret_cast<const char *>(&asci[0].fifo_wr), sizeof(asci[0].fifo_wr));
    stream.write(reinterpret_cast<const char *>(&asci[0].fifo_rd), sizeof(asci[0].fifo_rd));
    stream.write(reinterpret_cast<const char *>(&asci[0].fifo_count), sizeof(asci[0].fifo_count));
    stream.write(reinterpret_cast<const char *>(&asci[0].fifo_overrun), sizeof(asci[0].fifo_overrun));
    stream.write(reinterpret_cast<const char *>(&asci[0].cts), sizeof(asci[0].cts));
    stream.write(reinterpret_cast<const char *>(&asci[0].dcd), sizeof(asci[0].dcd));
    stream.write(reinterpret_cast<const char *>(&asci[0].irq), sizeof(asci[0].irq));
    stream.write(reinterpret_cast<const char *>(&asci[0].txa), sizeof(asci[0].txa));
    stream.write(reinterpret_cast<const char *>(&asci[0].rxa), sizeof(asci[0].rxa));
    stream.write(reinterpret_cast<const char *>(&asci[0].rts), sizeof(asci[0].rts));
    stream.write(reinterpret_cast<const char *>(&asci[0].divisor), sizeof(asci[0].divisor));
    stream.write(reinterpret_cast<const char *>(&asci[0].clock_state), sizeof(asci[0].clock_state));
    stream.write(reinterpret_cast<const char *>(&asci[0].tx_counter), sizeof(asci[0].tx_counter));
    stream.write(reinterpret_cast<const char *>(&asci[0].tx_state), sizeof(asci[0].tx_state));
    stream.write(reinterpret_cast<const char *>(&asci[0].rx_state), sizeof(asci[0].rx_state));
    stream.write(reinterpret_cast<const char *>(&asci[0].rx_bits), sizeof(asci[0].rx_bits));
    stream.write(reinterpret_cast<const char *>(&asci[0].rx_counter), sizeof(asci[0].rx_counter));
    stream.write(reinterpret_cast<const char *>(&asci[0].rx_count_to), sizeof(asci[0].rx_count_to));
    stream.write(reinterpret_cast<const char *>(&asci[0].rx_total_bits), sizeof(asci[0].rx_total_bits));
    stream.write(reinterpret_cast<const char *>(&asci[0].rx_enabled), sizeof(asci[0].rx_enabled));
    asci[0].bit_rate.save(stream);
    asci[0].sample_rate.save(stream);
    asci[0].receive_epoch.save(stream);
    stream.write(reinterpret_cast<const char *>(&asci[0].receive_parked), sizeof(asci[0].receive_parked));
    asci[0].receive_timer.deadline.save(stream);
    asci[0].receive_timer.period.save(stream);
    asci[0].transmit_timer.deadline.save(stream);
    asci[0].transmit_timer.period.save(stream);
    stream.write(reinterpret_cast<const char *>(&asci[0].clock_rate), sizeof(asci[0].clock_rate));
    stream.write(reinterpret_cast<const char *>(&asci[0].io_stopped), sizeof(asci[0].io_stopped));
    stream.write(reinterpret_cast<const char *>(&asci[1].asci_cntla), sizeof(asci[1].asci_cntla));
    stream.write(reinterpret_cast<const char *>(&asci[1].asci_cntlb), sizeof(asci[1].asci_cntlb));
    stream.write(reinterpret_cast<const char *>(&asci[1].asci_stat), sizeof(asci[1].asci_stat));
    stream.write(reinterpret_cast<const char *>(&asci[1].asci_tdr), sizeof(asci[1].asci_tdr));
    stream.write(reinterpret_cast<const char *>(&asci[1].asci_rdr), sizeof(asci[1].asci_rdr));
    stream.write(reinterpret_cast<const char *>(&asci[1].asci_ext), sizeof(asci[1].asci_ext));
    stream.write(reinterpret_cast<const char *>(&asci[1].asci_tc.w), sizeof(asci[1].asci_tc.w));
    stream.write(reinterpret_cast<const char *>(&asci[1].tdr_empty), sizeof(asci[1].tdr_empty));
    stream.write(reinterpret_cast<const char *>(&asci[1].tsr), sizeof(asci[1].tsr));
    stream.write(reinterpret_cast<const char *>(&asci[1].rsr), sizeof(asci[1].rsr));
    stream.write(reinterpret_cast<const char *>(&asci[1].data_fifo), sizeof(asci[1].data_fifo));
    stream.write(reinterpret_cast<const char *>(&asci[1].error_fifo), sizeof(asci[1].error_fifo));
    stream.write(reinterpret_cast<const char *>(&asci[1].fifo_wr), sizeof(asci[1].fifo_wr));
    stream.write(reinterpret_cast<const char *>(&asci[1].fifo_rd), sizeof(asci[1].fifo_rd));
    stream.write(reinterpret_cast<const char *>(&asci[1].fifo_count), sizeof(asci[1].fifo_count));
    stream.write(reinterpret_cast<const char *>(&asci[1].fifo_overrun), sizeof(asci[1].fifo_overrun));
    stream.write(reinterpret_cast<const char *>(&asci[1].cts), sizeof(asci[1].cts));
    stream.write(reinterpret_cast<const char *>(&asci[1].dcd), sizeof(asci[1].dcd));
    stream.write(reinterpret_cast<const char *>(&asci[1].irq), sizeof(asci[1].irq));
    stream.write(reinterpret_cast<const char *>(&asci[1].txa), sizeof(asci[1].txa));
    stream.write(reinterpret_cast<const char *>(&asci[1].rxa), sizeof(asci[1].rxa));
    stream.write(reinterpret_cast<const char *>(&asci[1].rts), sizeof(asci[1].rts));
    stream.write(reinterpret_cast<const char *>(&asci[1].divisor), sizeof(asci[1].divisor));
    stream.write(reinterpret_cast<const char *>(&asci[1].clock_state), sizeof(asci[1].clock_state));
    stream.write(reinterpret_cast<const char *>(&asci[1].tx_counter), sizeof(asci[1].tx_counter));
    stream.write(reinterpret_cast<const char *>(&asci[1].tx_state), sizeof(asci[1].tx_state));
    stream.write(reinterpret_cast<const char *>(&asci[1].rx_state), sizeof(asci[1].rx_state));
    stream.write(reinterpret_cast<const char *>(&asci[1].rx_bits), sizeof(asci[1].rx_bits));
    stream.write(reinterpret_cast<const char *>(&asci[1].rx_counter), sizeof(asci[1].rx_counter));
    stream.write(reinterpret_cast<const char *>(&asci[1].rx_count_to), sizeof(asci[1].rx_count_to));
    stream.write(reinterpret_cast<const char *>(&asci[1].rx_total_bits), sizeof(asci[1].rx_total_bits));
    stream.write(reinterpret_cast<const char *>(&asci[1].rx_enabled), sizeof(asci[1].rx_enabled));
    asci[1].bit_rate.save(stream);
    asci[1].sample_rate.save(stream);
    asci[1].receive_epoch.save(stream);
    stream.write(reinterpret_cast<const char *>(&asci[1].receive_parked), sizeof(asci[1].receive_parked));
    asci[1].receive_timer.deadline.save(stream);
    asci[1].receive_timer.period.save(stream);
    asci[1].transmit_timer.deadline.save(stream);
    asci[1].transmit_timer.period.save(stream);
    stream.write(reinterpret_cast<const char *>(&asci[1].clock_rate), sizeof(asci[1].clock_rate));
    stream.write(reinterpret_cast<const char *>(&asci[1].io_stopped), sizeof(asci[1].io_stopped));
    stream.write(reinterpret_cast<const char *>(&csio.cntr), sizeof(csio.cntr));
    stream.write(reinterpret_cast<const char *>(&csio.trdr), sizeof(csio.trdr));
    stream.write(reinterpret_cast<const char *>(&csio.shift_cnt), sizeof(csio.shift_cnt));
    stream.write(reinterpret_cast<const char *>(&csio.irq), sizeof(csio.irq));
    stream.write(reinterpret_cast<const char *>(&csio.cks_in), sizeof(csio.cks_in));
    stream.write(reinterpret_cast<const char *>(&csio.rxs_in), sizeof(csio.rxs_in));
    stream.write(reinterpret_cast<const char *>(&csio.cks_out), sizeof(csio.cks_out));
    stream.write(reinterpret_cast<const char *>(&csio.txs_out), sizeof(csio.txs_out));
    csio.timer.deadline.save(stream);
    csio.timer.period.save(stream);
    stream.write(reinterpret_cast<const char *>(&csio.clock_rate), sizeof(csio.clock_rate));
    stream.write(reinterpret_cast<const char *>(&csio.io_stopped), sizeof(csio.io_stopped));
    return stream ? 0 : 1;
}
// Restore the checkpoint and rebuild derived state; external callbacks remain
// attached to the current board instance.
// Return 1 for a wrong format/revision or a failed read, 0 on success.
// A read failure may have changed part of the state; callers must treat it
// as a failed restore. Derived pointers and time caches are rebuilt at the end.
int Z180::load(std::istream &stream) {
    bus_release_at = UINT64_MAX;
    uint32_t format = 0, revision = 0;
    stream.read(reinterpret_cast<char *>(&format), sizeof(format));
    stream.read(reinterpret_cast<char *>(&revision), sizeof(revision));
    if (!stream || format != 0x5a313802 || revision != uint32_t(silicon))
        return 1;
    stream.read(reinterpret_cast<char *>(&power_mode), sizeof(power_mode));
    stream.read(reinterpret_cast<char *>(&wake_remaining), sizeof(wake_remaining));
    stream.read(reinterpret_cast<char *>(&cmr), sizeof(cmr));
    stream.read(reinterpret_cast<char *>(&ccr), sizeof(ccr));
    epoch_time.load(stream);
    stream.read(reinterpret_cast<char *>(&epoch_half), sizeof(epoch_half));
    stream.read(reinterpret_cast<char *>(&epoch_rate), sizeof(epoch_rate));
    stream.read(reinterpret_cast<char *>(&attoseconds_per_cycle), sizeof(attoseconds_per_cycle));
    // Architectural registers and interrupt inputs precede on-chip peripherals.
    stream.read(reinterpret_cast<char *>(&AF.w.l), sizeof(AF.w.l));
    stream.read(reinterpret_cast<char *>(&BC.w.l), sizeof(BC.w.l));
    stream.read(reinterpret_cast<char *>(&DE.w.l), sizeof(DE.w.l));
    stream.read(reinterpret_cast<char *>(&HL.w.l), sizeof(HL.w.l));
    stream.read(reinterpret_cast<char *>(&IX.w.l), sizeof(IX.w.l));
    stream.read(reinterpret_cast<char *>(&IY.w.l), sizeof(IY.w.l));
    stream.read(reinterpret_cast<char *>(&PC.w.l), sizeof(PC.w.l));
    stream.read(reinterpret_cast<char *>(&SP.w.l), sizeof(SP.w.l));
    stream.read(reinterpret_cast<char *>(&AF2.w.l), sizeof(AF2.w.l));
    stream.read(reinterpret_cast<char *>(&BC2.w.l), sizeof(BC2.w.l));
    stream.read(reinterpret_cast<char *>(&DE2.w.l), sizeof(DE2.w.l));
    stream.read(reinterpret_cast<char *>(&HL2.w.l), sizeof(HL2.w.l));
    stream.read(reinterpret_cast<char *>(&R), sizeof(R));
    stream.read(reinterpret_cast<char *>(&R2), sizeof(R2));
    stream.read(reinterpret_cast<char *>(&IFF1), sizeof(IFF1));
    stream.read(reinterpret_cast<char *>(&IFF2), sizeof(IFF2));
    stream.read(reinterpret_cast<char *>(&HALT), sizeof(HALT));
    stream.read(reinterpret_cast<char *>(&IM), sizeof(IM));
    stream.read(reinterpret_cast<char *>(&I), sizeof(I));
    stream.read(reinterpret_cast<char *>(&nmi_state), sizeof(nmi_state));
    stream.read(reinterpret_cast<char *>(&nmi_pending), sizeof(nmi_pending));
    stream.read(reinterpret_cast<char *>(&irq_state), sizeof(irq_state));
    stream.read(reinterpret_cast<char *>(&int_pending), sizeof(int_pending));
    stream.read(reinterpret_cast<char *>(&frc_prescale), sizeof(frc_prescale));
    stream.read(reinterpret_cast<char *>(&bus_released), sizeof(bus_released));
    stream.read(reinterpret_cast<char *>(&after_EI), sizeof(after_EI));
    stream.read(reinterpret_cast<char *>(&read_tcr_tmdr), sizeof(read_tcr_tmdr));
    stream.read(reinterpret_cast<char *>(&tmdr_value), sizeof(tmdr_value));
    stream.read(reinterpret_cast<char *>(&tmdrh), sizeof(tmdrh));
    stream.read(reinterpret_cast<char *>(&tmdr_latch), sizeof(tmdr_latch));
    stream.read(reinterpret_cast<char *>(&prt1_timeout_odd), sizeof(prt1_timeout_odd));
    stream.read(reinterpret_cast<char *>(&tout_level), sizeof(tout_level));
    stream.read(reinterpret_cast<char *>(&tmdr[0].w), sizeof(tmdr[0].w));
    stream.read(reinterpret_cast<char *>(&tmdr[1].w), sizeof(tmdr[1].w));
    stream.read(reinterpret_cast<char *>(&rldr[0].w), sizeof(rldr[0].w));
    stream.read(reinterpret_cast<char *>(&rldr[1].w), sizeof(rldr[1].w));
    stream.read(reinterpret_cast<char *>(&tcr), sizeof(tcr));
    stream.read(reinterpret_cast<char *>(&frc), sizeof(frc));
    stream.read(reinterpret_cast<char *>(&dma_sar0.d), sizeof(dma_sar0.d));
    stream.read(reinterpret_cast<char *>(&dma_dar0.d), sizeof(dma_dar0.d));
    stream.read(reinterpret_cast<char *>(&dma_bcr[0].w), sizeof(dma_bcr[0].w));
    stream.read(reinterpret_cast<char *>(&dma_bcr[1].w), sizeof(dma_bcr[1].w));
    stream.read(reinterpret_cast<char *>(&dma_mar1.d), sizeof(dma_mar1.d));
    stream.read(reinterpret_cast<char *>(&dma_iar1.d), sizeof(dma_iar1.d));
    stream.read(reinterpret_cast<char *>(&dstat), sizeof(dstat));
    stream.read(reinterpret_cast<char *>(&dmode), sizeof(dmode));
    stream.read(reinterpret_cast<char *>(&dcntl), sizeof(dcntl));
    stream.read(reinterpret_cast<char *>(&il), sizeof(il));
    stream.read(reinterpret_cast<char *>(&itc), sizeof(itc));
    stream.read(reinterpret_cast<char *>(&rcr), sizeof(rcr));
    stream.read(reinterpret_cast<char *>(&mmu_cbr), sizeof(mmu_cbr));
    stream.read(reinterpret_cast<char *>(&mmu_bbr), sizeof(mmu_bbr));
    stream.read(reinterpret_cast<char *>(&mmu_cbar), sizeof(mmu_cbar));
    stream.read(reinterpret_cast<char *>(&omcr), sizeof(omcr));
    stream.read(reinterpret_cast<char *>(&iocr), sizeof(iocr));
    stream.read(reinterpret_cast<char *>(&iol), sizeof(iol));
    stream.read(reinterpret_cast<char *>(&ioltemp), sizeof(ioltemp));
    stream.read(reinterpret_cast<char *>(&mmu), sizeof(mmu));
#ifdef GS2_SS_TESTING
    // Preserve the partially completed bus cycle and its replay journal.
    // These fields allow a checkpoint between PHI edges, not just instructions.
    stream.read(reinterpret_cast<char *>(&cycle_reference), sizeof(cycle_reference));
#else
    bool saved_reference = false;
    stream.read(reinterpret_cast<char *>(&saved_reference), sizeof(saved_reference));
    if (saved_reference)
        return -1;
#endif
    stream.read(reinterpret_cast<char *>(&cycle_executing), sizeof(cycle_executing));
    stream.read(reinterpret_cast<char *>(&instruction_fast), sizeof(instruction_fast));
    stream.read(reinterpret_cast<char *>(&instruction_whole), sizeof(instruction_whole));
    stream.read(reinterpret_cast<char *>(&fast_irq_valid), sizeof(fast_irq_valid));
    stream.read(reinterpret_cast<char *>(&fast_interrupt), sizeof(fast_interrupt));
    stream.read(reinterpret_cast<char *>(&instruction_active), sizeof(instruction_active));
    stream.read(reinterpret_cast<char *>(&wait_line), sizeof(wait_line));
    stream.read(reinterpret_cast<char *>(&bus_ack), sizeof(bus_ack));
    stream.read(reinterpret_cast<char *>(&sampled_bus_request), sizeof(sampled_bus_request));
    stream.read(reinterpret_cast<char *>(&boundary_allowed), sizeof(boundary_allowed));
    stream.read(reinterpret_cast<char *>(&dma_allowed), sizeof(dma_allowed));
    stream.read(reinterpret_cast<char *>(&dma_start_pending), sizeof(dma_start_pending));
    stream.read(reinterpret_cast<char *>(&refresh_pending), sizeof(refresh_pending));
    stream.read(reinterpret_cast<char *>(&refresh_after_sleep), sizeof(refresh_after_sleep));
    stream.read(reinterpret_cast<char *>(&dma_edge), sizeof(dma_edge));
    stream.read(reinterpret_cast<char *>(&sampled_iol), sizeof(sampled_iol));
    stream.read(reinterpret_cast<char *>(&dma_edge_generation), sizeof(dma_edge_generation));
    stream.read(reinterpret_cast<char *>(&dma_edge_accepted), sizeof(dma_edge_accepted));
    stream.read(reinterpret_cast<char *>(&dma_edge_sampled), sizeof(dma_edge_sampled));
    stream.read(reinterpret_cast<char *>(&phi_half), sizeof(phi_half));
    stream.read(reinterpret_cast<char *>(&refresh_address), sizeof(refresh_address));
    stream.read(reinterpret_cast<char *>(&refresh_phase), sizeof(refresh_phase));
    stream.read(reinterpret_cast<char *>(&advance_left), sizeof(advance_left));
    stream.read(reinterpret_cast<char *>(&half_phi), sizeof(half_phi));
    stream.read(reinterpret_cast<char *>(&quiet_phi), sizeof(quiet_phi));
    stream.read(reinterpret_cast<char *>(&quiet_record), sizeof(quiet_record));
    stream.read(reinterpret_cast<char *>(&quiet_no_arbitration), sizeof(quiet_no_arbitration));
    stream.read(reinterpret_cast<char *>(&retired), sizeof(retired));
    stream.read(reinterpret_cast<char *>(&fast_instructions), sizeof(fast_instructions));
    stream.read(reinterpret_cast<char *>(&resumptions), sizeof(resumptions));
    checkpoint_continuation([&](auto &field) { stream.read(reinterpret_cast<char *>(&field), sizeof(field)); });
    stream.read(reinterpret_cast<char *>(&history_count), sizeof(history_count));
    stream.read(reinterpret_cast<char *>(&cursor), sizeof(cursor));
    stream.read(reinterpret_cast<char *>(&bus_number), sizeof(bus_number));
    stream.read(reinterpret_cast<char *>(&nominal), sizeof(nominal));
    stream.read(reinterpret_cast<char *>(&family), sizeof(family));
    stream.read(reinterpret_cast<char *>(&opcode), sizeof(opcode));
    stream.read(reinterpret_cast<char *>(&prelude_schedule), sizeof(prelude_schedule));
    stream.read(reinterpret_cast<char *>(&trapping), sizeof(trapping));
    stream.read(reinterpret_cast<char *>(&third_trap), sizeof(third_trap));
    stream.read(reinterpret_cast<char *>(&reti_notified), sizeof(reti_notified));
    stream.read(reinterpret_cast<char *>(&trap_started), sizeof(trap_started));
    stream.read(reinterpret_cast<char *>(&interrupt_started), sizeof(interrupt_started));
    stream.read(reinterpret_cast<char *>(&sampled_interrupt), sizeof(sampled_interrupt));
    stream.read(reinterpret_cast<char *>(&interrupt_recovery), sizeof(interrupt_recovery));
    stream.read(reinterpret_cast<char *>(&dma_nmi), sizeof(dma_nmi));
    stream.read(reinterpret_cast<char *>(&instruction_interrupt), sizeof(instruction_interrupt));
    stream.read(reinterpret_cast<char *>(&ack_vector), sizeof(ack_vector));
    stream.read(reinterpret_cast<char *>(&dma_channel), sizeof(dma_channel));
    stream.read(reinterpret_cast<char *>(&dma_stage), sizeof(dma_stage));
    stream.read(reinterpret_cast<char *>(&dma_continue), sizeof(dma_continue));
    stream.read(reinterpret_cast<char *>(&dma_byte), sizeof(dma_byte));
    stream.read(reinterpret_cast<char *>(&dma_source), sizeof(dma_source));
    stream.read(reinterpret_cast<char *>(&dma_destination), sizeof(dma_destination));
    stream.read(reinterpret_cast<char *>(&dma_source_io), sizeof(dma_source_io));
    stream.read(reinterpret_cast<char *>(&dma_destination_io), sizeof(dma_destination_io));
    stream.read(reinterpret_cast<char *>(&dma_source_mode), sizeof(dma_source_mode));
    stream.read(reinterpret_cast<char *>(&dma_destination_mode), sizeof(dma_destination_mode));
    // Serial shift registers, queued bytes and deadlines must agree with
    // the CPU clock epoch when execution resumes.
    stream.read(reinterpret_cast<char *>(&asci[0].asci_cntla), sizeof(asci[0].asci_cntla));
    stream.read(reinterpret_cast<char *>(&asci[0].asci_cntlb), sizeof(asci[0].asci_cntlb));
    stream.read(reinterpret_cast<char *>(&asci[0].asci_stat), sizeof(asci[0].asci_stat));
    stream.read(reinterpret_cast<char *>(&asci[0].asci_tdr), sizeof(asci[0].asci_tdr));
    stream.read(reinterpret_cast<char *>(&asci[0].asci_rdr), sizeof(asci[0].asci_rdr));
    stream.read(reinterpret_cast<char *>(&asci[0].asci_ext), sizeof(asci[0].asci_ext));
    stream.read(reinterpret_cast<char *>(&asci[0].asci_tc.w), sizeof(asci[0].asci_tc.w));
    stream.read(reinterpret_cast<char *>(&asci[0].tdr_empty), sizeof(asci[0].tdr_empty));
    stream.read(reinterpret_cast<char *>(&asci[0].tsr), sizeof(asci[0].tsr));
    stream.read(reinterpret_cast<char *>(&asci[0].rsr), sizeof(asci[0].rsr));
    stream.read(reinterpret_cast<char *>(&asci[0].data_fifo), sizeof(asci[0].data_fifo));
    stream.read(reinterpret_cast<char *>(&asci[0].error_fifo), sizeof(asci[0].error_fifo));
    stream.read(reinterpret_cast<char *>(&asci[0].fifo_wr), sizeof(asci[0].fifo_wr));
    stream.read(reinterpret_cast<char *>(&asci[0].fifo_rd), sizeof(asci[0].fifo_rd));
    stream.read(reinterpret_cast<char *>(&asci[0].fifo_count), sizeof(asci[0].fifo_count));
    stream.read(reinterpret_cast<char *>(&asci[0].fifo_overrun), sizeof(asci[0].fifo_overrun));
    stream.read(reinterpret_cast<char *>(&asci[0].cts), sizeof(asci[0].cts));
    stream.read(reinterpret_cast<char *>(&asci[0].dcd), sizeof(asci[0].dcd));
    stream.read(reinterpret_cast<char *>(&asci[0].irq), sizeof(asci[0].irq));
    stream.read(reinterpret_cast<char *>(&asci[0].txa), sizeof(asci[0].txa));
    stream.read(reinterpret_cast<char *>(&asci[0].rxa), sizeof(asci[0].rxa));
    stream.read(reinterpret_cast<char *>(&asci[0].rts), sizeof(asci[0].rts));
    stream.read(reinterpret_cast<char *>(&asci[0].divisor), sizeof(asci[0].divisor));
    stream.read(reinterpret_cast<char *>(&asci[0].clock_state), sizeof(asci[0].clock_state));
    stream.read(reinterpret_cast<char *>(&asci[0].tx_counter), sizeof(asci[0].tx_counter));
    stream.read(reinterpret_cast<char *>(&asci[0].tx_state), sizeof(asci[0].tx_state));
    stream.read(reinterpret_cast<char *>(&asci[0].rx_state), sizeof(asci[0].rx_state));
    stream.read(reinterpret_cast<char *>(&asci[0].rx_bits), sizeof(asci[0].rx_bits));
    stream.read(reinterpret_cast<char *>(&asci[0].rx_counter), sizeof(asci[0].rx_counter));
    stream.read(reinterpret_cast<char *>(&asci[0].rx_count_to), sizeof(asci[0].rx_count_to));
    stream.read(reinterpret_cast<char *>(&asci[0].rx_total_bits), sizeof(asci[0].rx_total_bits));
    stream.read(reinterpret_cast<char *>(&asci[0].rx_enabled), sizeof(asci[0].rx_enabled));
    asci[0].bit_rate.load(stream);
    asci[0].sample_rate.load(stream);
    asci[0].receive_epoch.load(stream);
    stream.read(reinterpret_cast<char *>(&asci[0].receive_parked), sizeof(asci[0].receive_parked));
    asci[0].receive_timer.deadline.load(stream);
    asci[0].receive_timer.period.load(stream);
    asci[0].transmit_timer.deadline.load(stream);
    asci[0].transmit_timer.period.load(stream);
    stream.read(reinterpret_cast<char *>(&asci[0].clock_rate), sizeof(asci[0].clock_rate));
    stream.read(reinterpret_cast<char *>(&asci[0].io_stopped), sizeof(asci[0].io_stopped));
    stream.read(reinterpret_cast<char *>(&asci[1].asci_cntla), sizeof(asci[1].asci_cntla));
    stream.read(reinterpret_cast<char *>(&asci[1].asci_cntlb), sizeof(asci[1].asci_cntlb));
    stream.read(reinterpret_cast<char *>(&asci[1].asci_stat), sizeof(asci[1].asci_stat));
    stream.read(reinterpret_cast<char *>(&asci[1].asci_tdr), sizeof(asci[1].asci_tdr));
    stream.read(reinterpret_cast<char *>(&asci[1].asci_rdr), sizeof(asci[1].asci_rdr));
    stream.read(reinterpret_cast<char *>(&asci[1].asci_ext), sizeof(asci[1].asci_ext));
    stream.read(reinterpret_cast<char *>(&asci[1].asci_tc.w), sizeof(asci[1].asci_tc.w));
    stream.read(reinterpret_cast<char *>(&asci[1].tdr_empty), sizeof(asci[1].tdr_empty));
    stream.read(reinterpret_cast<char *>(&asci[1].tsr), sizeof(asci[1].tsr));
    stream.read(reinterpret_cast<char *>(&asci[1].rsr), sizeof(asci[1].rsr));
    stream.read(reinterpret_cast<char *>(&asci[1].data_fifo), sizeof(asci[1].data_fifo));
    stream.read(reinterpret_cast<char *>(&asci[1].error_fifo), sizeof(asci[1].error_fifo));
    stream.read(reinterpret_cast<char *>(&asci[1].fifo_wr), sizeof(asci[1].fifo_wr));
    stream.read(reinterpret_cast<char *>(&asci[1].fifo_rd), sizeof(asci[1].fifo_rd));
    stream.read(reinterpret_cast<char *>(&asci[1].fifo_count), sizeof(asci[1].fifo_count));
    stream.read(reinterpret_cast<char *>(&asci[1].fifo_overrun), sizeof(asci[1].fifo_overrun));
    stream.read(reinterpret_cast<char *>(&asci[1].cts), sizeof(asci[1].cts));
    stream.read(reinterpret_cast<char *>(&asci[1].dcd), sizeof(asci[1].dcd));
    stream.read(reinterpret_cast<char *>(&asci[1].irq), sizeof(asci[1].irq));
    stream.read(reinterpret_cast<char *>(&asci[1].txa), sizeof(asci[1].txa));
    stream.read(reinterpret_cast<char *>(&asci[1].rxa), sizeof(asci[1].rxa));
    stream.read(reinterpret_cast<char *>(&asci[1].rts), sizeof(asci[1].rts));
    stream.read(reinterpret_cast<char *>(&asci[1].divisor), sizeof(asci[1].divisor));
    stream.read(reinterpret_cast<char *>(&asci[1].clock_state), sizeof(asci[1].clock_state));
    stream.read(reinterpret_cast<char *>(&asci[1].tx_counter), sizeof(asci[1].tx_counter));
    stream.read(reinterpret_cast<char *>(&asci[1].tx_state), sizeof(asci[1].tx_state));
    stream.read(reinterpret_cast<char *>(&asci[1].rx_state), sizeof(asci[1].rx_state));
    stream.read(reinterpret_cast<char *>(&asci[1].rx_bits), sizeof(asci[1].rx_bits));
    stream.read(reinterpret_cast<char *>(&asci[1].rx_counter), sizeof(asci[1].rx_counter));
    stream.read(reinterpret_cast<char *>(&asci[1].rx_count_to), sizeof(asci[1].rx_count_to));
    stream.read(reinterpret_cast<char *>(&asci[1].rx_total_bits), sizeof(asci[1].rx_total_bits));
    stream.read(reinterpret_cast<char *>(&asci[1].rx_enabled), sizeof(asci[1].rx_enabled));
    asci[1].bit_rate.load(stream);
    asci[1].sample_rate.load(stream);
    asci[1].receive_epoch.load(stream);
    stream.read(reinterpret_cast<char *>(&asci[1].receive_parked), sizeof(asci[1].receive_parked));
    asci[1].receive_timer.deadline.load(stream);
    asci[1].receive_timer.period.load(stream);
    asci[1].transmit_timer.deadline.load(stream);
    asci[1].transmit_timer.period.load(stream);
    stream.read(reinterpret_cast<char *>(&asci[1].clock_rate), sizeof(asci[1].clock_rate));
    stream.read(reinterpret_cast<char *>(&asci[1].io_stopped), sizeof(asci[1].io_stopped));
    stream.read(reinterpret_cast<char *>(&csio.cntr), sizeof(csio.cntr));
    stream.read(reinterpret_cast<char *>(&csio.trdr), sizeof(csio.trdr));
    stream.read(reinterpret_cast<char *>(&csio.shift_cnt), sizeof(csio.shift_cnt));
    stream.read(reinterpret_cast<char *>(&csio.irq), sizeof(csio.irq));
    stream.read(reinterpret_cast<char *>(&csio.cks_in), sizeof(csio.cks_in));
    stream.read(reinterpret_cast<char *>(&csio.rxs_in), sizeof(csio.rxs_in));
    stream.read(reinterpret_cast<char *>(&csio.cks_out), sizeof(csio.cks_out));
    stream.read(reinterpret_cast<char *>(&csio.txs_out), sizeof(csio.txs_out));
    csio.timer.deadline.load(stream);
    csio.timer.period.load(stream);
    stream.read(reinterpret_cast<char *>(&csio.clock_rate), sizeof(csio.clock_rate));
    stream.read(reinterpret_cast<char *>(&csio.io_stopped), sizeof(csio.io_stopped));
    instruction_fast = instruction_whole = cycle_executing = false;
    quiet_phi = 0;
    cycle_update_direct_reads();
    reset_time_cache();
    return stream ? 0 : 1;
}

// Compose established idle cycles only. The scheduler grant is already bounded
// by every external and serial event. Timers/refresh that can alter a sampled
// input stay on the phase engine. No bus callback or observer is bypassed.
// Advance HALT clocks only to the next interrupt/peripheral/bus event.
bool Z180::skip_machine_idle() {
    if (!idle_skipping || cycle_reference || cycle_observer || trace || cycle.active || advance_left || quiet_phi ||
        wait_line || bus_released || bus_ack || nmi_pending || after_EI || (rcr & 0x80) || (tcr & 3) ||
        dma_channel != 0xff || dma_continue != 0xff || cycle_dma_ready(0) || cycle_dma_ready(1))
        return false;
    const bool sleep = HALT == 2;
    if (cycle.kind != (sleep ? cycle_kind::internal : cycle_kind::halt))
        return false;
    auto direct = sleep ? nullptr : cycle_direct_read(MMU_REMAP_ADDR(uint16_t(pc16 + 1)));
    if (!sleep && !direct)
        return false;
    unsigned const period = sleep ? 2 : 2 * (3 + memory_wait_states());
    unsigned const count = unsigned(std::max(0, icount - 1)) / period;
    if (!count)
        return false;
    unsigned const ticks = count * period;
    icount -= ticks;
    half_phi += ticks;
    handle_io_timers(ticks / 2);
    refresh_phase = (refresh_phase + ticks) % 160;
    if (!sleep)
        cycle.data = drive_direct_read(*direct);
    cycle_sample_stable_inputs();
    boundary_allowed = true;
    dma_allowed = !sleep;
    return true;
}

// The firmware's DMA1 wait loop has a fixed state after its first iteration.
// Certification is live on every admission: self-modification, MMU/pin changes,
// DMA routing, interrupts, waits or observers return to ordinary execution.
// Skip a certified polling loop only while its memory and external inputs
// remain stable; the next board event must still observe its final bus state.
bool Z180::skip_machine_poll() {
    if (!idle_skipping || cycle_reference || cycle_observer || trace || !idle_memory || !direct_reads_enabled ||
        cycle.active || advance_left || HALT || IFF1 || after_EI || nmi_pending || bus_released || bus_ack ||
        wait_line || (tcr & 3) || (rcr & 0x80) || (dcntl & 0xf0) || (dstat & 0xc1) != 0x81 || cycle_dma_ready(1) ||
        dma_channel != 0xff || dma_continue != 0xff || !is_internal_io_address(0x30) ||
        previous_pc != uint16_t(pc16 + 5))
        return false;
    if (a8 != uint8_t(dstat | ~Z180_DSTAT_MASK | 0x30) || f8 != uint8_t((f8 & CF) | HF | SZ_BIT[0x80]))
        return false;
    unsigned const phi = cc[Z180_TABLE_ed][0x38] + cc[Z180_TABLE_cb][0x7f] + cc[Z180_TABLE_op][0xc2] + 3;
    unsigned const count = unsigned(std::max(0, icount - 1)) / (2 * phi);
    if (!count || idle_memory(0x100030, false) < 0)
        return false;
    uint8_t const bytes[] = {0xed, 0x38, 0x30, 0xcb, 0x7f, 0xc2, uint8_t(pc16), uint8_t(pc16 >> 8)};
    for (unsigned i = 0; i < std::size(bytes); ++i) {
        uint16_t const a = pc16 + i;
        auto const page = direct_logical[a >> 12];
        if (!page || page[a & 4095] != bytes[i])
            return false;
    }
    cycle_flush_quiet();
    unsigned const ticks = 2 * phi * count;
    half_phi += ticks;
    icount -= ticks;
    retired += 3 * count;
    R += 5 * count;
    instruction_start.control[0] += 5 * count;
    handle_io_timers(phi * count);
    refresh_phase = (refresh_phase + ticks) % 160;
    drive_direct_read(bytes[7]);
    cycle_sample_stable_inputs();
    return true;
}

// K: 1992 Zilog data book pp374-377, STANDBY is 2^19 cycles; CCR3
// is reserved. N: UM0050 pp37-41,49 adds IDLE and quick recovery.
// While PHI is stopped, half_phi is elapsed scheduler time in half-PHI units;
// it must not clock FRC, refresh, PRT, serial or DMA.
// Apply silicon-specific SLEEP/standby clock stopping and wake-up timing;
// K silicon must not inherit N-only idle/quick-recovery behaviour.
bool Z180::run_low_power() {
    if (HALT != 2 || !(iocr & 0x20) || !(ccr & 0x48))
        return false;
    if (!power_mode) {
        cycle_flush_quiet();
        power_mode = silicon == Silicon::K ? 2 : ((ccr >> 5) & 2) | ((ccr >> 3) & 1);
        wake_remaining = 0;
    }
    int source = -1;
    if (nmi_pending)
        source = 1;
    else if (irq_state[0] && (itc & 1))
        source = 2;
    else if (irq_state[1] && (itc & 2))
        source = 3;
    else if (irq_state[2] && (itc & 4))
        source = 4;
    bool const bus_wake = (ccr & 0x20) && bus_released;
    // K BUSREQ-only wake temporarily runs the released bus. On release it
    // re-enters STANDBY without executing the instruction following SLP (p375).
    if (power_mode == 4) {
        if (source >= 0) {
            power_mode = 0;
            sampled_interrupt = source;
            LEAVE_HALT();
            return false;
        }
        if (bus_released)
            return false;
        // Finish the already-started release cycle and drop BUSACK first.
        if (bus_ack) {
            cycle_arbitrate();
            if (bus_ack)
                return true;
        }
        power_mode = 2;
    }
    if (source < 0 && !bus_wake) {
        wake_remaining = 0; // Level-triggered recovery withdrawn: restart standby.
        half_phi += icount;
        icount = 0;
        return true;
    }
    if (!wake_remaining) {
        unsigned const clocks = power_mode == 1 ? 8 : power_mode == 2 ? 131072 : 64;
        wake_remaining = silicon == Silicon::K ? 2 * 524288 : 2 * clocks + 3;
    }
    unsigned const ticks = std::min<unsigned>(icount, wake_remaining);
    half_phi += ticks;
    icount -= ticks;
    wake_remaining -= ticks;
    if (wake_remaining)
        return true;
    power_mode = silicon == Silicon::K && source < 0 && bus_wake ? 4 : 0;
    sampled_interrupt = source;
    sampled_bus_request = bus_released;
    refresh_pending = false;
    refresh_after_sleep = true;
    boundary_allowed = true;
    dma_allowed = false;
    if (power_mode != 4)
        LEAVE_HALT();
    return false;
}

#ifdef GS2_SS_TESTING
#include "legacy.hxx"
#endif
