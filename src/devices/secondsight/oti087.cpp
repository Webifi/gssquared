/*
 *   SPDX-License-Identifier: BSD-3-Clause
 *   Copyright holders: Nathan Woods, Peter Trauner, Angelo Salese.
 *   See secondsight-video.LICENSE for the BSD-3-Clause terms.
 *
 *   The Oak OTI-087X is Second Sight's VGA (Video Graphics Array) controller.
 *   It owns one MiB of video RAM (VRAM), decodes Z180 memory/I/O accesses and
 *   produces a raster: pixels scanned left to right, then top to bottom.
 *   The CRTC (cathode-ray-tube controller) counts pixels/lines and generates
 *   blanking intervals, when the monitor must not display video. The RAMDAC
 *   is the Brooktree Bt481 palette memory and digital-to-analog converter;
 *   the model returns RGB (red, green, blue) colors rather than voltages.
 *
 *   SSCard owns OTI087. Its execution thread changes registers and advances
 *   time. The main thread renders or inspects it only after synchronization.
 *   A latch retains a value for later use; register writes can prepare a new
 *   screen address before the display latches it. An event is a scheduled
 *   change such as the start of vertical blanking. Time is in emulated seconds;
 *   an attosecond is 10^-18 seconds, used to round clock periods consistently.
 *
 *   Sources: Oak OTI-087 February 1993 databook pp. 29-46 and OTI-087X addendum
 *   (https://bitsavers.org/components/oakTechnology/OTI-087_Feb1993.pdf);
 *   IBM VGA/XGA Technical Reference May 1992, VGA registers; Brooktree Bt481A
 *   L481A_C pp. 6-8, 14-20 for palette and color formats.
 */

#include "oti087.hpp"
#include <algorithm>
#include <cmath>

namespace {
// Extract bit n as zero or one when decoding a documented register field.
constexpr unsigned bit(uint32_t value, unsigned n) {
    return (value >> n) & 1;
}
} // namespace

// The board retains its data-bus byte until an enabled device drives a read.
// Decode active mono/color VGA ports and Oak extension ports before the
// board decides whether the data bus is driven.
// port is a Z180 I/O address. VGA ports: $3C0/$3C1 attribute index/data,
// $3C2 input status/miscellaneous output, $3C4/$3C5 sequencer index/data,
// $3C6-$3C9 color-converter ports, $3CA feature status, $3CC miscellaneous
// readback, $3CE/$3CF graphics index/data. $3B4/$3B5 or $3D4/$3D5 select
// CRTC index/data; $3BA/$3DA report beam status. Oak adds $3DE/$3DF index/data
// and $0102 enable under $46E8 setup control (Oak databook pp. 29-31).
bool OTI087::io_read_selected(uint16_t port) const {
    if (port == 0x102)
        return setup & 0x10;
    if (!enabled())
        return false;
    if (port == 0x3de || port == 0x3df || (port >= 0x3c6 && port <= 0x3c9))
        return true;
    if (ext[0x19] != 15 && (port & 0xfff0) == (0x300 | (ext[0x19] << 4)))
        return true;
    switch (port) {
        case 0x3c0:
        case 0x3c1:
        case 0x3c2:
        case 0x3c4:
        case 0x3c5:
        case 0x3ca:
        case 0x3cc:
        case 0x3ce:
        case 0x3cf:
            return true;
    }
    return (port & 0xfff0) == get_crtc_port() && ((port & 15) == 4 || (port & 15) == 5 || (port & 15) == 10);
}

