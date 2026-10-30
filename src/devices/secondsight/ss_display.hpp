/*
 *   Present the Second Sight monitor using pixels from its Oak OTI-087X
 *   video controller. The card has its own screen, even when the Apple IIgs
 *   built-in display shows something different. SSCard offers a stopped scan;
 *   video_system_t then calls this adapter to upload and draw it.
 *
 *   SDL is the host window/graphics library. The application owns this
 *   adapter and all SDL windows/textures. All methods run on the main thread;
 *   render owns stopped Oak state before reading it, or reuses its last texture.
 *   Native builds can use two windows; browser builds place both monitors on
 *   one canvas. Input from either monitor feeds the same emulated Apple.
 *   Source: Second Sight manual, monitor installation; Oak OTI-087 databook
 *   pp. 39-40 describes the screen timing extensions. Window layout and input mapping
 *   are application choices, not card registers.
 */

#pragma once

#include <SDL3/SDL.h>
#include "oti087.hpp"
#include "ss_crt.hpp" // Host monitor scan, shared by native and web.

struct computer_t;
class SSCard;

// Host-side monitor/window adapter; consumes Oak pixels without owning
// Z180 time or changing video registers.
class SecondSightDisplay {
public:
    // Borrow computer and card; install main-thread render and input callbacks.
    SecondSightDisplay(computer_t *computer, SSCard &card);
    // Detach callbacks, then release the owned textures and secondary window.
    ~SecondSightDisplay();

    // Last submitted production draw, owned and read on the render thread.
    // This host record is not a card register or a serialized wire structure.
    struct Presentation {
        bool valid = false, checksummed = false;
        uint64_t sequence = 0, host_ticks = 0;
        SDL_FRect viewport = {}, destination = {};
        ss_video_timing_t timing;
        uint32_t before_width = 0, before_height = 0, before_bytes = 0, before_crc = 0;
        uint32_t after_width = 0, after_height = 0, after_bytes = 0, after_crc = 0;
    };
    enum Path { MAIN = 0, WINDOW = 1 };
    // Find the borrowed adapter for a live machine; no cross-thread access.
    static SecondSightDisplay *for_computer(computer_t *computer);
    // Return a value snapshot; a currently inactive path is always invalid.
    Presentation presentation(Path path) const;
    // External tests may install a checksum only for one paused presentation.
    // Null by default: no source walk, allocation or CRC cost in normal use.
    uint32_t (*source_checksum)(const uint32_t *, size_t) = nullptr;

    // Match video_system_t::calculate_target_rect: one centered aspect fit for
    // the complete monitor window. Sampling/cropping belongs to the scan, so
    // active video and its overscan can never receive different scale factors.
    static SDL_FRect frame_rect(const SDL_FRect &target, float aspect) {
        float w = target.w, h = w / aspect;
        if (h > target.h) { h = target.h; w = h * aspect; }
        return {target.x + (target.w - w) * 0.5f, target.y + (target.h - h) * 0.5f, w, h};
    }

private:
    // One main-thread SDL texture plus its allocated pixel dimensions. Each
    // renderer needs its own copy because SDL textures cannot cross renderers.
    struct Texture {
        SDL_Texture *value = nullptr;
        int width = 0, height = 0;
        // Retain the other field on the GPU; unchanged interlaced frames then
        // alternate textures without copying or filtering their pixels again.
        SDL_Texture *odd = nullptr;
        SecondSightCrt crt;
        bool filtered = false;
        // These belong to the uploaded texture, so a missed worker handoff
        // can draw it without reading any live Oak or interlace state.
        SDL_Texture *shown = nullptr;
        ss_video_timing_t timing;
    };
    computer_t *computer;
    SSCard &card;
    OTI087 &video;
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Renderer *last_renderer = nullptr;
    Texture main_texture, window_texture;
    SDL_FRect card_content = {};
    bool card_mouse_down = false;
    bool mouse_captured = false;
    bool visible = false;
    bool window_failed = false;

    // Two small retained draw records; pending is private until submission.
    std::array<Presentation, 2> presentations;
    Presentation pending;
    uint64_t presentation_sequence = 0;
    bool missed_frame = false;
    void begin_presentation(const Texture &texture);
    void finish_presentation(Path path, const SDL_FRect &viewport, const SDL_FRect &destination);

    // Draw the selected monitor layout from the synchronized card frame.
    void render();
    // Route one SDL event; return true if consumed or handled for the card window.
    bool handle_event(const SDL_Event &event);
    // Update texture for dest; return it or null if geometry/upload/allocation fails.
    SDL_Texture *prepare_texture(SDL_Renderer *dest, Texture &texture, bool refresh);
    // Lazily open the second monitor; return false on failure or in a browser.
    bool open_window();
};
