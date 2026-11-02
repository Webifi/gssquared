#pragma once

// Apple IIgs $C03C volume stage, gain relative to nibble 15. The DOC mix and
// the 1-bit speaker are summed, then pass through an MC3340 attenuator whose
// control pin is a binary resistor ladder on the Sound GLU volume outputs:
// 75k, 36k, 18k and 9.1k, with 27k to ground (Apple IIgs ROM 01 and ROM 03
// schematics, part 8). A set volume bit grounds its resistor and a clear bit
// drives it to 5 V, so nibble 15 is loudest. Each value is that ladder's pin
// voltage on the Motorola MC3340 data sheet Figs. 3 and 4 at the board's 9.1 V
// zener rail, about +-3 dB per step. Nibble 0 is -72.4 dB, not silence;
// Apple's Cortland Sound ERS gives the stage about a 90 dB range.
inline constexpr float k_c03c_gain[16] = {
    0.000239883f, 0.00042658f, 0.000776247f, 0.00134896f, // -72.4 -67.4 -62.2 -57.4 dB
    0.00266073f, 0.00489779f, 0.0103514f, 0.0194984f,     // -51.5 -46.2 -39.7 -34.2 dB
    0.0384592f, 0.0785236f, 0.18197f, 0.312608f,          // -28.3 -22.1 -14.8 -10.1 dB
    0.44157f, 0.60256f, 0.785236f, 1.0f                   //  -7.1  -4.4  -2.1   0.0 dB
};
