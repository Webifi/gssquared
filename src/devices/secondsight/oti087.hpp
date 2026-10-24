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

#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
#include "bt481.hpp"
#include "ss_eeprom.hpp"

struct video_system_t;
struct SDL_Texture;

// Pixel rectangle passed by value on the rendering thread. Inclusive edges
// match the CRTC last-pixel/last-line convention; inverted bounds are empty.
struct ss_video_rect_t {
    int left, right, top, bottom;
    // Number of included columns; use empty() before treating it as a size.
    int width() const { return right - left + 1; }
    // Number of included rows; use empty() before treating it as a size.
    int height() const { return bottom - top + 1; }
    // True when either pair of inclusive edges is inverted.
    bool empty() const { return right < left || bottom < top; }
    // Whether screen pixel (x,y) lies inside these inclusive edges.
    bool contains(int x, int y) const { return x >= left && x <= right && y >= top && y <= bottom; }
    // Return the overlap with b; an inverted result represents no overlap.
    ss_video_rect_t intersect(ss_video_rect_t b) const {
        return {std::max(left, b.left), std::min(right, b.right), std::max(top, b.top), std::min(bottom, b.bottom)};
    }
};

// Borrowed host surface used only by the rendering thread; pitch counts
// uint32_t pixels between rows, not bytes. The caller owns the allocation.
struct ss_video_buffer_t {
    uint32_t *pixels;
    int width, height, pitch; // pitch in uint32_t pixels
    // Full drawable rectangle of this borrowed buffer.
    ss_video_rect_t bounds() const { return {0, width - 1, 0, height - 1}; }
    // Borrow the pixel at row y, column x; callers clip coordinates first.
    uint32_t &pixel(int y, int x) { return pixels[y * pitch + x]; }
    // Paint area in opaque host color, clipping it to this buffer.
    void fill(uint32_t color, ss_video_rect_t area) {
        area = area.intersect(bounds());
        if (area.empty())
            return;
        for (int y = area.top; y <= area.bottom; y++)
            std::fill(pixels + y * pitch + area.left, pixels + y * pitch + area.right + 1, color);
    }
};

// Geometry/timing owned by OTI087 and borrowed by synchronized presentation.
// Totals include blanking; periods use integer attosecond rounding.
struct ss_video_timing_t {
    int width = 640, height = 480, total_width = 800, total_height = 525;
    uint32_t pixel_clock = 25175000;
    unsigned pixel_divisor = 1;
    // Preserve the reference model's integral clock-period rounding before
    // multiplying by the raster totals (one unit is 10^-18 seconds).
    uint64_t frame_units() const {
        uint64_t ticks = uint64_t(total_width) * total_height * pixel_divisor;
        // Clamp implausibly long programmed frames to one second. This is
        // a model bound, not an Oak clock-divider feature.
        return ticks >= pixel_clock ? 1000000000000000000ULL : (1000000000000000000ULL / pixel_clock) * ticks;
    }
    // Duration of one complete raster in seconds, including blanking.
    double frame_period() const { return double(frame_units()) * 1e-18; }
    // Duration of one complete raster row in seconds.
    double line_period() const { return double(frame_units() / total_height) * 1e-18; }
    // Duration of one output pixel position in seconds.
    double pixel_period() const { return double(frame_units() / (total_height * total_width)) * 1e-18; }
    // Active video rectangle; raster totals also include blank positions.
    ss_video_rect_t visible() const { return {0, width - 1, 0, height - 1}; }
};

enum class ss_video_mode_t { Text, Planar, VGA256, Packed, CGA };
// Caller-owned copy of mode, geometry and palette; safe to retain after the
// synchronized read because it contains no pointers into the card.
struct ss_video_frame_t {
    ss_video_mode_t mode;
    int width, height;
    unsigned pitch, start, depth; // packed: byte units; legacy: VGA offset/address units
    uint8_t palette[256][3];
};
// Value returned to SSCard on the execution thread: one byte plus wait
// periods measured at 18 MHz. SSCard converts the delay to its current clock.
struct ss_video_access_t {
    uint8_t data;
    unsigned waits;
};

