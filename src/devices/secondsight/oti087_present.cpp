/*
 *   Upload Second Sight's Oak image to a texture in SDL, the host graphics
 *   library. VRAM means the card's video memory. SecondSightDisplay
 *   calls upload_frame() on the main thread after SSCard has stopped its worker.
 *   The Oak renderer supplies opaque pixels; SDL receives only rows that changed.
 *   A new texture or changed geometry forces a full copy.
 *
 *   The card owns its pixel cache; the caller owns the texture. ARGB8888 means
 *   four eight-bit components: alpha, red, green and blue. XRGB8888 has the same
 *   layout but ignores alpha. No chip timing is changed by presentation.
 *   The source for raster/color behavior is the renderer's Oak OTI-087 databook
 *   and Brooktree Bt481A L481A_C references; texture caching is host policy.
 */

#include "oti087.hpp"
#include "videosystem.hpp"

// Upload changed rows after rasterization; a new SDL texture requires a full
// copy even if the emulated VRAM has not changed.
bool OTI087::upload_frame(SDL_Texture *texture, bool full) {
    int const width = timing.width, height = timing.height;
    // texture must match the current raster size. full forces a copy even
    // when the cached image is unchanged; false retains it on the SDL side.
    frame_pixels.resize(width * height);
    render_frame({frame_pixels.data(), width, height, width});
    if (full || width != present_width || height != present_height) {
        present_top = 0;
        present_bottom = height - 1;
    }
    // Native ARGB8888 pixels are opaque; XRGB8888 needs no conversion. Upload damage
    // directly, including rows refreshed by a debug snapshot between frames.
    int const top = std::max(0, present_top), bottom = std::min(height - 1, present_bottom);
    if (bottom >= top) {
        SDL_Rect damage = {0, top, width, bottom - top + 1};
        if (!SDL_UpdateTexture(texture, &damage, frame_pixels.data() + size_t(top) * width,
                               width * sizeof(uint32_t))) return false;
    }
    // Clear the pending upload range only after SDL accepts it. On failure
    // the early return above leaves those rows pending for another attempt.
    present_top = 0;
    present_bottom = -1;
    present_width = width;
    present_height = height;
    return true;
}
