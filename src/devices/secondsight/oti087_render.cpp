/*
 *   SPDX-License-Identifier: BSD-3-Clause
 *   Copyright holders: Nathan Woods, Peter Trauner, Angelo Salese.
 *   See secondsight-video.LICENSE for the BSD-3-Clause terms.
 *
 *   Turn Second Sight's Oak OTI-087X video RAM (VRAM) into a host pixel image.
 *   The CRTC (cathode-ray-tube controller) supplies the starting address, row
 *   stride and screen dimensions. Text fetches characters and font rows; planar
 *   modes combine bits from four separate memory planes; packed modes fetch
 *   consecutive pixel bytes. The Brooktree Bt481 palette/converter supplies
 *   RGB (red, green, blue) colors. Scanout means these display-memory reads.
 *
 *   SecondSightDisplay calls the renderer on the main thread after SSCard has
 *   synchronized. OTI087 owns memory, registers and cached rows; callers own the
 *   destination surface. The output is opaque ARGB8888: eight bits each for
 *   alpha, red, green and blue. Rendering never advances the processor clock.
 *   A display-address latch retains the address until its scheduled load;
 *   recorded changes split the image into rows using the appropriate address.
 *
 *   Sources: Oak OTI-087 February 1993 databook pp. 39-46 and OTI-087X addendum;
 *   IBM VGA/XGA Technical Reference May 1992 pp. 2-72 to 2-95 (address generation
 *   and palettes); Brooktree Bt481A L481A_C pp. 7-8 (direct-color bit positions).
 *   SIMD means one host instruction handles several pixels; scalar fallbacks
 *   apply the same conversion one pixel at a time.
 */

#include "oti087.hpp"
#include "vga_render_text_9x16.hpp"
#include <cstring>
#if defined(__wasm_simd128__)
#include <wasm_simd128.h>
#endif
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