// Read indexed VGA/Oak registers and forward DAC ports to the Bt481;
// status reads also control the attribute-controller index/data flip-flop.
uint8_t OTI087::io_read_impl(uint32_t port) {
    port &= 0xffff;
    if (port == 0x102)
        return bit(setup, 4) ? enable : 0xff;
    if (!enabled())
        return 0xff;
    if (port == 0x3de)
        return index;
    if (port == 0x3df)
        return extended_r();
    if (port >= 0x3c6 && port <= 0x3c9)
        return dac_r(port & 3);
    // OTI p41: this range is in addition to the standard palette addresses.
    if (ext[0x19] != 15 && (port & 0xfff0) == (0x300 | (ext[0x19] << 4)))
        return dac_r(port & 3); // Second Sight grounds the RAMDAC RS2 input.
    switch (port) {
        case 0x3c0:
            return attribute.index;
        case 0x3c1:
            return attribute_read();
        case 0x3c2:
            return 0x60 | (((monitor_override < 0 ? 1 : monitor_override) & 1) << 4) | (crtc.irq_latch << 7);
        case 0x3c4:
            return sequencer.index;
        case 0x3c5:
            return sequencer.data[sequencer.index];
        case 0x3ca:
            return feature_control;
        case 0x3cc:
            return miscellaneous_output;
        case 0x3ce:
            return gc.index;
        case 0x3cf:
            return gc_read();
    }
    if ((port & 0xfff0) == get_crtc_port()) {
        switch (port & 15) {
            case 4:
                return crtc.index;
            case 5:
                return crtc_read();
            case 10: {
                // Read side effects must not occur for debugger inspections.
                if (side_effects)
                    attribute.state = 0;
                int const vt = crtc.vert_total + 2;
                int const field_y = vpos() % vt;
                unsigned const dots = (!gc.alpha_dis && !bit(sequencer.data[1], 0)) ? 9 : 8;
                unsigned const char_width = dots * horizontal_multiplier();
                int const hchar = hpos() * pixel_divisor() / char_width;
                // Blank-end registers store only low counter bits. Subtract
                // modulo that width, treating zero as a full wrap, then test
                // distance from blank start. This handles blank intervals
                // that cross the end of a line or frame.
                int const hb_len = (crtc.horz_blank_end - crtc.horz_blank_start) & 63;
                int const vb_len = (crtc.vert_blank_end - crtc.vert_blank_start) & 127;
                bool const hb = (hchar - crtc.horz_blank_start + crtc.horz_total + 5) % (crtc.horz_total + 5) <
                                (hb_len ? hb_len : 64);
                bool const vb = (field_y - crtc.vert_blank_start + vt * 2) % vt < (vb_len ? vb_len : 128);
                return (hb || vb ? 1 : 0) | (vertical_retrace() ? 8 : 0);
            }
        }
    }
    return 0xff;
}

// Route VGA index/data pairs and RAMDAC writes through the chip decoders.
void OTI087::io_w(uint32_t port, uint8_t data) {
    port &= 0xffff;
    // OTI pp29-30: only setup state exposes 0102; 46e8 is write-only.
    if (port == 0x46e8) {
        setup = data & 0x18;
        return;
    }
    if (port == 0x102) {
        if (bit(setup, 4))
            enable = data & 1;
        return;
    }
    if (!enabled())
        return;
    if (port == 0x3de) {
        index = data;
        return;
    }
    if (port == 0x3df) {
        extended_w(data);
        return;
    }
    if (port >= 0x3c6 && port <= 0x3c9) {
        dac_w(port & 3, data);
        return;
    }
    if (ext[0x19] != 15 && (port & 0xfff0) == (0x300 | (ext[0x19] << 4))) {
        dac_w(port & 3, data);
        return;
    }
    switch (port) {
        case 0x3c0:
            attribute_write(data);
            recompute_params();
            return;
        case 0x3c2:
            ext[6] = (ext[6] & 12) | ((data >> 2) & 3);
            eeprom_update();
            miscellaneous_output = data;
            recompute_params();
            return;
        case 0x3c4:
            sequencer.index = data;
            return;
        case 0x3c5:
            sequencer_write(data);
            recompute_params();
            return;
        case 0x3ce:
            gc.index = data;
            return;
        case 0x3cf:
            gc_write(data);
            recompute_params();
            return;
    }
    if ((port & 0xfff0) == get_crtc_port()) {
        switch (port & 15) {
            case 4:
                crtc.index = data;
                break;
            case 5:
                crtc_write(data);
                recompute_params();
                break;
            case 10:
                feature_control = data;
                break;
        }
    }
}

