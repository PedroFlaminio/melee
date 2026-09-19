#pragma once
#include <melee_host/gx.h>
#include <vector>
#include "render/play_window.hpp"

#include <cstdint>

namespace melee::render {
    bool load_custom_texture(const MeleeHostGxTextureDesc& desc, const MeleeHostGxTlutDesc& tlut, std::vector<std::uint8_t>& rgba_out, std::uint16_t& out_w, std::uint16_t& out_h);
    void set_custom_textures_enabled(bool enabled);
    [[nodiscard]] std::uint64_t custom_texture_revision();
}