namespace {
// Extract bit n as zero or one when decoding a documented register field.
constexpr unsigned bit(uint32_t value, unsigned n) {
    return (value >> n) & 1;
}
// Two cursor bits choose color 0, color 1, leave the screen alone, or invert
// its RGB bits. This is the model's interpretation of the Oak cursor planes.
enum class CursorOp { color0, color1, transparent, invert };
constexpr CursorOp cursor_codes[] = {CursorOp::color0, CursorOp::color1, CursorOp::transparent, CursorOp::invert};

// VRAM retains four 64K planes per 256K block. Unpack a scanline in wide
// groups, splitting at each block boundary so the hardware wrap is retained.
// dest receives count bytes starting at display-byte address in memory.
// Leading/trailing bytes use scalar reads; whole groups interleave the planes.
void packed_bytes(uint8_t *dest, uint8_t const *memory, uint32_t address, unsigned count) {
    auto byte = [&](uint32_t a) { return memory[(a & 0xc0000) | ((a & 3) << 16) | ((a >> 2) & 0xffff)]; };
    while (count && (address & 3)) {
        *dest++ = byte(address++);
        --count;
    }
    // Limit each group to the current 256 KiB block before forming plane
    // pointers; the next iteration recalculates them across the wrap.
    while (count >= 4) {
        unsigned groups = std::min(count / 4, (0x40000 - (address & 0x3ffff)) / 4);
        uint8_t const *p = memory + (address & 0xc0000) + ((address >> 2) & 0xffff);
        unsigned n = groups;
#if defined(__wasm_simd128__)
        while (n >= 16) {
            auto a = wasm_v128_load(p), b = wasm_v128_load(p + 0x10000);
            auto c = wasm_v128_load(p + 0x20000), d = wasm_v128_load(p + 0x30000);
            auto ab0 = wasm_i8x16_shuffle(a,b,0,16,1,17,2,18,3,19,4,20,5,21,6,22,7,23);
            auto ab1 = wasm_i8x16_shuffle(a,b,8,24,9,25,10,26,11,27,12,28,13,29,14,30,15,31);
            auto cd0 = wasm_i8x16_shuffle(c,d,0,16,1,17,2,18,3,19,4,20,5,21,6,22,7,23);
            auto cd1 = wasm_i8x16_shuffle(c,d,8,24,9,25,10,26,11,27,12,28,13,29,14,30,15,31);
            wasm_v128_store(dest, wasm_i16x8_shuffle(ab0,cd0,0,8,1,9,2,10,3,11));
            wasm_v128_store(dest+16, wasm_i16x8_shuffle(ab0,cd0,4,12,5,13,6,14,7,15));
            wasm_v128_store(dest+32, wasm_i16x8_shuffle(ab1,cd1,0,8,1,9,2,10,3,11));
            wasm_v128_store(dest+48, wasm_i16x8_shuffle(ab1,cd1,4,12,5,13,6,14,7,15));
            p += 16; dest += 64; n -= 16;
        }
#elif defined(__ARM_NEON)
        while (n >= 16) {
            uint8x16x4_t lanes = {{vld1q_u8(p), vld1q_u8(p + 0x10000), vld1q_u8(p + 0x20000), vld1q_u8(p + 0x30000)}};
            vst4q_u8(dest, lanes);
            p += 16;
            dest += 64;
            n -= 16;
        }
#endif
        while (n--) {
            dest[0] = p[0];
            dest[1] = p[0x10000];
            dest[2] = p[0x20000];
            dest[3] = p[0x30000];
            ++p;
            dest += 4;
        }
        count -= groups * 4;
        address += groups * 4;
    }
    while (count--)
        *dest++ = byte(address++);
}

// Expand packed pixels with exactly the Bt481 channel bit placement; vector
// and scalar paths must leave unused low color bits zero (L481A_C pp. 7-8).
// Convert count pixels from source bytes to dest ARGB words. palette has
// 256 resolved Bt481 colors for indexed mode; direct modes use command bits.
void packed_colors(uint32_t *dest, uint8_t const *source, unsigned count, Bt481 const &dac, uint32_t const *palette) {
    // Sleep outputs black. Indexed bytes use the already masked palette;
    // direct color decodes two/three/four input bytes for each output pixel.
    unsigned const bytes = dac.bytes_per_pixel();
    uint8_t const command = dac.command_r();
    if (dac.asleep()) {
        std::fill_n(dest, count, 0xff000000);
        return;
    }
    if (bytes == 1) {
        while (count >= 4) {
            dest[0] = palette[source[0]];
            dest[1] = palette[source[1]];
            dest[2] = palette[source[2]];
            dest[3] = palette[source[3]];
            source += 4;
            dest += 4;
            count -= 4;
        }
        while (count--)
            *dest++ = palette[*source++];
        return;
    }
#if defined(__wasm_simd128__)
    if (bytes == 2) {
        auto colors = [command](v128_t pixels) {
            auto blue = wasm_v128_and(wasm_i32x4_shl(pixels,3), wasm_i32x4_splat(0xf8));
            auto green = wasm_v128_and(wasm_i32x4_shl(pixels, command & 0x40 ? 5 : 6),
                                      wasm_i32x4_splat(command & 0x40 ? 0xfc00 : 0xf800));
            auto red = wasm_v128_and(wasm_i32x4_shl(pixels, command & 0x40 ? 8 : 9), wasm_i32x4_splat(0xf80000));
            return wasm_v128_or(wasm_i32x4_splat(0xff000000), wasm_v128_or(red,wasm_v128_or(green,blue)));
        };
        while (count >= 8) {
            auto pixels = wasm_v128_load(source);
            wasm_v128_store(dest, colors(wasm_u32x4_extend_low_u16x8(pixels)));
            wasm_v128_store(dest+4, colors(wasm_u32x4_extend_high_u16x8(pixels)));
            source += 16; dest += 8; count -= 8;
        }
    } else if (bytes == 3) {
        // A load covers 16 bytes; keep at least eight pixels before loading
        // twelve bytes of output, so the final source row is never overread.
        const auto order = command & 2 ? wasm_i8x16_make(0,1,2,16,3,4,5,16,6,7,8,16,9,10,11,16)
                                      : wasm_i8x16_make(2,1,0,16,5,4,3,16,8,7,6,16,11,10,9,16);
        while (count >= 8) {
            auto pixels = wasm_i8x16_swizzle(wasm_v128_load(source),order);
            wasm_v128_store(dest, wasm_v128_or(pixels,wasm_i32x4_splat(0xff000000)));
            source += 12; dest += 4; count -= 4;
        }
    }
#elif defined(__ARM_NEON) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    if (bytes == 2) {
        while (count >= 8) {
            uint16x8_t pixels = vreinterpretq_u16_u8(vld1q_u8(source));
            uint8x8_t red = command & 0x40 ? vshrn_n_u16(pixels, 8) : vshrn_n_u16(pixels, 7);
            uint8x8_t green = command & 0x40 ? vshrn_n_u16(pixels, 3) : vshrn_n_u16(pixels, 2);
            uint8x8x4_t rgb = {{vand_u8(vshl_n_u8(vmovn_u16(pixels), 3), vdup_n_u8(0xf8)),
                                vand_u8(green, vdup_n_u8(command & 0x40 ? 0xfc : 0xf8)), vand_u8(red, vdup_n_u8(0xf8)),
                                vdup_n_u8(0xff)}};
            vst4_u8(reinterpret_cast<uint8_t *>(dest), rgb);
            source += 16;
            dest += 8;
            count -= 8;
        }
    } else if (bytes == 3) {
        while (count >= 16) {
            uint8x16x3_t pixels = vld3q_u8(source);
            uint8x16x4_t rgb = {
                {pixels.val[command & 2 ? 0 : 2], pixels.val[1], pixels.val[command & 2 ? 2 : 0], vdupq_n_u8(0xff)}};
            vst4q_u8(reinterpret_cast<uint8_t *>(dest), rgb);
            source += 48;
            dest += 16;
            count -= 16;
        }
    }
#endif
    while (count--) {
        if (bytes == 2) {
            uint16_t pixel = source[0] | (uint16_t(source[1]) << 8);
            *dest++ = command & 0x40 ? VideoColor((pixel >> 8) & 0xf8, (pixel >> 3) & 0xfc, (pixel << 3) & 0xf8)
                                     : VideoColor((pixel >> 7) & 0xf8, (pixel >> 2) & 0xf8, (pixel << 3) & 0xf8);
        } else if (bytes == 3) {
            *dest++ =
                command & 2 ? VideoColor(source[2], source[1], source[0]) : VideoColor(source[0], source[1], source[2]);
        } else {
            uint32_t pixel =
                source[0] | (uint32_t(source[1]) << 8) | (uint32_t(source[2]) << 16) | (uint32_t(source[3]) << 24);
            *dest++ = dac.direct_color(pixel);
        }
        source += bytes;
    }
}
} // namespace

