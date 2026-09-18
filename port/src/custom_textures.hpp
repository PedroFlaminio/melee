#pragma once
#include <melee_host/gx.h>
#include <vector>
#include "render/play_window.hpp"

namespace melee::render {
    bool load_custom_texture(const MeleeHostGxTextureDesc& desc, const MeleeHostGxTlutDesc& tlut, std::vector<std::uint8_t>& rgba_out, std::uint16_t& out_w, std::uint16_t& out_h);
}