// Owns VGA registers, one MiB of VRAM, the Bt481 and the serial EEPROM;
// the board supplies absolute time before accessing these devices.
class OTI087 {
public:
    static constexpr int MAX_FRAME_DIMENSION = 4096;
    static constexpr uint32_t VRAM_SIZE = 0x100000;
    // Allocate one MiB of VRAM and initialize deterministic power-up state.
    OTI087();
    // Reset registers and pending chip events; retain VRAM and EEPROM contents.
    void reset();
    // Absolute emulated seconds; call before bus accesses and at host frame end.
    void advance(double seconds);
    // Absolute emulated seconds last supplied by the board.
    double time() const { return now; }
    // Earliest pending chip transition in absolute seconds; infinity means none.
    double next_event() const;
    // Current horizontal output-pixel position, including blanking.
    int hpos() const;
    // Current raster row, including blanking and both interlaced fields.
    int vpos() const;
    // Count of frames completed, used by cursor and text blink logic.
    uint64_t frame_number() const { return frames; }
    // Positive seconds until the next occurrence of output position (x,y).
    double time_until_pos(int y, int x = 0) const;
    // Borrow geometry/timing; synchronize card execution before host inspection.
    const ss_video_timing_t &display_timing() const { return timing; }
    // Copy active mode, addressing units and resolved palette for inspection.
    ss_video_frame_t display_state() const;

    // Read the 16-bit port; effects=false peeks without consuming register state.
    uint8_t io_r(uint32_t port, bool effects = true);
    // Whether this enabled chip drives the specified port read.
    bool io_read_selected(uint16_t port) const;
    // Whether this board-supplied physical byte address selects video memory.
    bool memory_read_selected(uint32_t address) const {
        uint32_t offset;
        return decode_memory(address, false, offset);
    }
    // Write data to an indexed VGA/Oak port, applying its register side effects.
    void io_w(uint32_t port, uint8_t data);
    // Read a byte and report wait periods in units of a fixed 18 MHz clock.
    ss_video_access_t memory_read(uint32_t address);
    // Store data and return wait periods in units of a fixed 18 MHz clock.
    unsigned memory_write(uint32_t address, uint8_t data);
    // Decoded memory read with latch effects; unselected addresses return $FF.
    uint8_t bus_memory_r(uint32_t address);
    // Decoded memory write with plane/mask effects; unselected addresses do nothing.
    void bus_memory_w(uint32_t address, uint8_t data);
    // Convenience read using an already translated Oak address.
    uint8_t memory_r(uint32_t address) { return bus_memory_r(address); }
    // Convenience write using an already translated Oak address.
    void memory_w(uint32_t address, uint8_t data) { bus_memory_w(address, data); }
    // Read through the board window; half chooses its upper 512 KiB.
    uint8_t memory_r(uint32_t address, bool half) { return bus_memory_r(card_address(address, half)); }
    // Write through the board window with the same half-select translation.
    void memory_w(uint32_t address, bool half, uint8_t data) { bus_memory_w(card_address(address, half), data); }
    // Debugger read that leaves video latches and readiness unchanged.
    uint8_t peek_memory(uint32_t address);
    // Raw display-byte inspection, bypassing CPU aperture and latch effects.
    uint8_t mem_linear_r(uint32_t address) const { return linear_byte(address); }
    // Raw display-byte store, bypassing CPU aperture and latch effects.
    void mem_linear_w(uint32_t address, uint8_t data) { linear_byte_w(address, data); }