// Scan packed pixels using Oak start/offset units and the active Bt481 depth.
void OTI087::draw_packed(ss_video_buffer_t &surface, ss_video_rect_t const &clip) {
    unsigned const bytes = ramdac.bytes_per_pixel();
    unsigned const repeat = crtc.maximum_scan_line * (crtc.scan_doubling + 1);
    unsigned const interlace = get_interlace_mode() ? 2 : 1;
    // The packed-mode pitch uses two eight-byte fetches per offset unit.
    // Bypassing the VGA word/dword mux is inferred from Oak BIOS settings;
    // Second Sight ROM1.4 mode13 also sets VGA dword/PEL-width bits.
    uint32_t const pitch = crtc.offset * 16;
    uint32_t const start = crtc.start_addr * 8;
    std::array<uint32_t, 256> palette{};
    if (bytes == 1)
        for (unsigned i = 0; i < palette.size(); ++i)
            palette[i] = ramdac.pixel_select(i);
    packed_row.resize(clip.width() * bytes);
    for (int y = clip.top; y <= clip.bottom; ++y) {
        unsigned const field_line = y / interlace;
        bool const split = field_line > crtc.line_compare;
        unsigned const row = (split ? field_line - crtc.line_compare - 1 : field_line + crtc.preset_row_scan) / repeat;
        uint32_t const address = (split ? 0 : start) + (row * interlace + (y % interlace)) * pitch;
        packed_bytes(packed_row.data(), memory.data(), address + clip.left * bytes, packed_row.size());
        packed_colors(&surface.pixel(y, clip.left), packed_row.data(), clip.width(), ramdac, palette.data());
    }
}

// Resolve the VGA attribute palette and color-select bits before DAC lookup.
VideoColor OTI087::attribute_color(uint8_t index) const {
    index &= attribute.data[0x12] & 15;
    uint8_t palette = attribute.data[index];
    if (bit(attribute.data[0x10], 7))
        palette = (palette & 15) | ((attribute.data[0x14] & 3) << 4);
    palette |= (attribute.data[0x14] & 12) << 4;
    return ramdac.pixel_select(palette);
}

