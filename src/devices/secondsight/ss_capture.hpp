/*
 *   Second Sight's U7 Lattice ispLSI1016 chip watches the Apple bus. This
 *   model decodes each access into a video-memory copy or a firmware event.
 *   Capture means retaining the access's address/data; an event means firmware
 *   must interpret a soft-switch change or host command. A soft switch is an
 *   Apple address whose read or write changes a hardware setting.
 *
 *   SSCard calls transfer() to decide what the access needs, waits if needed,
 *   then calls complete() to load the U2 Apple data-in latch. A latch holds a
 *   byte until another load. The U17 copy controller writes captured bytes
 *   to card RAM; the firmware reads event addresses through Z180 ports.
 *
 *   Mega II supplies the IIgs slow Apple II bus. /M2SEL is its active-low
 *   select signal; M2B0 is the CPU bank's low bit during the address phase.
 *   RDY (Ready) holds the Apple CPU until the previous event is acknowledged.
 *   Sources: Apple IIgs Technical Note 68, Mega II Select/Bank 0 sections;
 *   Second Sight manual and Jawaid Bazyar's ROM_BEEF.asm Reset and event
 *   handlers at https://github.com/jawaidbazyar2/secondsight . The selectable
 *   BE8C/BBB1 capture equations model card logic; the manuals describe the
 *   signals and firmware interface, not every equation or internal delay.
 */

#pragma once

#include <cstdint>
#include "mmus/megaii_bus.hpp"

using ss_mega_transfer_t = megaii_transfer_t;

// One decode result, returned by value to SSCard on the card execution thread.
struct ss_capture_result_t {
    bool copy = false;  // Copy this write into the card's Apple video-memory image.
    bool bank = false;  // Destination: false = main 64 KiB, true = auxiliary 64 KiB.
    bool event = false; // Save the low address byte for firmware to interpret.
    bool drive = false; // The U3 Apple data-out latch supplies this Apple read.
    bool stall = false; // An older event must be acknowledged before this access.
};

// Owned by SSCard. Only the card execution thread mutates it; host inspection
// must wait for the worker. No IIgs RAM pointer is retained here.
class SSCapture {
public:
    // BE8C selects the shipped equations; V2 selects the BBB1 revised equations.
    enum class LatticeMap { BE8C, V2 };
    LatticeMap lattice_map;
    // State of the Lattice capture equations, independent of CPU scheduling.
    // SSCard owns event acknowledgement and the subsequent SRAM copy.
    explicit SSCapture(LatticeMap map = LatticeMap::BE8C) : lattice_map(map) {}

    // Port $A1 D3 enables IIgs bank/select handling; D2 inhibits video copies.
    // bank_held retains M2B0 through the later data phase. HIRES selects Apple
    // high-resolution graphics; PAGE2 selects the second display page; RAMWRT
    // directs ordinary writes to auxiliary RAM; 80STORE changes how PAGE2
    // selects the text/graphics bank (Apple IIgs Hardware Reference, memory map).
    bool mode_d3 = false;
    bool copy_inhibit = false;
    bool bank_held = false;
    bool hires = false;
    bool page2 = false;
    bool ramwrt = false;
    bool store80 = false;
    bool pending = false;
    // Bits 0-7 track writes to $2000-$9FFF in 4 KiB bands, including super-high-resolution (SHR) control/palette bytes.
    uint8_t bands = 0;
    uint8_t event_address = 0;
    uint8_t host_input = 0;

    // Clear volatile latches at power-on while retaining the selected equations.
    void power_on() { *this = SSCapture(lattice_map); }
    // Latch capture inhibit and IIgs bank-selection mode from Z180 port $A1.
    void configure(uint8_t data) {
        copy_inhibit = data & 4;
        mode_d3 = data & 8;
        if (!mode_d3)
            bank_held = false;
    }

