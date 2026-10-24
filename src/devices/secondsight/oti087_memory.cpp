/*
 *   Memory access for Second Sight's Oak OTI-087X video controller.
 *   SSCard supplies a physical address after the board selects its video-memory
 *   window. This file chooses an Oak bank, reads or writes video RAM (VRAM),
 *   and tells the board when the chip is ready for another access.
 *
 *   Video Graphics Array (VGA) planar mode stores a pixel's bits in four
 *   separate byte arrays, called planes. An aperture is a CPU address window.
 *   A read loads latches: retained bytes used by later masked writes. Chain-4
 *   uses the low two CPU address bits to select a plane. Oak packed mode instead
 *   presents consecutive pixel bytes, with an eight-byte latch and pixel mask.
 *   Both views share storage so a mode change does not move the underlying data.
 *
 *   OTI087 owns this state. SSCard's execution thread calls these methods; a
 *   host debugger must synchronize first. Sources: Oak OTI-087 February 1993
 *   databook pp. 35, 42-44, with its OTI-087X addendum; IBM VGA/XGA Technical
 *   Reference May 1992 pp. 2-38, 2-54 and 2-72 to 2-75. Inferred address rules
 *   are marked below; they are not claims about undocumented chip wiring.
 */

#include "oti087.hpp"
#include <algorithm>
#include <iterator>

namespace {
// Extract bit n as zero or one when decoding a documented register field.
constexpr unsigned bit(uint32_t value, unsigned n) {
    return (value >> n) & 1;
}
} // namespace

// VGA enable and Oak disable bits gate memory selection as well as scanout.
bool OTI087::enabled() const {
    return bit(enable, 0) && bit(setup, 3) && !bit(setup, 4);
}

// Select the VGA aperture or Oak linear window and apply the read/write bank.
// Oak OTI-087X databook p. 35 describes linear-window base-zero compatibility.
bool OTI087::decode_memory(uint32_t address, bool write, uint32_t &offset) const {
    if (!enabled() || !bit(miscellaneous_output, 1))
        return false;
    if (bit(ext[5], 0) && (ext[5] & 0xf0)) {
        uint32_t const size = 0x40000U << ((ext[5] >> 2) & 3);
        uint32_t const base = (uint32_t(ext[5] & 0xf0) << 16) & ~(size - 1);
        if (address < base || address - base >= size)
            return false;
        offset = address - base;
        return true;
    }
    // OTI-087X data book p. 35: base zero retains the VGA aperture, including when bit0 is set.
    // Graphics Miscellaneous bits 3:2 select the CPU window. The four entries
    // are 128 KiB at A0000, 64 KiB at A0000, 32 KiB at B0000, 32 KiB at B8000.
    static constexpr uint32_t bases[] = {0xa0000, 0xa0000, 0xb0000, 0xb8000};
    static constexpr uint32_t sizes[] = {0x20000, 0x10000, 0x8000, 0x8000};
    unsigned const map = gc.memory_map_sel & 3;
    if (address < bases[map] || address - bases[map] >= sizes[map])
        return false;
    offset = address - bases[map];
    if (address < 0xb0000)
        offset += uint32_t(ext[write ? 0x24 : 0x23]) << 16;
    return true;
}

// Map sequential display bytes to the interleaved four-plane VRAM storage.
uint32_t OTI087::storage_address(uint32_t linear) const {
    // Internal representation only: retain the generic VGA core's four
    // 64K plane arrays per 256K block. All packed and linear paths translate
    // through the same mapping, so changing modes never copies/loses VRAM.
    return (linear & 0xc0000) | ((linear & 3) << 16) | ((linear >> 2) & 0xffff);
}

// Read one display-byte address through the common plane-storage mapping.
uint8_t OTI087::linear_byte(uint32_t address) const {
    return memory[storage_address(address)];
}

// Store data at one display-byte address; mode changes keep the same storage.
void OTI087::linear_byte_w(uint32_t address, uint8_t data) {
    memory[storage_address(address)] = data;
}