// Fetch character, attribute and font-plane bytes at CRTC addresses; apply
// blink and cursor rules before converting the palette index to color.
void OTI087::draw_text(ss_video_buffer_t &surface, ss_video_rect_t const &clip) {
    // Standard VGA character/attribute planes 0/1 and font plane2. Use the
    // same physical VRAM translation as CPU accesses, including odd/even mode.
    unsigned const dots = bit(sequencer.data[1], 0) ? 8 : 9;
    unsigned const scan_repeat = crtc.scan_doubling + 1;
    unsigned const cell_height = crtc.maximum_scan_line * scan_repeat;
    unsigned const pitch = offset() / 2;
    unsigned const pan = dots == 9 ? (attribute.pel_shift + 1) % 9 : attribute.pel_shift & 7;
    bool const blink = bit(attribute.data[0x10], 3);
    bool const blink_off = bit(frames, 5);
    for (int y = clip.top; y <= clip.bottom; ++y) {
        bool const split = y > crtc.line_compare;
        unsigned const scan = split ? y - crtc.line_compare - 1 : y + crtc.preset_row_scan;
        unsigned const glyph_row = (scan % cell_height) / scan_repeat;
        uint32_t const start = (split ? 0 : crtc.start_addr) + (scan / cell_height) * pitch;
        for (int x = clip.left; x <= clip.right;) {
            unsigned dx = x + ((split && bit(attribute.data[0x10], 5)) ? 0 : pan);
            uint32_t cell = start + dx / dots;
            uint8_t ch = linear_byte(cell * 4), attr = linear_byte(cell * 4 + 1);
            unsigned bank = bit(attr, 3) ? sequencer.char_sel.A : sequencer.char_sel.B;
            uint32_t font = bank * 0x2000 + ch * 32 + (glyph_row & 31);
            uint8_t glyph = linear_byte(font * 4 + 2);
            bool cursor = crtc.cursor_enable && bit(frames, 4) && cell == crtc.cursor_addr &&
                          glyph_row >= crtc.cursor_scan_start && glyph_row <= crtc.cursor_scan_end;
            uint32_t row_pixels[9];
            vga_raster_text_glyph_row(row_pixels, dots, glyph, ch, bit(attribute.data[0x10], 2),
                                      blink && blink_off && bit(attr, 7), cursor, attribute_color(attr & 15),
                                      attribute_color((attr >> 4) & (blink ? 7 : 15)));
            for (unsigned column = dx % dots; column < dots && x <= clip.right; column++, x++)
                surface.pixel(y, x) = row_pixels[column];
        }
    }
}

// Combine one bit from each VGA plane into the attribute-controller index.
void OTI087::draw_planar(ss_video_buffer_t &surface, ss_video_rect_t const &clip) {
    unsigned const repeat = crtc.maximum_scan_line * (crtc.scan_doubling + 1);
    unsigned const interlace = get_interlace_mode() ? 2 : 1;
    for (int y = clip.top; y <= clip.bottom; ++y) {
        unsigned const field_line = y / interlace;
        bool const split = field_line > crtc.line_compare;
        unsigned const row = (split ? field_line - crtc.line_compare - 1 : field_line + crtc.preset_row_scan) / repeat;
        uint32_t const start = (split ? 0 : crtc.start_addr) + (row * interlace + y % interlace) * offset();
        unsigned const pan = (split && bit(attribute.data[0x10], 5)) ? 0 : attribute.pel_shift & 7;
        for (int x = clip.left; x <= clip.right; ++x) {
            unsigned const dx = x + pan;
            uint32_t const address = (start + dx / 8) * 4;
            uint8_t color = 0;
            for (unsigned p = 0; p != 4; ++p)
                color |= bit(linear_byte(address + p), 7 - (dx & 7)) << p;
            surface.pixel(y, x) = attribute_color(color);
        }
    }
}

