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
[[nodiscard]] bool show_captured_geometry(MeleeHostContext* context,
                                           std::string* error);

} // namespace melee::render

#endif