// Choose linear, packed or VGA latch reads after aperture decode; unselected
// addresses leave the card data bus to its caller.
uint8_t OTI087::bus_memory_r(uint32_t address) {
    uint32_t offset;
    if (!decode_memory(address, false, offset))
        return 0xff;
    access_begin(false);
    if (!packed())
        return planar_r(offset);
    if (side_effects) {
        if (bit(ext[0x22], 4) && bit(sequencer.data[4], 3)) {
            // Inferred from ROM_BEEF.asm sub_AE92 super-high-resolution (SHR)
            // lookup stores: an 8-bit chain-4 read
            // loads the addressed four-byte word into both halves of the wide latch.
            for (unsigned i = 0; i != 4; ++i)
                latch[i] = latch[4 + i] = gc.latch[i] = linear_byte((offset & ~3) + i);
        } else {
            // Packed reads without chain-4 use an aligned eight-byte latch.
            // This layout is a model convention, not specified by the data book.
            for (unsigned i = 0; i != 8; ++i)
                latch[i] = linear_byte((offset & ~7) + i);
        }
    }
    return linear_byte(offset);
}

// Choose packed or planar writes after bank/aperture decode so CPU and DMA
// (direct memory access) copies share the same VGA write-mode effects.
void OTI087::bus_memory_w(uint32_t address, uint8_t data) {
    uint32_t offset;
    if (!decode_memory(address, true, offset))
        return;
    access_begin(true);
    if (packed())
        packed_w(offset, data);
    else
        planar_w(offset, data);
}

// Use the address low bits to select a VGA plane while preserving Oak wide
// latch layout (IBM VGA/XGA Technical Reference, Sequencer Memory Mode).
uint32_t OTI087::chain4_address(uint32_t address) const {
    // Keep Oak packed/wide-latch accesses on their existing byte layout.
    if (packed())
        return address >> 2;
    // Inferred from IBM VGA/XGA TRM pp. 2-38, 2-54, 2-72, 2-74, 2-75:
    // CH4 selects map A1:A0; within the 64K VGA aperture the map address is
    // (A & FFFC) | (A >> 14). CPU A15:A14 replace A1:A0, matching the CRTC
    // doubleword mux's recirculated MA13:MA12. IBM does not specify this
    // CPU-side map address explicitly. Retain extended bank bits above A15.
    return (address & ~uint32_t(3)) | ((address >> 14) & 3);
}

// Load all four VGA latches, then apply read-map selection or color compare.
// IBM VGA/XGA Technical Reference, Graphics Mode and Color Compare registers.
uint8_t OTI087::planar_r(uint32_t address) {
    unsigned plane = gc.read_map_sel & 3;
    if (bit(sequencer.data[4], 3)) {
        plane = address & 3;
        address = chain4_address(address);
    } else if (gc.host_oe) {
        plane = (plane & 2) | (address & 1);
        address >>= 1;
    }
    // Each VGA read samples all four planes, even though read mode 0 returns
    // only the selected plane. Read mode 1 instead compares every cared-about
    // plane to the requested color, yielding a mask of matching pixel bits.
    uint8_t plane_latch[4];
    for (unsigned p = 0; p != 4; ++p)
        plane_latch[p] = linear_byte((address << 2) | p);
    if (side_effects) {
        std::copy(std::begin(plane_latch), std::end(plane_latch), std::begin(gc.latch));
        for (unsigned i = 0; i != 8; ++i)
            latch[i] = linear_byte(((address & ~1) << 2) + i);
    }
    if (!gc.read_mode)
        return plane_latch[plane];
    uint8_t result = 0xff;
    for (unsigned p = 0; p != 4; ++p)
        if (bit(gc.color_dont_care, p))
            result &= plane_latch[p] ^ (bit(gc.color_compare, p) ? 0 : 0xff);
    return result;
}