// Read Oak extended registers, including muxed clock/EEPROM status and
// auto-incremented latch data (OTI-087X databook, extended register map).
// Oak register index guide (hex; databook pp. 31-46):
// 00 identification; 02 memory size; 03 test/VSYNC control; 04 local-bus
// control; 05 linear window; 06 clock select; 07/08 configuration straps;
// 09-0B scratch bytes; 0D compatibility aliases; 10 status; 11 bank aliases;
// 13 bus waits; 14 vertical overflow/interlace; 15 odd-field sync phase;
// 17 display-start high bits; 18 EEPROM pins; 19 alternate DAC-port base;
// 20 FIFO threshold; 21 display format; 22 memory/latch mode; 23 read bank;
// 24 write bank; 25 both banks; 30 color expansion/pixel-mask control;
// 31 foreground; 32 background; 33 expansion pattern; 34 pixel mask;
// 35 latch-byte index; 36 latch data with increment; 40/41 cursor X;
// 42/43 cursor Y; 44/45 cursor crop X/Y; 47-49 cursor address;
// 4A/4B cursor colors; 4C cursor enable/order/blink. Others read as $FF.
// FIFO is the chip's first-in-first-out pixel queue; this model stores its
// control fields but does not simulate individual queue entries.
uint8_t OTI087::extended_r() {
    // Index $00 bit 1 identifies the 087 (clear); index $02 bits 2:1 hold
    // the memory-size setting. Zero status/power-up values here are model
    // defaults, not guaranteed silicon reset values.
    switch (index) {
        case 0:
            return 0;
        case 2:
            return ext[2] & 6;
        case 0x0d:
            return (ext[0x20] & 7) | ((ext[0x21] & 12) << 1) | ((ext[6] & 4) << 3);
        case 0x11:
            return (ext[0x23] & 15) | ((ext[0x24] & 15) << 4);
        case 0x14:
            return (ext[0x14] & 0x87) | ((ext[0x17] & 1) << 3);
        case 0x18:
            if (side_effects)
                recompute_params(); // READY/BUSY can change between clocks
            return (ext[0x18] & 14) | bit(csel_pins(), 0);
        case 0x25:
            return ext[0x24];
        case 0x36: {
            uint8_t const data = latch[ext[0x35]];
            if (side_effects)
                ext[0x35] = (ext[0x35] + 1) & 7;
            return data;
        }
        case 1:
        case 0x0c:
        case 0x0e:
        case 0x0f:
        case 0x12:
        case 0x16:
        case 0x46:
            return 0xff;
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 0x10:
        case 0x13:
        case 0x15:
        case 0x17:
        case 0x19:
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
            return ext[index];
        default:
            return 0xff;
    }
}

// Mask reserved Oak register bits and apply bank, clock, cursor and EEPROM
// pin changes (OTI-087X databook, extended register descriptions).
void OTI087::extended_w(uint8_t data) {
    uint8_t mask = 0;
    switch (index) {
        case 2:
            mask = 6;
            break; // Keep the memory-size field; bit 0 is not writable in this model.
        case 3:
            mask = 0xf7;
            break; // OTI087 p. 33: coherent memory has no deferred writes to flush
        case 4:
            mask = 0x1f;
            break; // pp33-34, local bus only
        case 5:
            mask = 0xff;
            break; // pp34-35
        case 6:
            mask = 15;
            break; // p35
        case 9:
        case 10:
        case 11:
            mask = 0xff;
            break; // p37, scratch
        case 0x0d: // p38, aliases share storage
            ext[0x20] = (ext[0x20] & 8) | (data & 7);
            ext[0x21] = (ext[0x21] & ~12) | ((data >> 1) & 12);
            ext[6] = (ext[6] & ~4) | ((data >> 3) & 4);
            eeprom_update();
            recompute_params();
            return;
        case 0x11:
            ext[0x23] = (ext[0x23] & 16) | (data & 15);
            ext[0x24] = (ext[0x24] & 16) | (data >> 4);
            return;
        case 0x13:
            mask = 0xf8;
            break; // p39
        case 0x14:
            ext[0x17] = (ext[0x17] & 6) | bit(data, 3);
            mask = 0x87;
            break; // pp39-40; bit3 lives in index17
        case 0x15:
            mask = 0xff;
            break; // p40 (odd-field sync phase)
        case 0x17:
            mask = 7;
            break;
        case 0x18:
            mask = 15;
            break;
        case 0x19:
        case 0x20:
            mask = 15;
            break; // p41
        case 0x21:
            mask = 0x8f;
            break;
        case 0x22:
            mask = 0x3d;
            break; // p42
        case 0x23:
        case 0x24:
            mask = 31;
            break;
        case 0x25:
            ext[0x23] = ext[0x24] = data & 31;
            return;
        case 0x30:
            mask = 31;
            break; // p43
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
            mask = 0xff;
            break;
        case 0x35:
            mask = 7;
            break; // p44
        case 0x36:
            latch[ext[0x35]] = data;
            ext[0x35] = (ext[0x35] + 1) & 7;
            return;
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
            mask = 0xff;
            break;
        case 0x4c:
            mask = 0x3f;
            break; // pp44-46
        default:
            return; // read-only straps, unused and undocumented registers
    }
    ext[index] = data & mask;
    if (index == 6)
        miscellaneous_output = (miscellaneous_output & ~12) | ((data & 3) << 2);
    if (index == 0x18 || index == 6)
        eeprom_update();
    recompute_params();
}

