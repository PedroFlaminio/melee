#ifndef MELEE_HOST_SDL_GL_RENDERER_HPP
#define MELEE_HOST_SDL_GL_RENDERER_HPP

#include <string>
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
};

void set_texture_images(std::vector<TextureImage> images);

/* Called once per displayed frame, before the geometry is read.  A viewer that
 * wants motion advances the animation and captures again from here, which is
 * what makes the window show a moving model instead of a still one. */
using FrameCallback = void (*)(void* user_data);

[[nodiscard]] bool show_captured_geometry(MeleeHostContext* context,
                                           std::string* error,
                                           FrameCallback on_frame = nullptr,
                                           void* user_data = nullptr);

} // namespace melee::render

#endif