// Interpret VGA 256-color shift mode using CRTC address generation; this
// layout differs from Oak packed-pixel scanout.
void OTI087::draw_vga256(ss_video_buffer_t &surface, ss_video_rect_t const &clip) {
    // Four consecutive PELs come from the four maps at one CRTC address.
    // Use the same plane storage as CPU accesses, independently of CH4.
    // The output geometry already accounts for attribute PW (pixel width),
    // so only the remaining shift-mode repetition is applied here.
    unsigned const scan_repeat = crtc.scan_doubling + 1;
    unsigned const repeat = crtc.maximum_scan_line * scan_repeat;
    unsigned const interlace = get_interlace_mode() ? 2 : 1;
    unsigned const pixel_repeat = bit(attribute.data[0x10], 6) ? 1 : 2;
    unsigned const count_divisor = crtc.div4 ? 4 : crtc.div2 ? 2 : 1;
    for (int y = clip.top; y <= clip.bottom; ++y) {
        unsigned const field_line = y / interlace;
        bool const split = field_line > crtc.line_compare;
        unsigned const scan = split ? field_line - crtc.line_compare - 1 : field_line + crtc.preset_row_scan;
        unsigned const row_scan = (scan % repeat) / scan_repeat;
        uint32_t const start =
            (split ? 0 : crtc.start_addr) + ((scan / repeat) * interlace + y % interlace) * crtc.offset * 2;
        unsigned const pan = (split && bit(attribute.data[0x10], 5)) ? 0 : (attribute.pel_shift & 6) >> 1;
        for (int x = clip.left; x <= clip.right; ++x) {
            unsigned const dx = x / pixel_repeat + pan;
            uint32_t const counter = start + crtc.byte_panning + dx / (4 * count_divisor);
            uint32_t address = counter;
            // IBM TRM Figure2-65, p2-75. DW takes precedence over WB.
            // Retain the full MA12/MA13 recirculation, including 16K boundaries.
            if (crtc.dw)
                address = (counter & ~uint32_t(0xffff)) | ((counter << 2) & 0xfffc) | ((counter >> 12) & 3);
            else if (!crtc.word_mode)
                address = (counter & ~uint32_t(0xffff)) | ((counter << 1) & 0xfffe) | bit(counter, crtc.aw ? 15 : 13);
            if (!crtc.map13)
                address = (address & ~uint32_t(0x2000)) | (bit(row_scan, 0) << 13);
            if (!crtc.map14)
                address = (address & ~uint32_t(0x4000)) | (bit(row_scan, 1) << 14);
            surface.pixel(y, x) = ramdac.pixel_select(linear_byte((address << 2) | (dx & 3)));
        }
    }
}

// Overlay the Oak two-plane hardware cursor after the underlying raster.
// Cursor storage and position come from the OTI-087X extended registers.
void OTI087::draw_cursor(ss_video_buffer_t &surface, ss_video_rect_t const &clip) {
    uint8_t const control = ext[0x4c];
    if (!bit(control, 0) || ramdac.asleep())
        return;
    if (bit(control, 3) && bit(frames, 2 + ((control >> 4) & 3)))
        return;
    unsigned const px = ext[0x44], py = ext[0x45];
    if (px > 63 || py > 63)
        return; // p45: presets end at 63, never wrap
    unsigned const x0 = (ext[0x40] << 8) | ext[0x41];
    unsigned const y0 = (ext[0x42] << 8) | ext[0x43];
    // The cursor base uses eight-byte units. Each 64-pixel row occupies
    // eight bytes in each of two planes (Oak databook pp. 44-46).
    uint32_t const address = ((ext[0x47] << 16) | (ext[0x48] << 8) | ext[0x49]) << 3;
    ss_video_rect_t area{int(x0), int(x0 + 63 - px), int(y0), int(y0 + 63 - py)};
    area = area.intersect(clip);
    area = area.intersect(surface.bounds());
    // p46 bit1 permits the cursor over overscan, but never sync/blanking.
    if (!bit(control, 1))
        area = area.intersect(timing.visible());
    for (int y = area.top; y <= area.bottom; ++y)
        for (int x = area.left; x <= area.right; ++x) {
            if (blanked(x, y))
                continue;
            unsigned const sx = x - x0 + px, sy = y - y0 + py;
            // Oak SetCursor uses the first eight bytes of each row for plane A
            // and the next eight for B. Its per-byte nibble swap reorders pixels,
            // never planes. Bit 2 selects the bit order within each byte
            // (Oak databook p. 46). The code table states this model's
            // interpretation of the two plane bits.
            uint8_t const a = linear_byte(address + sy * 16 + sx / 8);
            uint8_t const b = linear_byte(address + sy * 16 + 8 + sx / 8);
            unsigned const lane = bit(control, 2) ? 7 - (sx & 7) : (sx & 7);
            switch (cursor_codes[(bit(a, lane) << 1) | bit(b, lane)]) {
                // Model assumption: color registers select DAC entries in all modes;
                // inversion complements displayed RGB. No true-color cursor bus
                // description is available to establish a different interpretation.
                case CursorOp::color0:
                    surface.pixel(y, x) = ramdac.pixel_select(ext[0x4a]);
                    break;
                case CursorOp::color1:
                    surface.pixel(y, x) = ramdac.pixel_select(ext[0x4b]);
                    break;
                case CursorOp::transparent:
                    break;
                case CursorOp::invert:
                    surface.pixel(y, x) ^= 0x00ffffff;
                    break;
            }
        }
}