// Combine VGA Miscellaneous Output and Oak extension bits into clock-select pins.
uint8_t OTI087::csel_pins() const {
    // p40: ext18 redirects the physical CSEL pins, not the ext06 storage.
    // The OTI-068 sees those same pins, including DO from the EEPROM.
    return bit(ext[0x18], 2) ? (ext[6] & 8) | (bit(ext[0x18], 3) << 2) | (bit(ext[0x18], 0) << 1) | eeprom.output(now)
                             : ext[6] & 15;
}

// Account for Oak extended horizontal addressing when deriving raster width.
unsigned OTI087::horizontal_multiplier() const {
    // p41. Setting both bits is not documented.
    return bit(ext[0x21], 1) ? 3 : bit(ext[0x21], 0) ? 2 : 1;
}

// Include the Bt481 input packing ratio in the VGA dot-to-pixel conversion.
unsigned OTI087::pixel_divisor() const {
    unsigned const dac = ramdac.pixels_per_clock_divisor();
    return dac * (bit(attribute.data[0x10], 6) ? 2 : 1);
}

// Combine the CRTC start-address latch with Oak address-extension bits.
uint32_t OTI087::latch_start_addr() {
    return (crtc.start_addr_latch & 0xffff) | ((ext[0x17] & 7) << 16);
}

// Evaluate the wrapped CRTC vertical retrace interval at the current beam line.
bool OTI087::vertical_retrace() {
    int const ht = timing.total_width, vt = crtc.vert_total + 2;
    int const field_y = vpos() % vt;
    bool const odd = get_interlace_mode() && vpos() >= vt;
    unsigned const dots = (!gc.alpha_dis && !bit(sequencer.data[1], 0)) ? 9 : 8;
    unsigned const char_width = dots * horizontal_multiplier();
    // OTI p40: odd-field vertical retrace starts at the character
    // position in index15, rather than invariably half a scan line.
    int const phase = odd ? (ext[0x15] & 0x7f) * char_width / pixel_divisor() : 0;
    int const length = (crtc.vert_retrace_end - crtc.vert_retrace_start) & 15;
    int const position = field_y * ht + hpos();
    int const sync_start = crtc.vert_retrace_start * ht + phase;
    return (position - sync_start + vt * ht * 2) % (vt * ht) < (length ? length : 16) * ht;
}

// Apply Oak VSYNC polarity/toggle controls to the beam retrace state
// (OTI-087X databook p. 33).
int OTI087::vsync_r() {
    // OTI-087X data book p. 33: either transition of ext03 bit5 toggles VSYNC.
    // Bit7 qualifies only the global modes in bits2:0. The saved register
    // bit supplies the test phase; repeated writes leave the pin unchanged.
    // Input Status 1 reports the retrace interval, before polarity/test.
    return vertical_retrace() ^ bit(miscellaneous_output, 7) ^ bit(ext[3], 5);
}

// Test horizontal and vertical blank intervals with their hardware wraparound.
bool OTI087::blanked(int x, int y) const {
    int const ht = crtc.horz_total + 5, vt = crtc.vert_total + 2;
    unsigned const dots = (!gc.alpha_dis && !bit(sequencer.data[1], 0)) ? 9 : 8;
    int const hx = x * pixel_divisor() / (dots * horizontal_multiplier());
    int const vy = y / (bit(ext[0x14], 7) ? 2 : 1);
    int const hb = (crtc.horz_blank_end - crtc.horz_blank_start) & 63;
    int const vb = (crtc.vert_blank_end - crtc.vert_blank_start) & 127;
    return (hx - crtc.horz_blank_start + ht * 2) % ht < (hb ? hb : 64) ||
           (vy - crtc.vert_blank_start + vt * 2) % vt < (vb ? vb : 128);
}

