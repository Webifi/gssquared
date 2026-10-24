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

#include "ss_display.hpp"
#include "oti087.hpp"
#include "computer.hpp"
#include "videosystem.hpp"
#include "util/SystemSettings.hpp"
#include <algorithm>

// Attach card rendering and input routing to the existing video system.
SecondSightDisplay::SecondSightDisplay(computer_t *computer, OTI087 &video)
    : computer(computer), video(video) {
    computer->video_system->additional_display = [this]() { render(); };
    computer->video_system->additional_display_event = [this](const SDL_Event &event) {
        return handle_event(event);
    };
}

// Detach host callbacks before releasing textures and the optional monitor window.
SecondSightDisplay::~SecondSightDisplay() {
    auto *vs = computer->video_system;
    vs->additional_window = nullptr;
    vs->additional_display = {};
    vs->additional_display_event = {};
    if (vs->last_texture == main_texture.value) vs->last_texture = nullptr;
    if (main_texture.value) SDL_DestroyTexture(main_texture.value);
    if (window_texture.value) SDL_DestroyTexture(window_texture.value);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
}

// Create the native card monitor lazily. Browsers share the main canvas
// because an SDL secondary window is not a separate browser monitor.
bool SecondSightDisplay::open_window() {
#if !defined(__EMSCRIPTEN__)
    // Do not retry a failed window allocation every frame. Keep the machine
    // usable with its existing window and report the first SDL error.
    if (window_failed) return false;
    if (window) return true;
    const auto title = gs2_window_title("GSSquared - Second Sight monitor");
    window = SDL_CreateWindow(title.c_str(), 640, 480,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
    if (window) renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        window_failed = true;
        fprintf(stderr, "Second Sight monitor: %s\n", SDL_GetError());
        if (window) SDL_DestroyWindow(window);
        window = nullptr;
        return false;
    }
    computer->video_system->additional_window = window;
    int x, y, w, h;
    SDL_GetWindowPosition(computer->video_system->window, &x, &y);
    SDL_GetWindowSize(computer->video_system->window, &w, &h);
    SDL_Rect bounds;
    // Place the new monitor beside the Apple window and clamp it to the
    // usable desktop, so its title bar stays reachable on small displays.
    if (SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(computer->video_system->window), &bounds)) {
        x = std::max(bounds.x, std::min(x + w, bounds.x + bounds.w - 640));
        y = std::max(bounds.y, std::min(y, bounds.y + bounds.h - 480));
        SDL_SetWindowPosition(window, x, y);
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    return true;
#else
    return false;
#endif
}

// Recreate the texture when Oak changes geometry; force a full upload when
// switching renderers because each texture has its own retained pixels.
SDL_Texture *SecondSightDisplay::prepare_texture(SDL_Renderer *dest, Texture &texture) {
    // dest owns this texture; its dimensions come from the programmed card
    // raster. Reject incomplete or excessive modes before allocating pixels.
    const auto &timing = video.display_timing();
    if (timing.width < 1 || timing.height < 1 || timing.width > OTI087::MAX_FRAME_DIMENSION || timing.height > OTI087::MAX_FRAME_DIMENSION)
        return nullptr;
    bool full = last_renderer != dest;
    if (!texture.value || texture.width != timing.width || texture.height != timing.height) {
        if (texture.value) SDL_DestroyTexture(texture.value);
        texture.width = timing.width;
        texture.height = timing.height;
        texture.value = SDL_CreateTexture(dest, SDL_PIXELFORMAT_XRGB8888,
            SDL_TEXTUREACCESS_STREAMING, texture.width, texture.height);
        if (!texture.value) return nullptr;
        SDL_SetTextureBlendMode(texture.value, SDL_BLENDMODE_NONE);
        full = true;
    }
    SDL_SetTextureScaleMode(texture.value, computer->video_system->display_pixel_mode == DM_PIXEL_FUZZ
        ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_PIXELART);
    if (!video.upload_frame(texture.value, full)) return nullptr;
    last_renderer = dest;
    return texture.value;
}