// Apply rotate, set/reset, logical operation and bit mask to selected planes.
// IBM VGA/XGA Technical Reference, Graphics Controller write modes 0-3.
void OTI087::planar_w(uint32_t address, uint8_t data) {
    // OTI-087X data book index22: with bit4 clear, bit0 selects the latch
    // byte through CPU A2:A0, as index35 does, for write mode 1.
    bool const addressed_latch = (ext[0x22] & 0x11) == 1 && gc.write_mode == 1;
    uint8_t const latch_byte = latch[address & 7];
    // Map-mask bits enable plane stores. Chain-4 chooses one of four planes;
    // odd/even mode chooses the two planes associated with the byte parity.
    uint8_t mask = sequencer.map_mask;
    if (bit(sequencer.data[4], 3)) {
        mask &= 1 << (address & 3);
        address = chain4_address(address);
    } else if (!bit(sequencer.data[4], 2)) {
        mask &= (address & 1) ? 10 : 5;
        address >>= 1;
    }
    if (bit(ext[0x30], 3))
        mask = ext[0x34] >> ((address & 1) ? 0 : 4);
    for (unsigned p = 0; p != 4; ++p) {
        if (!bit(mask, p))
            continue;
        uint8_t value = addressed_latch ? latch_byte : vga_latch_write(p, data);
        if ((ext[0x30] & 3) == 3) {
            uint8_t const pattern = bit(ext[0x30], 2) ? ext[0x33] : data;
            value = (bit(ext[0x31], p) ? pattern : 0) | (bit(ext[0x32], p) ? ~pattern : 0);
        }
        linear_byte_w((address << 2) | p, value);
    }
}

// Apply Oak wide-latch and pixel-mask writes without the planar VGA address
// mask when pixel masking is enabled (OTI-087X databook pp. 43-44).
void OTI087::packed_w(uint32_t address, uint8_t data) {
    bool const pixel_mask = bit(ext[0x30], 3);
    bool const expand = (ext[0x30] & 3) == 1;
    bool const wide_chain4 = bit(ext[0x22], 4) && bit(sequencer.data[4], 3);
    if (wide_chain4 && !pixel_mask) {
        // ROM1.4 builds its lookup table with mode-0 byte stores while
        // ext22=1c; the wide latch must not change their addressing.
        planar_w(address, data);
        return;
    }
    // Ext22 bit4 widens the VGA graphics latch; it does not select it as
    // the write source. VGA write mode 1 does that. ROM_BEEF sub_AE92 builds
    // the SHR lookup table with ordinary mode-0 byte stores while bit4 is
    // already set, then selects mode1 and ext30 pixel masking for the block copy.
    bool const addressed_latch = (ext[0x22] & 0x11) == 1;
    bool const use_latch = (bit(ext[0x22], 4) || addressed_latch) && gc.write_mode == 1;
    // OTI-087X data book p. 43: pixel masking disables packed-pixel address/map masking.
    // With ordinary masking, a byte store still addresses only one byte.
    // With the wide latch and pixel mask, A0 selects four bytes; A1/A2 do not
    // select an individual byte. This half-selection is a model interpretation of
    // the wide-latch path; ext24's bank is already included in address.
    bool const half_store = wide_chain4 && pixel_mask;
    uint32_t const base = half_store               ? (address & ~7) + 4 * (address & 1)
                          : (expand || pixel_mask) ? (address & ~7)
                                                   : address;
    unsigned const count = half_store ? 4 : (expand || pixel_mask) ? 8 : 1;
    uint8_t const pattern = bit(ext[0x30], 2) ? ext[0x33] : data;
    for (unsigned i = 0; i != count; ++i) {
        unsigned const pos = (base + i) & 7;
        if (pixel_mask) {
            // OTI-087X data book pp. 43-44: bit4 reverses pixel-mask order.
            if (!bit(ext[0x34], bit(ext[0x30], 4) ? 7 - pos : pos))
                continue;
        } else if (!bit(sequencer.map_mask, (base + i) & 3))
            continue;
        uint8_t value = expand ? ext[bit(pattern, 7 - pos) ? 0x31 : 0x32] : data;
        if (use_latch)
            value = latch[addressed_latch ? address & 7 : pos ^ (bit(ext[0x22], 5) ? 4 : 0)];
        linear_byte_w(base + i, value);
    }
}
