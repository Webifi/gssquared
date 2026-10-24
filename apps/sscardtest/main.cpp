/*
 *   Second Sight reset and component-lifetime check. It creates a card with a
 *   small in-memory ROM, feeds an Apple text-memory write, and compares warm
 *   reset with replacing the firmware and U7 Lattice capture chip equations.
 *   The main thread waits before inspecting worker-owned card state. Assertions
 *   produce the result; main returns zero on success. Source: ROM_BEEF.asm
 *   Reset at https://github.com/jawaidbazyar2/secondsight ; keeping the display
 *   and electrically erasable configuration memory (EEPROM) is an application
 *   lifetime contract, not a CPU instruction rule.
 */

// Replacing ROM/U7 Lattice chip at a restart keeps the live display and EEPROM objects.
#include "devices/secondsight/ss_card.hpp"
#include <cassert>
#include <cstdio>

// Run the independent fixtures below; assertions fail at the violated contract.
// No arguments are used. A normal return means every check completed.
int main() {
    std::array<uint8_t, SSCard::ROM_SIZE> rom{};
    SSCard card(rom.data(), 14318180);
    card.start_worker();
    // Borrow the stopped video object, then seed one EEPROM word to detect
    // accidental replacement when only firmware/capture hardware changes.
    auto *display = &card.video();
    auto words = display->serial_eeprom().contents();
    words[7] = 0x1234;
    display->serial_eeprom().set_contents(words);
    // Capture one Apple text byte, then drain the worker before warm reset.
    megaii_transfer_t t{40, 0x400, 0xa5, false};
    card.observe(t);
    card.catch_up(100);
    card.reset(100); // Warm reset preserves the installed map and copied SRAM.
    assert(card.capture_state().lattice_map == SSCapture::LatticeMap::BE8C);
    assert(card.sram_data()[0x400] == 0xa5);
    // HALT ($76) identifies the replacement image. A power cycle clears
    // SRAM and execution totals but must preserve display identity and EEPROM.
    rom[0] = 0x76;
    card.change_hardware(100, rom.data(), SSCapture::LatticeMap::V2);
    assert(card.memory_read(0) == 0x76 && card.processor().cycles() == 0);
    assert(card.capture_state().lattice_map == SSCapture::LatticeMap::V2);
    assert(card.sram_data()[0x400] == 0 && card.copies == 0);
    assert(&card.video() == display && display->serial_eeprom().contents() == words);
    card.reset(100);
    assert(card.capture_state().lattice_map == SSCapture::LatticeMap::V2 && card.memory_read(0) == 0x76);
    std::puts("PASS worker hardware replacement, warm-reset retention, SRAM power reset, stable display and EEPROM");
}