// Present the selected monitor(s) without altering card timing. Restore the
// IIgs viewport and draw state after placing the card image.
void SecondSightDisplay::render() {
    auto *vs = computer->video_system;
    const int view = SystemSettings::instance().ss_view();
#if !defined(__EMSCRIPTEN__)
    // A native second window has its own renderer and mouse-capture setting.
    // Preserve the card aspect ratio inside it; black margins are not input pixels.
    if (view == SystemSettings::SS_VIEW_BOTH && open_window()) {
        if (!visible) { vs->show(window); visible = true; }
        if (mouse_captured != vs->mouse_captured) {
            mouse_captured = vs->mouse_captured;
            SDL_SetWindowRelativeMouseMode(window, mouse_captured);
        }
        SDL_RenderClear(renderer);
        if (auto *texture = prepare_texture(renderer, window_texture)) {
            int w, h;
            SDL_GetRenderOutputSize(renderer, &w, &h);
            float scale = std::min(float(w) / window_texture.width, float(h) / window_texture.height);
            SDL_FRect dest = {(w - window_texture.width * scale) / 2,
                (h - window_texture.height * scale) / 2,
                window_texture.width * scale, window_texture.height * scale};
            card_content = dest;
            SDL_RenderTexture(renderer, texture, nullptr, &dest);
        }
        SDL_RenderPresent(renderer);
    } else if (visible) {
        vs->hide(window);
        visible = false;
    }
    if (view != SystemSettings::SS_VIEW_CARD) return;
#else
    if (view == SystemSettings::SS_VIEW_IIGS) return;
#endif
    // The browser shares the Apple renderer. Save its viewport and content
    // rectangle so card drawing cannot change later Apple input coordinates.
    const auto target = vs->target;
    const auto content = vs->content;
    if (view == SystemSettings::SS_VIEW_BOTH) vs->target.x += vs->target.w;
    // Only card pixels go in this monitor, even before its firmware sets a mode.
    Uint8 r, g, b, a;
    SDL_BlendMode blend;
    SDL_GetRenderDrawColor(vs->renderer, &r, &g, &b, &a);
    SDL_GetRenderDrawBlendMode(vs->renderer, &blend);
    SDL_SetRenderDrawColor(vs->renderer, 0, 0, 0, 255);
    SDL_SetRenderDrawBlendMode(vs->renderer, SDL_BLENDMODE_NONE);
    SDL_RenderFillRect(vs->renderer, &vs->target);
    if (auto *texture = prepare_texture(vs->renderer, main_texture)) {
        SDL_FRect source = {0, 0, float(main_texture.width), float(main_texture.height)};
        auto border = video.border_color();
        vs->render_frame_vga(texture, &source, border.r(), border.g(), border.b());
        card_content = vs->content;
    }
    SDL_SetRenderDrawColor(vs->renderer, r, g, b, a);
    SDL_SetRenderDrawBlendMode(vs->renderer, blend);
    vs->target = target;
    if (view == SystemSettings::SS_VIEW_BOTH) vs->content = content;
}

// Map card-monitor input to the existing IIgs input path; closing the native
// monitor changes the view preference rather than removing the card.
bool SecondSightDisplay::handle_event(const SDL_Event &event) {
    auto *vs = computer->video_system;
#if defined(__EMSCRIPTEN__)
    if (SystemSettings::instance().ss_view() != SystemSettings::SS_VIEW_BOTH || SDL_GetWindowFromEvent(&event) != vs->window)
        return false;
    auto *source_window = vs->window;
    auto *source_renderer = vs->renderer;
#else
    if (!window || SDL_GetWindowFromEvent(&event) != window) return false;
    auto *source_window = window;
    auto *source_renderer = renderer;
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        SystemSettings::instance().set_ss_view(SystemSettings::SS_VIEW_IIGS);
        return true;
    }
#endif
    SDL_Event guest = event;
    const bool motion = event.type == SDL_EVENT_MOUSE_MOTION;
    const bool button = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP;
    if (motion || button) {
        int w, h, pw, ph;
        SDL_GetWindowSize(source_window, &w, &h);
        SDL_GetRenderOutputSize(source_renderer, &pw, &ph);
        if (w < 1 || h < 1 || card_content.w <= 0 || card_content.h <= 0) return false;
        // SDL events use window units; rendering uses physical pixels on high-
        // density screens. Convert to pixels before testing the card rectangle.
        float x = (motion ? event.motion.x : event.button.x) * pw / w;
        float y = (motion ? event.motion.y : event.button.y) * ph / h;
#if defined(__EMSCRIPTEN__)
        const bool inside = x >= card_content.x && x < card_content.x + card_content.w &&
                            y >= card_content.y && y < card_content.y + card_content.h;
        // Once a drag starts in the card image, deliver its release there
        // even if the pointer leaves it. Otherwise the guest button sticks.
        if (!inside && !card_mouse_down) return false;
#endif
        // The IIgs mouse handler maps main-window content coordinates to guest
        // coordinates. Give either monitor the same mapping, without changing it.
        x = vs->content.x + (x - card_content.x) * vs->content.w / card_content.w;
        y = vs->content.y + (y - card_content.y) * vs->content.h / card_content.h;
        SDL_GetWindowSize(vs->window, &w, &h);
        SDL_GetRenderOutputSize(vs->renderer, &pw, &ph);
        if (pw < 1 || ph < 1) return false;
        if (motion) { guest.motion.x = x * w / pw; guest.motion.y = y * h / ph; }
        else {
            guest.button.x = x * w / pw; guest.button.y = y * h / ph;
            card_mouse_down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        }
        computer->dispatch->dispatch(guest);
        return true;
    }
#if defined(__EMSCRIPTEN__)
    return false;
#else
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP || event.type == SDL_EVENT_TEXT_INPUT)
        computer->dispatch->dispatch(guest);
    return true;
#endif
}
