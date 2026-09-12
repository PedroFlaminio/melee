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

/* Decodes the GameCube color-indexed formats (C4, C8, and C14X2) using a
 * big-endian GX TLUT. `tlut_format` is GX_TL_IA8 (0), GX_TL_RGB565 (1), or
 * GX_TL_RGB5A3 (2).  Keeping the palette separate mirrors the GPU interface
 * and prevents an HSD disk pointer from leaking into the host renderer. */
[[nodiscard]] DecodedTexture decode_gx_texture_with_tlut(
    std::span<const std::byte> data, std::uint16_t width,
    std::uint16_t height, std::uint32_t format,
    std::span<const std::byte> tlut, std::uint32_t tlut_format);

} // namespace melee::assets

#endif