// Clip the requested surface and dispatch the active VGA mode; blanking and
// the hardware cursor are applied to the same visible region.
void OTI087::render(ss_video_buffer_t surface, ss_video_rect_t clip) {
    auto bounds = clip.intersect(surface.bounds());
    if (bounds.empty())
        return;
    if (!enabled() || !crtc.sync_en || (sequencer.data[1] & 0x20) || !(attribute.index & 0x20)) {
        surface.fill(0xff000000, bounds);
        return;
    }
    auto active = bounds.intersect(timing.visible());
    bool const overscan = active.width() != bounds.width() || active.height() != bounds.height();
    if (overscan)
        surface.fill(0xff000000, bounds);
    if (!active.empty()) {
        if (packed())
            draw_packed(surface, active);
        else if (!gc.alpha_dis)
            draw_text(surface, active);
        else if (gc.shift256)
            draw_vga256(surface, active);
        else if (!gc.shift_reg)
            draw_planar(surface, active);
        else
            draw_cga(surface, active);
    }
    if (overscan)
        for (int y = bounds.top; y <= bounds.bottom; y++)
            for (int x = bounds.left; x <= bounds.right; x++)
                if (!active.contains(x, y) && !blanked(x, y))
                    surface.pixel(y, x) = ramdac.pixel_select(attribute.data[0x11]);
    draw_cursor(surface, bounds);
}

// Render only packed rows whose VRAM or palette dependency changed.
// Start-address latch changes split the frame into independently addressed spans.
bool OTI087::render_packed_frame(ss_video_buffer_t surface) {
    unsigned const bytes = ramdac.bytes_per_pixel();
    if (!packed() || bytes == 4 || !enabled() || !crtc.sync_en || (sequencer.data[1] & 0x20) ||
        !(attribute.index & 0x20) || (ext[0x4c] & 1) || surface.width != timing.width ||
        surface.height != timing.height || surface.width <= 0 || surface.height <= 0)
        return false;
    // Row reuse needs ordered start-address changes beginning at line zero.
    // Reject ambiguous spans so the general renderer handles them instead.
    if (!starts.empty()) {
        if (starts.front().line != 0)
            return false;
        for (unsigned i = 1; i < starts.size(); ++i)
            if (starts[i].line <= starts[i - 1].line)
                return false;
    }
    std::array<uint32_t, 256> palette{};
    if (bytes == 1)
        for (unsigned i = 0; i < palette.size(); ++i)
            palette[i] = ramdac.pixel_select(i);
    // Equal VRAM bytes are not enough: geometry, packing, sleep and palette
    // can change their visible colors. Any such change invalidates every row.
    bool const redraw = !packed_valid || packed_width != surface.width || packed_height != surface.height ||
                        packed_command != ramdac.command_r() || packed_asleep != ramdac.asleep() ||
                        packed_palette != palette;
    packed_width = surface.width;
    packed_height = surface.height;
    packed_command = ramdac.command_r();
    packed_asleep = ramdac.asleep();
    packed_palette = palette;
    unsigned const row_bytes = surface.width * bytes;
    packed_row.resize(row_bytes);
    packed_source.resize(row_bytes * surface.height);
    frame_pixels.resize(surface.width * surface.height);
    unsigned const repeat = crtc.maximum_scan_line * (crtc.scan_doubling + 1);
    unsigned const interlace = get_interlace_mode() ? 2 : 1;
    unsigned change = 0;
    for (int y = 0; y < surface.height; ++y) {
        while (change + 1 < starts.size() && starts[change + 1].line <= y)
            ++change;
        uint32_t const start = (starts.empty() ? crtc.start_addr : starts[change].address) * 8;
        unsigned const field_line = y / interlace;
        bool const split = field_line > crtc.line_compare;
        unsigned const row = (split ? field_line - crtc.line_compare - 1 : field_line + crtc.preset_row_scan) / repeat;
        uint32_t const address = (split ? 0 : start) + (row * interlace + y % interlace) * crtc.offset * 16;
        packed_bytes(packed_row.data(), memory.data(), address, row_bytes);
        uint8_t *previous = packed_source.data() + y * row_bytes;
        uint32_t *pixels = frame_pixels.data() + y * surface.width;
    // Compare the displayed source bytes, after current address translation,
    // so a page flip or stride change is detected without per-write dirty flags.
        if (redraw || std::memcmp(previous, packed_row.data(), row_bytes)) {
            std::memcpy(previous, packed_row.data(), row_bytes);
            packed_colors(pixels, packed_row.data(), surface.width, ramdac, palette.data());
            if (present_bottom < present_top)
                present_top = y;
            else
                present_top = std::min(present_top, y);
            present_bottom = std::max(present_bottom, y);
        }
        if (&surface.pixel(y, 0) != pixels)
            std::memcpy(&surface.pixel(y, 0), pixels, surface.width * sizeof(uint32_t));
    }
    packed_valid = true;
    return true;
}