    // Apply the board output-clock divider and recompute raster timing.
    void output_divider_w(int state) {
        output_divider = bool(state);
        recompute_params();
    }
    // Select the board linear-address input used by convenience window accesses.
    void linear_address_w(int state) { linear_address = bool(state); }
    // Cable sense: nonzero=AppleColor RGB, zero=VGA; read by mode-setting firmware.
    void monitor_type_w(int applecolor) { monitor_override = !applecolor; }
    // Selected pixel oscillator frequency in Hz (Oak OTI-068 frequency table p. 7).
    uint32_t vclk_hz() const;
    // Board video-memory clock frequency, independent of the selected pixel clock.
    static constexpr uint32_t mclk_hz() { return 40000000; }
    // Read Oak AT-bus zero-wait enable, extended $13 bit 3 (databook p. 39).
    bool at_zero_wait_enabled() const { return ext[0x13] & 8; }
    // Set the modelled minimum read delay in seconds, clamped to two 18 MHz periods.
    void set_access_wait(double seconds) { access_delay = std::max(seconds, 2.0 / 18000000); }
    // Delay in seconds; writes require at least three 18 MHz periods in this model.
    double access_wait(bool write = false) const { return std::max(access_delay, double(write ? 3 : 2) / 18000000); }
    // IOCHRDY (I/O channel ready) is high once the modelled access delay expires.
    bool iochrdy_r() const { return now >= ready_until; }
    // Whether the enabled vertical interrupt latch requests service.
    bool cint_r() const { return crtc.irq_latch && !crtc.irq_disable; }
    // Electrical vertical-sync output after Oak polarity/test controls.
    int vsync_r();
    // Electrical output is CINTn; true here means asserted (low at the pin).
    std::function<void(bool)> cint_callback;
    std::function<void(bool)> ready_callback;
    std::function<void(bool, bool, bool)> eeprom_callback; // DI, CS, SK observation
    // Borrow the palette/converter owned by this chip; caller must synchronize.
    Bt481 &dac() { return ramdac; }
    // Resolve the eight-bit overscan (border) palette index through the Bt481.
    VideoColor border_color() const { return ramdac.pixel_select(attribute.data[0x11]); }
    // Borrow persistent configuration memory for main-thread load/save.
    SerialEEPROM93C46 &serial_eeprom() { return eeprom; }

    // Native ARGB output. Rendering is deferred to frame end. A start-address
    // latch change records a row boundary, never a timer for every scanline.
    // Draw the clipped region into borrowed pixels; do not advance chip time.
    void render(ss_video_buffer_t surface, ss_video_rect_t clip);
    // Draw all rows with the display-address changes recorded for this frame.
    void render_frame(ss_video_buffer_t surface);
    // Copy changed rows to caller-owned SDL texture; true means upload succeeded.
    bool upload_frame(SDL_Texture *texture, bool full = false);

private:
    uint8_t miscellaneous_output;
    uint8_t feature_control;

    // Sequencer registers: plane-write enables, font banks and pixel-clock division.
    struct {
        uint8_t index;
        uint8_t data[0x100];
        uint8_t map_mask;
        // Font-bank selectors used by text attribute bit 3.
        struct {
            uint8_t A, B;
        } char_sel;
    } sequencer;

    // CRTC counters and display-address latches, including split-screen and IRQ state.
    struct {
        uint8_t index;
        uint8_t data[0x100];
        uint16_t horz_total;
        uint16_t horz_disp_end;
        uint8_t horz_blank_start;
        uint8_t horz_blank_end;
        uint8_t horz_retrace_start;
        uint8_t horz_retrace_skew;
        uint8_t horz_retrace_end;
        uint8_t disp_enable_skew;
        uint8_t evra;
        uint16_t vert_total;
        uint16_t vert_disp_end;
        uint16_t vert_retrace_start;
        uint8_t vert_retrace_end;
        uint16_t vert_blank_start;
        uint16_t line_compare;
        uint32_t cursor_addr;
        uint8_t byte_panning;
        uint8_t preset_row_scan;
        uint8_t scan_doubling;
        uint8_t maximum_scan_line;
        uint8_t cursor_enable;
        uint8_t cursor_scan_start;
        uint8_t cursor_skew;
        uint8_t cursor_scan_end;
        uint32_t start_addr;
        uint32_t start_addr_latch;
        uint8_t protect_enable;
        uint8_t bandwidth;
        uint16_t offset;
        uint8_t word_mode;
        uint8_t dw;
        uint8_t div4;
        uint8_t underline_loc;
        uint16_t vert_blank_end;
        uint8_t sync_en;
        uint8_t aw;
        uint8_t div2;
        uint8_t sldiv;
        uint8_t map14;
        uint8_t map13;
        uint8_t irq_clear;
        uint8_t irq_disable;
        uint8_t irq_latch;
        uint8_t no_wrap;
    } crtc;

    // Graphics controller: read latches and CPU write/compare/mask settings.
    struct {
        uint8_t index;
        uint8_t latch[4];
        uint8_t set_reset;
        uint8_t enable_set_reset;
        uint8_t color_compare;
        uint8_t logical_op;
        uint8_t rotate_count;
        uint8_t shift256;
        uint8_t shift_reg;
        uint8_t read_map_sel;
        uint8_t read_mode;
        uint8_t write_mode;
        uint8_t color_dont_care;
        uint8_t bit_mask;
        uint8_t alpha_dis;
        uint8_t memory_map_sel;
        uint8_t host_oe;
        uint8_t chain_oe;
    } gc;