// Allocate board VRAM and establish chip state before any firmware access.
OTI087::OTI087() : memory(VRAM_SIZE) {
    next_blank = timing.frame_period();
    next_frame = std::numeric_limits<double>::infinity();
    next_line_zero = timing.line_period() * (timing.total_height - timing.height);
    reset();
}

// Reset control and raster scheduling while preserving nonvolatile EEPROM contents.
void OTI087::reset() {
    sequencer = {};
    crtc = {};
    gc = {};
    attribute = {};
    miscellaneous_output = feature_control = 0;
    crtc.maximum_scan_line = 1;
    crtc.line_compare = 0x3ff;
    std::fill(std::begin(ext), std::end(ext), 0);
    std::fill(std::begin(latch), std::end(latch), 0);
    ext[4] = 15;
    ext[7] = 0x56;
    ext[8] = 1;
    ext[0x10] = 0xff;
    ext[0x19] = 4;
    ext[0x20] = 1;
    ext[0x4c] = 0x10;
    index = setup = enable = 0;
    ready_until = now;
    next_latch = std::numeric_limits<double>::infinity();
    starts = {{0, 0, 0}};
    ramdac.reset();
    if (ready_callback)
        ready_callback(true);
    if (cint_callback)
        cint_callback(false);
    eeprom_update();
}

// Allow debugger inspection without advancing port-read latches or counters.
uint8_t OTI087::io_r(uint32_t port, bool effects) {
    side_effects = effects;
    uint8_t value = io_read_impl(port);
    side_effects = true;
    return value;
}

// Inspect VRAM without changing VGA latches, readiness or bus-visible state.
uint8_t OTI087::peek_memory(uint32_t address) {
    side_effects = false;
    uint8_t value = bus_memory_r(address);
    side_effects = true;
    return value;
}

// Perform a selected bus read and report its access wait requirement.
ss_video_access_t OTI087::memory_read(uint32_t address) {
    uint32_t offset;
    unsigned waits = decode_memory(address, false, offset) ? unsigned(std::ceil(access_wait() * 18000000 - 1e-9)) : 0;
    return {bus_memory_r(address), waits};
}

// Perform a bus write and report waits to the board memory decoder.
unsigned OTI087::memory_write(uint32_t address, uint8_t data) {
    uint32_t offset;
    unsigned waits =
        decode_memory(address, true, offset) ? unsigned(std::ceil(access_wait(true) * 18000000 - 1e-9)) : 0;
    bus_memory_w(address, data);
    return waits;
}

// Hold IOCHRDY until the modeled memory access completes; the board converts
// this deadline to Z180 WAIT sampling.
void OTI087::access_begin(bool write) {
    if (!side_effects)
        return;
    ready_until = now + access_wait(write);
    if (ready_callback)
        ready_callback(false);
}

// Read palette/control state selected by the VGA attribute index.
uint8_t OTI087::attribute_read() const {
    unsigned addr = attribute.index & 0x3f;
    if (addr < 16)
        return attribute.data[addr];
    if ((addr & 31) >= 16 && (addr & 31) <= 20)
        return attribute.data[addr & 31];
    return 0xff;
}

// Latch VGA attribute controls; pixel panning has a separate display latch.
void OTI087::attribute_write(uint8_t data) {
    if (!attribute.state)
        attribute.index = data;
    else {
        unsigned addr = attribute.index & 0x3f;
        if (addr < 16)
            attribute.data[addr] = data & 0x3f;
        else {
            addr &= 31;
            // IBM VGA/XGA TRM p2-94: overscan uses all eight DAC index bits.
            if (addr == 0x10 || addr == 0x11)
                attribute.data[addr] = data;
            if (addr == 0x12)
                attribute.data[addr] = data & 0x3f;
            if (addr == 0x13)
                attribute.pel_shift_latch = attribute.data[addr] = data & 15;
            if (addr == 0x14)
                attribute.data[addr] = data & 15;
        }
    }
    attribute.state = !attribute.state;
}

// Decode VGA plane enables and font selection for the memory and text paths.
void OTI087::sequencer_write(uint8_t data) {
    sequencer.data[sequencer.index] = data;
    if (sequencer.index == 2)
        sequencer.map_mask = data & 15;
    if (sequencer.index == 3) {
        sequencer.char_sel.A = ((data & 12) >> 1) | ((data & 0x20) >> 5);
        sequencer.char_sel.B = ((data & 3) << 1) | ((data & 0x10) >> 4);
    }
}