// Draw a full frame into surface, replaying retained display-address/pan
// boundaries. Restore the live register values after rendering those spans;
// presenting a frame must not change what the emulated CPU reads.
void OTI087::render_frame(ss_video_buffer_t surface) {
    if (render_packed_frame(surface))
        return;
    packed_valid = false;
    present_top = 0;
    present_bottom = surface.height - 1;
    uint32_t address = crtc.start_addr;
    uint8_t pan = attribute.pel_shift;
    if (starts.empty())
        render(surface, surface.bounds());
    for (unsigned i = 0; i < starts.size(); i++) {
        int end = i + 1 < starts.size() ? starts[i + 1].line - 1 : surface.height - 1;
        crtc.start_addr = starts[i].address;
        attribute.pel_shift = starts[i].pan;
        if (end >= starts[i].line)
            render(surface, {0, surface.width - 1, starts[i].line, end});
    }
    crtc.start_addr = address;
    attribute.pel_shift = pan;
}

// Expose scanout geometry and palette to presentation without changing registers.
ss_video_frame_t OTI087::display_state() const {
    ss_video_frame_t state{};
    state.mode = packed()        ? ss_video_mode_t::Packed
                 : !gc.alpha_dis ? ss_video_mode_t::Text
                 : gc.shift256   ? ss_video_mode_t::VGA256
                 : gc.shift_reg  ? ss_video_mode_t::CGA
                                 : ss_video_mode_t::Planar;
    state.width = timing.width;
    state.height = timing.height;
    state.pitch = packed() ? crtc.offset * 16 : offset();
    state.start = crtc.start_addr * (packed() ? 8 : 1);
    state.depth = packed() ? ramdac.bytes_per_pixel() * 8 : gc.shift256 ? 8 : 4;
    if (packed() && (ramdac.command_r() & 0xd0) == 0x80)
        state.depth = 15;
    for (unsigned i = 0; i < 256; i++) {
        VideoColor color = ramdac.pixel_select(i);
        state.palette[i][0] = color.r();
        state.palette[i][1] = color.g();
        state.palette[i][2] = color.b();
    }
    return state;
}

// Interpret CGA-compatible shift/address modes through the VGA palette path.
void OTI087::draw_cga(ss_video_buffer_t &surface, const ss_video_rect_t &clip) {
    // Generic VGA shift-register path retained from pc_vga, including its
    // dense plane-zero addressing and odd/even 8K scanline banks.
    unsigned height = crtc.scan_doubling + 1;
    unsigned width = (crtc.horz_disp_end + 1) * 8;
    for (int y = clip.top; y <= clip.bottom; y++) {
        unsigned row = y / height;
        uint32_t address = (row & 1) * 0x2000 + (row >> 1) * width / 4;
        for (int x = clip.left; x <= clip.right; x++) {
            uint8_t color = (memory[(address + x / 4) & 0xfffff] >> (6 - (x & 3) * 2)) & 3;
            // The generic VGA CGA fallback selects its pens directly; unlike
            // Oak's planar path it does not apply the color-plane enable mask.
            uint8_t palette = attribute.data[color];
            palette = (attribute.data[0x10] & 0x80) ? (palette & 15) | ((attribute.data[0x14] & 15) << 4)
                                                    : (palette & 63) | ((attribute.data[0x14] & 12) << 4);
            surface.pixel(y, x) = ramdac.pixel_select(palette);
        }
    }
}
