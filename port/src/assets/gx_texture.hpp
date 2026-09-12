#ifndef MELEE_HOST_GX_TEXTURE_HPP
#define MELEE_HOST_GX_TEXTURE_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace melee::assets {

struct DecodedTexture {
    std::uint16_t width;
    std::uint16_t height;
    std::vector<std::uint8_t> rgba;
};

[[nodiscard]] std::size_t gx_texture_data_size(std::uint16_t width,
                                                std::uint16_t height,
                                                std::uint32_t format);
[[nodiscard]] DecodedTexture decode_gx_texture(std::span<const std::byte> data,
                                                std::uint16_t width,
                                                std::uint16_t height,
                                                std::uint32_t format);

} // namespace melee::assets

#endif