// Combine rotated CPU data with a VGA read latch under the selected bit mask.
uint8_t OTI087::vga_logical_op(uint8_t data, uint8_t plane, uint8_t mask) const {
    // Operation 0 replaces enabled bits, 1 ANDs, 2 ORs, 3 XORs with the
    // retained plane byte. Bits outside mask always keep their latch value.
    switch (gc.logical_op & 3) {
        case 0:
            return (data & mask) | (gc.latch[plane] & ~mask);
        case 1:
            return (data | ~mask) & gc.latch[plane];
        case 2:
            return (data & mask) | gc.latch[plane];
        default:
            return (data & mask) ^ gc.latch[plane];
    }
}

// Drive 93C46 DI/CS/SK from Oak control bits; pin multiplexing selects the
// clock/data source before the EEPROM sees an edge.
void OTI087::eeprom_update() {
    // DI=data input, CS=chip select, SK=serial clock. $18 bit 2 chooses
    // explicit EEPROM pins; otherwise clock-select outputs supply DI/SK
    // (Oak databook p. 41 and EEPROM interface p. 23).
    bool enabled = ext[0x18] & 4;
    bool cs = ext[0x18] & 2;
    bool di = enabled ? bit(ext[0x18], 0) : bit(ext[6], 1);
    bool sk = enabled ? bit(ext[0x18], 3) : bit(ext[6], 2);
    eeprom.drive(di, cs, sk, now);
    if (eeprom_callback)
        eeprom_callback(di, cs, sk);
}

// Select the board pixel-clock frequency from the four Oak CSEL outputs.
uint32_t OTI087::vclk_hz() const {
    // Hz for each CSEL[3:0] combination on the card clock generator.
    // OTI-068 clock-generator frequency table, printed p. 7. CSEL means
    // clock select; the four pins choose one entry without interpolation.
    static constexpr uint32_t clocks[] = {25175000, 28322000, 65000000, 44900000, 28322000, 36000000,
                                          40000000, 36000000, 25175000, 28322000, 78000000, 65000000,
                                          63000000, 72000000, 40000000, 50000000};
    return clocks[csel_pins()];
}

// Translate VGA counts and Oak extensions into raster totals, then reschedule
// blanking/latch events without resetting an unexpired frame epoch.
void OTI087::recompute_params() {
    crtc.vert_total = (crtc.vert_total & 0x3ff) | (bit(ext[0x14], 0) << 10);
    crtc.vert_blank_start = (crtc.vert_blank_start & 0x3ff) | (bit(ext[0x14], 1) << 10);
    crtc.vert_retrace_start = (crtc.vert_retrace_start & 0x3ff) | (bit(ext[0x14], 2) << 10);
    if (cint_callback)
        cint_callback(cint_r());
    // Firmware writes several registers to install a mode. Keep the previous
    // usable geometry until the totals describe a nonempty visible rectangle.
    if (!crtc.horz_total || !crtc.horz_disp_end || !crtc.vert_disp_end)
        return;
    unsigned dots = (!gc.alpha_dis && !bit(sequencer.data[1], 0)) ? 9 : 8;
    unsigned mult = horizontal_multiplier(), div = pixel_divisor();
    unsigned interlace = get_interlace_mode() ? 2 : 1;
    // VGA stores horizontal total minus five character clocks, display end
    // minus one, and vertical total minus two. Character clocks contain eight
    // or nine dots. Interlace adds a second field (IBM VGA/XGA CRTC registers;
    // Oak databook pp. 39-40). Convert all of them to output pixels/rows.
    int ht = (crtc.horz_total + 5) * dots * mult / div;
    int hd = (crtc.horz_disp_end + 1) * dots * mult / div;
    int vt = (crtc.vert_total + 2) * interlace;
    int vd = (crtc.vert_disp_end + 1) * interlace;
    if (!hd || hd >= ht || !vd || vd >= vt)
        return;
    uint32_t clock = vclk_hz() / (bit(sequencer.data[1], 3) ? 2 : 1) / (output_divider ? 2 : 1);
    timing = {hd, vd, ht, vt, clock, div};
    // Keep the last blank-start time when firmware retimes the raster.
    // Only a shortened period that has already elapsed starts a new blank.
    // This is the model's phase convention during partial mode programming.
    if (now - blank_origin >= timing.frame_period()) {
        blank_origin = now;
        next_frame = now + timing.line_period() * (vt - vd);
    }
    next_blank = now + time_until_pos(vd);
    next_line_zero = now + time_until_pos(0);
    if (vpos() == 0)
        starts = {{0, crtc.start_addr, attribute.pel_shift}};

    schedule_display_latch();
}

