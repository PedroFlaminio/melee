#include "custom_textures.hpp"
#include "assets/gx_texture.hpp"

#define XXH_INLINE_ALL
#include "xxhash.h"

_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")
_Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"")
_Pragma("GCC diagnostic ignored \"-Wimplicit-int-conversion\"")
#include "stb_image.h"
_Pragma("GCC diagnostic pop")

#include <filesystem>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <string>

namespace melee::render {

static std::unordered_map<std::string, std::filesystem::path> g_texture_index;
static bool g_index_built = false;

static void build_texture_index() {
    if (g_index_built) return;
    std::filesystem::path textures_dir = "textures";
    if (std::filesystem::exists(textures_dir) && std::filesystem::is_directory(textures_dir)) {
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(textures_dir, ec)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.ends_with(".png") || filename.ends_with(".dds")) {
                    g_texture_index[filename] = entry.path();
                }
            }
        }
    }
    g_index_built = true;
}

static std::string format_hash(std::uint64_t hash) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

bool load_custom_texture(const MeleeHostGxTextureDesc& desc, const MeleeHostGxTlutDesc& tlut, std::vector<std::uint8_t>& rgba_out, std::uint16_t& out_w, std::uint16_t& out_h) {
    if (!is_custom_textures_enabled()) return false;
    
    build_texture_index();
    if (g_texture_index.empty()) return false;

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

    auto it = g_texture_index.find(filename);
    if (it == g_texture_index.end()) {
        if (desc.color_indexed) {
            filename = "tex1_" + std::to_string(desc.width) + "x" + std::to_string(desc.height) + "_" + 
                       format_hash(textureHash) + "_" + std::to_string(desc.format) + ".png";
            it = g_texture_index.find(filename);
        }
    }

    if (it == g_texture_index.end()) {
        return false;
    }

    int w, h, channels;
    stbi_uc* pixels = stbi_load(it->second.string().c_str(), &w, &h, &channels, 4);
    if (!pixels) {
        return false;
    }

    std::size_t out_size = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4;
    rgba_out.assign(pixels, pixels + out_size);
    out_w = static_cast<std::uint16_t>(w);
    out_h = static_cast<std::uint16_t>(h);
    stbi_image_free(pixels);
    

    return true;
}

} // namespace melee::render