    // Attribute controller: 16-color lookup, blink, pixel panning and index/data phase.
    struct {
        uint8_t index, data[0x15];
        int state;
        uint8_t prot_bit;
        uint8_t pel_shift;
        uint8_t pel_shift_latch;
    } attribute;

    std::vector<uint8_t> memory;
    uint8_t ext[0x80] = {}, latch[8] = {};
    uint8_t index = 0, setup = 0, enable = 0;
    bool output_divider = false, linear_address = false, side_effects = true;
    int monitor_override = -1;
    Bt481 ramdac;
    SerialEEPROM93C46 eeprom;
    ss_video_timing_t timing;
    double now = 0, blank_origin = 0, next_frame = 0, next_blank = 0, next_line_zero = 0, next_latch = 0;
    double ready_until = 0, access_delay = 2.0 / 18000000;
    uint64_t frames = 0;
    // Display-address/pan value from a latch edge at this raster line. Owned
    // by OTI087; execution records it, synchronized rendering consumes it.
    struct start_change_t {
        int line;
        uint32_t address;
        uint8_t pan;
    };
    std::vector<start_change_t> starts;
    std::vector<uint32_t> frame_pixels;
    // Derived display data only. Compare source bytes and DAC output before
    // reusing a row; register writes and debug captures need no dirty hooks.
    std::vector<uint8_t> packed_source, packed_row;
    std::array<uint32_t, 256> packed_palette{};
    int packed_width = 0, packed_height = 0;
    uint8_t packed_command = 0;
    bool packed_valid = false, packed_asleep = false;
    int present_top = 0, present_bottom = -1;
    int present_width = 0, present_height = 0;

