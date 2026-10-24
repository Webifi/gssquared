/*
 *   Present the Second Sight monitor using pixels from its Oak OTI-087X
 *   video controller. The card has its own screen, even when the Apple IIgs
 *   built-in display shows something different. SSCard first catches up to the
 *   host frame; video_system_t then calls this adapter to upload and draw it.
 *
 *   SDL is the host window/graphics library. The application owns this
 *   adapter and all SDL windows/textures. Every
 *   method runs on the main thread after card execution has synchronized.
 *   Native builds can use two windows; browser builds place both monitors on
 *   one canvas. Input from either monitor feeds the same emulated Apple.
 *   Source: Second Sight manual, monitor installation; Oak OTI-087 databook
 *   pp. 39-40 describes the screen timing extensions. Window layout and input mapping
 *   are application choices, not card registers.
 */

#pragma once

#include <SDL3/SDL.h>

struct computer_t;
class OTI087;

// Host-side monitor/window adapter; consumes Oak pixels without owning
// Z180 time or changing video registers.
class SecondSightDisplay {
public:
    // Borrow computer and video; install main-thread render and input callbacks.
    SecondSightDisplay(computer_t *computer, OTI087 &video);
    // Detach callbacks, then release the owned textures and secondary window.
    ~SecondSightDisplay();

private:
    // One main-thread SDL texture plus its allocated pixel dimensions. Each
    // renderer needs its own copy because SDL textures cannot cross renderers.
    struct Texture {
        SDL_Texture *value = nullptr;
        int width = 0, height = 0;
    };
    computer_t *computer;
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

    // Draw the selected monitor layout from the synchronized card frame.
    void render();
    // Route one SDL event; return true if consumed or handled for the card window.
    bool handle_event(const SDL_Event &event);
    // Update texture for dest; return it or null if geometry/upload/allocation fails.
    SDL_Texture *prepare_texture(SDL_Renderer *dest, Texture &texture);
    // Lazily open the second monitor; return false on failure or in a browser.
    bool open_window();
};