    // Decode t's address, read/write direction and bank pins. Return actions
    // for SSCard without consuming the pending event. This also updates the
    // chip's remembered soft switches and dirty-band bits. BE8C preserves
    // its bank-selection behavior and does not capture border-color writes.
    ss_capture_result_t transfer(const ss_mega_transfer_t &t) {
        // Retain the bank signal while /M2SEL is low. The 7M clock is the
        // Apple 7 MHz clock; its edges close the model's bank sampling window.
        // M2B0 later stops driving, so sampling it in the data phase would
        // lose auxiliary-bank writes (Apple IIgs Technical Note 68, Fig. 1).
        bank_held = mode_d3 && t.m2b0 && (bank_held || !t.m2sel_n);
        const uint16_t a = t.address;
        const bool valid = !mode_d3 || !t.m2sel_n;
        const bool c0 = (a & 0xff00) == 0xc000 && valid;
        ss_capture_result_t result;
        // Text pages occupy $0400-$0BFF; high-resolution pages occupy
        // $2000-$5FFF. IIgs super-high-resolution (SHR) pixels, scan-line
        // controls and palettes extend the auxiliary-bank range to $9FFF.
        // These are bus address tests, not reads of the emulator's RAM.
        result.copy = !t.read && !copy_inhibit && valid &&
                      ((a >= 0x0400 && a < 0x0c00) || (a >= 0x2000 && a < 0x6000) ||
                       (a >= 0x2000 && a < 0xa000 && mode_d3 &&
                        (lattice_map == LatticeMap::V2 ? (bank_held || ramwrt) : (bank_held != ramwrt))));
        // $C050-$C057 select graphics/text, mixed mode, page and resolution;
        // $C05E/$C05F select double resolution. $C00C-$C00F control 80-column
        // text/font selection, $C07E/$C07F the IOUDIS switch (I/O-unit disable), and
        // $C0B0-$C0B7 send card commands. IIgs writes $C022/$C029/$C02B
        // change text colors, video mode and monochrome control. V2 also
        // observes $C034 border color. Reads of write-only switches do not act.
        result.event =
            c0 && ((a >= 0xc050 && a <= 0xc053) || (a >= 0xc054 && a <= 0xc055 && !store80) ||
                   (a >= 0xc056 && a <= 0xc057) || (a >= 0xc05e && a <= 0xc05f) || (a >= 0xc0b0 && a <= 0xc0b7) ||
                   (!t.read &&
                    ((a >= 0xc00c && a <= 0xc00f) || a == 0xc07e || a == 0xc07f ||
                     (mode_d3 && (a == 0xc022 || a == 0xc029 || a == 0xc02b || (lattice_map == LatticeMap::V2 && a == 0xc034))))));
        result.drive = c0 && t.read && (a & 0xf0) == 0xb0;
        result.stall = pending && (result.copy || result.event);
        // Track switch values from the bus before choosing the destination
        // bank. $C068 is STATEREG, a byte containing several switch states.
        // It can change PAGE2 and RAMWRT together (Apple IIgs Tech Note 68).
        if (c0) {
            if (a == 0xc056 || a == 0xc057)
                hires = a & 1;
            if (a == 0xc054 || a == 0xc055)
                page2 = a & 1;
            if (!t.read && (a == 0xc004 || a == 0xc005))
                ramwrt = a & 1;
            if (!t.read && (a == 0xc000 || a == 0xc001))
                store80 = a & 1;
            if (a == 0xc068) {
                page2 = t.data & 0x40;
                ramwrt = t.data & 0x10;
            }
        }
        if (lattice_map == LatticeMap::V2) {
            // 80STORE selects PAGE2 only for text page 1
            // and (with HIRES on) hires page 1. All other copies use RAMWRT.
            const bool page1 = (a >= 0x0400 && a < 0x0800) || (hires && a >= 0x2000 && a < 0x4000);
            result.bank = bank_held || (store80 && page1 ? page2 : ramwrt);
        } else {
            result.bank = (bank_held && mode_d3 && !ramwrt) || (!bank_held && page2 && store80) ||
                          (!bank_held && ramwrt && !store80) || (!bank_held && mode_d3 && ramwrt && !hires);
        }
        if (result.copy)
            mark_band(a);
        return result;
    }

    // Accept t after any RDY hold, using the decode result from transfer().
    // Load the U2 Apple data-in latch only for copied writes or event writes.
    // Event reads retain its byte. Set pending once; held bus phases must
    // not overwrite the byte that firmware has yet to acknowledge.
    void complete(const ss_mega_transfer_t &t, const ss_capture_result_t &result) {
        if (result.copy || (result.event && !t.read))
            host_input = t.data;
        if (result.copy)
            mark_band(t.address);
        if (result.event) {
            event_address = t.address;
            pending = true;
        }
    }

    // Release the pending host event when the Z180 selects its acknowledge port.
    void acknowledge() { pending = false; }
    // Read status at port $A4-$A7 (only its low three bits are used).
    // A nibble is four bits. $A4/$A5 return and clear low/high dirty-band
    // nibbles; $A6/$A7 return low/high event-address nibbles. The selector
    // drives only four pins, so keep the previous bus byte's upper nibble.
    // See ROM_BEEF.asm event dispatch reads of ports $A4-$A7.
    uint8_t status(unsigned port, uint8_t bus) {
        uint8_t nibble = 0;
        switch (port & 7) {
        case 4:
            nibble = bands & 15;
            bands &= 0xf0;
            break;
        case 5:
            nibble = bands >> 4;
            bands &= 0x0f;
            break;
        case 6:
            nibble = event_address & 15;
            break;
        case 7:
            nibble = event_address >> 4;
            break;
        }
        return (bus & 0xf0) | nibble;
    }

private:
    // Set the 4 KiB dirty-band bit for copied writes from $2000 through $9FFF;
    // SHR palette and scan-line bytes are tracked with their containing band.
    void mark_band(uint16_t address) {
        if (address >= 0x2000 && address < 0xa000)
            bands |= 1 << ((address - 0x2000) >> 12);
    }
};
