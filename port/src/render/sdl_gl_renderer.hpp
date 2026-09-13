#ifndef MELEE_HOST_SDL_GL_RENDERER_HPP
#define MELEE_HOST_SDL_GL_RENDERER_HPP

#include <string>
#include <cstddef>
#include <cstdint>
#include <vector>

struct MeleeHostContext;

namespace melee::render {

struct TextureImage {
    std::uint16_t width;
    std::uint16_t height;
    std::uint32_t wrap_s;
    std::uint32_t wrap_t;
    std::vector<std::uint8_t> rgba;
    /* The texture asked for bilinear magnification.  Mipmaps are not decoded,
     * so minification follows the same choice. */
    bool linear_filter;
};

/* Images in the order of the captured texture table, which is the space the
 * captured texture sets index. */
void set_texture_images(std::vector<TextureImage> images);

/* Called once per displayed frame, before the geometry is read.  A viewer that
 * wants motion advances the animation and captures again from here, which is
 * what makes the window show a moving model instead of a still one. */
using FrameCallback = void (*)(void* user_data);

/* Shows the captured geometry, each draw coloured by its TEV program evaluated
 * per fragment.  When the MELEE_HOST_SCREENSHOT environment variable names a
 * path, one frame is rendered off screen into that BMP instead and the window
 * never opens. */
[[nodiscard]] bool show_captured_geometry(MeleeHostContext* context,
                                           std::string* error,
                                           FrameCallback on_frame = nullptr,
                                           void* user_data = nullptr);

struct TevConformanceReport {
    std::size_t programs = 0;
    std::size_t cases = 0;
    std::size_t mismatches = 0;
    std::string first_mismatch;
};

/* Holds the generated shaders to the CPU reference: every captured pair of
 * TEV program and pixel state is rendered into a one-pixel target with
 * random rasterized colours and texels, and the pixel read back must equal
 * what melee::gx::evaluate_tev and the alpha test compute, discards included.
 * Needs a GL context but never shows a window. */
[[nodiscard]] bool run_tev_conformance(std::size_t cases_per_program,
                                       TevConformanceReport* report,
                                       std::string* error);

} // namespace melee::render

#endif
