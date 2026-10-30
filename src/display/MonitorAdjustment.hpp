/*
 * Apply the host monitor size and position controls to a sampled image window.
 * SystemSettings stores one record per monitor. The Apple and Second Sight
 * presenters read it on the main thread before fitting that window on screen.
 * These are display preferences, not emulated monitor registers or timing.
 */
#pragma once

#include <array>

// Host monitor knobs, expressed as percent of the calibrated visible window.
// Zero leaves the calibrated crop untouched. Positive size enlarges the picture;
// positive position moves it right/down by moving the sampled window left/up.
// These values never change the emulated raster or its timing.
struct MonitorAdjustment {
    // Horizontal/vertical size, then horizontal/vertical position; COUNT is
    // the number of stored controls. Settings clamp each step to -20..20.
    enum Control { H_SIZE, V_SIZE, H_POSITION, V_POSITION, COUNT };
    std::array<int, COUNT> steps{};

    // Adjust a caller-owned source rectangle in its existing pixel/line units.
    // Shrink the sampled window to enlarge the picture, keeping its center
    // fixed before applying the requested position shift.
    void apply(float &x, float &y, float &width, float &height) const {
        if (steps == std::array<int, COUNT>{}) return;
        const float w = width * 100.0f / (100 + steps[H_SIZE]);
        const float h = height * 100.0f / (100 + steps[V_SIZE]);
        x += (width - w) * 0.5f - w * steps[H_POSITION] / 100.0f;
        y += (height - h) * 0.5f - h * steps[V_POSITION] / 100.0f;
        width = w;
        height = h;
    }
};