// Locate the raster line relative to the retained vertical-blank epoch.
int OTI087::vpos() const {
    double pixel_time = timing.pixel_period();
    int line = int((now - blank_origin + pixel_time / 2) / timing.line_period());
    return (timing.height + line) % timing.total_height;
}

// Locate the pixel within that raster line using the same rounded clock periods.
int OTI087::hpos() const {
    double pixel_time = timing.pixel_period();
    double delta = now - blank_origin + pixel_time / 2;
    int line = int(delta / timing.line_period());
    return int((delta - line * timing.line_period()) / pixel_time);
}

// Find the next occurrence of a beam position, wrapping into the next frame.
double OTI087::time_until_pos(int y, int x) const {
    double pixel_time = timing.pixel_period();
    int line = (y + timing.total_height - timing.height) % timing.total_height;
    double at = blank_origin + timing.line_period() * line + x * pixel_time;
    // A position at the current rounded pixel is already reached. Advance
    // by whole frames until it is strictly ahead, avoiding a zero-time event loop.
    while (at <= now + pixel_time / 2)
        at += timing.frame_period();
    return at - now;
}

// Schedule start-address/pan latching at programmed vertical blank, including
// the second field when interlace is selected.
void OTI087::schedule_display_latch() {
    unsigned total = crtc.vert_total + 2;
    unsigned blank = crtc.vert_blank_start % total;
    double delay = time_until_pos(blank);
    if (get_interlace_mode())
        delay = std::min(delay, time_until_pos(blank + total));
    next_latch = now + delay;
}

// Latch the scanout address/pan and vertical interrupt; record a row boundary
// only if the visible addressing changes.
void OTI087::display_latch() {
    uint32_t address = latch_start_addr();
    uint8_t pan = attribute.pel_shift_latch;
    // Keep only addressing changes, not one entry per scan line. The renderer
    // later replays these row boundaries without executing the chip again.
    if (address != crtc.start_addr || pan != attribute.pel_shift) {
        starts.push_back({vpos(), address, pan});
        crtc.start_addr = address;
        attribute.pel_shift = pan;
    }
    if (!crtc.irq_latch && !crtc.irq_disable && crtc.irq_clear) {
        crtc.irq_latch = 1;
        if (cint_callback)
            cint_callback(true);
    }
    schedule_display_latch();
}

// Return the earliest raster, IOCHRDY or EEPROM deadline to the card scheduler.
double OTI087::next_event() const {
    double ready = ready_until > now ? ready_until : std::numeric_limits<double>::infinity();
    return std::min({next_frame, next_blank, next_line_zero, next_latch, ready, eeprom.deadline(now)});
}

// Dispatch all chip events through the requested emulated time, preserving
// simultaneous raster, ready and EEPROM transitions.
void OTI087::advance(double seconds) {
    if (seconds < now)
        return;
    // Several signals can change at the same timestamp. Use independent ifs
    // below so none is lost; each handler must move its deadline forward or
    // disable it before the next loop iteration.
    while (next_event() <= seconds) {
        double event = next_event();
        bool ee = event == eeprom.deadline(now);
        bool ready = ready_until > now && event == ready_until;
        now = event;
        ramdac.advance(now);
        if (event == next_frame) {
            frames++;
            next_frame = std::numeric_limits<double>::infinity();
        }
        if (event == next_line_zero) {
            starts = {{0, crtc.start_addr, attribute.pel_shift}};
            next_line_zero = now + timing.frame_period();
        }
        if (event == next_blank) {
            blank_origin = now;
            next_blank = now + timing.frame_period();
            next_frame = now + timing.line_period() * (timing.total_height - timing.height);
        }
        if (event == next_latch)
            display_latch();
        if (ready && ready_callback)
            ready_callback(true);
        if (ee && (ext[0x18] & 4))
            recompute_params();
    }
    now = seconds;
    ramdac.advance(now);
}