    // Return whether Oak setup/enable bits permit VGA register and memory access (databook pp. 29-30).
    bool enabled() const;
    // Decode port and return its register byte; side_effects controls destructive reads.
    uint8_t io_read_impl(uint32_t port);
    // Decode programmed counters/clocks into raster geometry and event times
    // (Oak pp. 39-40; IBM VGA/XGA Technical Reference, CRTC registers).
    void recompute_params();
    // Schedule the edge where programmed start/pan values become active (Oak pp. 39-40).
    void schedule_display_latch();
    // Copy programmed start/pan into active values and record the affected raster row.
    void display_latch();
    // Lower readiness for the modelled memory delay; write selects read or write latency.
    void access_begin(bool write);
    // Drive serial EEPROM data/select/clock from extended $18 and report the pin levels (Oak p. 41).
    void eeprom_update();
    // Return oscillator-select bits after board wiring and extended-register decoding (Oak pp. 40-41).
    uint8_t csel_pins() const;
    // Return how many output pixel positions each CRTC character count spans.
    unsigned horizontal_multiplier() const;
    // Return the selected clock-to-output-pixel divisor, including board clock division.
    unsigned pixel_divisor() const;
    // Form the active screen address from VGA low bits and Oak extension bits (Oak pp. 39-40).
    uint32_t latch_start_addr();
    // Oak $14 bit 7 selects alternate even/odd-line fields (databook pp. 39-40).
    bool get_interlace_mode() const { return ext[0x14] & 0x80; }
    // Return whether the raster is inside the programmed vertical-sync interval, including wrap.
    bool vertical_retrace();
    // Miscellaneous Output bit 0 chooses monochrome $3B0 or color $3D0 ports.
    uint16_t get_crtc_port() const { return miscellaneous_output & 1 ? 0x3d0 : 0x3b0; }
    // Read the Oak register selected by index; status/EEPROM bits reflect live pins (Oak pp. 31-46).
    uint8_t extended_r();
    // Write data to the selected Oak register with its mask and side effects (Oak pp. 31-46).
    void extended_w(uint8_t data);
    // Forward register-select pins to the Bt481, honoring debugger peek mode.
    uint8_t dac_r(uint8_t rs) { return ramdac.vga_r(rs, side_effects); }
    // Forward a VGA DAC write and invalidate cached colors used by scanout.
    void dac_w(uint8_t rs, uint8_t data) {
        ramdac.vga_w(rs, data);
        recompute_params();
    }
    // Return the selected CRTC register with unused/read-only bits handled as VGA defines.
    uint8_t crtc_read() const;
    // Apply data to the selected CRTC counter/control field and retain register protection rules.
    void crtc_write(uint8_t data);
    // Return the selected graphics-controller field; undefined indices return 0xff.
    uint8_t gc_read() const;
    // Apply data to the selected graphics plane, latch, compare or mask field.
    void gc_write(uint8_t data);
    // Return the attribute-controller register selected by its index, or 0xff outside its map.
    uint8_t attribute_read() const;
    // Use the index/data toggle to interpret data; status reads reset that toggle (IBM VGA attribute registers).
    void attribute_write(uint8_t data);
    // Store data in the selected sequencer register and decode plane/font selections.
    void sequencer_write(uint8_t data);
    // Return the programmed row stride in the addressing units of the selected mode.
    uint16_t offset() const;
    // Return the byte produced for plane 0-3 by the selected VGA write mode,
    // using CPU data, retained read latch and write masks (IBM VGA graphics registers).
    uint8_t vga_latch_write(int plane, uint8_t data);
    // Rotate CPU write data by graphics-controller count, wrapping the byte.
    uint8_t rotate_right(uint8_t value) const { return (value >> gc.rotate_count) | (value << (8 - gc.rotate_count)); }
    // Combine data and the retained plane latch by replace/AND/OR/XOR;
    // mask selects changed bits and unmasked bits retain the latch value.
    uint8_t vga_logical_op(uint8_t data, uint8_t plane, uint8_t mask) const;
    // Oak $21 bits 3:2 = 01 selects packed pixels (databook p. 42).
    bool packed() const { return (ext[0x21] & 12) == 4; }
    // Combine the board half-select with the Z180 VGA aperture address.
    uint32_t card_address(uint32_t address, bool half) const {
        return (address & 0x7ffff) | (half ? 0x80000 : 0) | (linear_address ? 0x100000 : 0);
    }
    // Test whether address selects the configured aperture. On true, write
    // the translated offset; write selects read versus write bank (Oak pp. 42-46).
    bool decode_memory(uint32_t address, bool write, uint32_t &offset) const;
    // Convert a linear display address to the four-plane backing-store layout.
    uint32_t storage_address(uint32_t address) const;
    // Return a raw linear display byte after storage-address translation; no CPU latch effects.
    uint8_t linear_byte(uint32_t address) const;
    // Store data at a raw linear display address after storage translation.
    void linear_byte_w(uint32_t address, uint8_t data);
    // Translate chain-four addressing: the low two address bits choose a VGA plane.
    uint32_t chain4_address(uint32_t address) const;
    // Read address into all plane latches, then return the selected plane or
    // color-compare result (IBM VGA graphics-controller read modes).
    uint8_t planar_r(uint32_t address);
    // Apply a CPU byte write at address using map mask, write mode and retained latches.
    void planar_w(uint32_t address, uint8_t data);
    // Apply the Oak packed-pixel write/expansion mode at address using byte data (Oak pp. 42-46).
    void packed_w(uint32_t address, uint8_t data);
    // Render clipped packed pixels into borrowed surface using Bt481 color conversion.
    void draw_packed(ss_video_buffer_t &surface, const ss_video_rect_t &clip);
    // Render packed rows with source/palette reuse; return false when another
    // mode or frame change requires the general renderer. surface remains caller-owned.
    bool render_packed_frame(ss_video_buffer_t surface);
    // Render clipped character cells, font/attribute bytes and text cursor into surface.
    void draw_text(ss_video_buffer_t &surface, const ss_video_rect_t &clip);
    // Combine one bit from each of four planes into a 16-color pixel in surface.
    void draw_planar(ss_video_buffer_t &surface, const ss_video_rect_t &clip);
    // Render chained/unchained VGA 256-color bytes through the palette into surface.
    void draw_vga256(ss_video_buffer_t &surface, const ss_video_rect_t &clip);
    // Render the legacy Color Graphics Adapter compatible two-bit pixel organization.
    void draw_cga(ss_video_buffer_t &surface, const ss_video_rect_t &clip);
    // Overlay the Oak two-plane hardware cursor within clip (Oak pp. 34-36).
    void draw_cursor(ss_video_buffer_t &surface, const ss_video_rect_t &clip);
    // Return whether output pixel x,y lies in a programmed blank interval, including wrap.
    bool blanked(int x, int y) const;
    // Resolve attribute palette entry index through VGA color selection and the Bt481.
    VideoColor attribute_color(uint8_t index) const;
};
