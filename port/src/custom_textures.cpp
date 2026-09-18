#include "custom_textures.hpp"
#include "assets/gx_texture.hpp"

#define XXH_INLINE_ALL
#include "xxhash.h"

#include "stb_image.h"

#include <filesystem>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace melee::render {

static std::string format_hash(std::uint64_t hash) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

bool load_custom_texture(const MeleeHostGxTextureDesc& desc, const MeleeHostGxTlutDesc& tlut, std::vector<std::uint8_t>& rgba_out) {
    if (!is_custom_textures_enabled()) return false;
    
    std::filesystem::path textures_dir = "textures";
    if (!std::filesystem::exists(textures_dir)) return false;

    std::size_t byte_count = melee::assets::gx_texture_data_size(desc.width, desc.height, desc.format);
    if (byte_count == 0 || desc.image == nullptr) return false;

    std::uint64_t textureHash = XXH64(desc.image, byte_count, 0);
    
    std::string filename;
    if (desc.color_indexed && tlut.loaded && tlut.entries != nullptr) {
        std::uint64_t tlutHash = XXH64(tlut.entries, tlut.entry_count * 2, 0);
        filename = "tex1_" + std::to_string(desc.width) + "x" + std::to_string(desc.height) + "_" + 
                   format_hash(textureHash) + "_" + format_hash(tlutHash) + "_" + std::to_string(desc.format) + ".png";
    } else {
        filename = "tex1_" + std::to_string(desc.width) + "x" + std::to_string(desc.height) + "_" + 
                   format_hash(textureHash) + "_" + std::to_string(desc.format) + ".png";
    }

    std::filesystem::path path = textures_dir / filename;
    if (!std::filesystem::exists(path)) {
        // try without tlut if indexed
        if (desc.color_indexed) {
            filename = "tex1_" + std::to_string(desc.width) + "x" + std::to_string(desc.height) + "_" + 
                       format_hash(textureHash) + "_" + std::to_string(desc.format) + ".png";
            path = textures_dir / filename;
            if (!std::filesystem::exists(path)) return false;
        } else {
            return false;
        }
    }

    int w, h, channels;
    stbi_uc* pixels = stbi_load(path.string().c_str(), &w, &h, &channels, 4);
    if (!pixels) {
        return false;
    }

    if (w != desc.width || h != desc.height) {
        // Warning: mismatch
    }

    std::size_t out_size = static_cast<std::size_t>(w) * h * 4;
    rgba_out.assign(pixels, pixels + out_size);
    stbi_image_free(pixels);

    return true;
}

} // namespace melee::render
