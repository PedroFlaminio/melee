#ifndef MELEE_HOST_SCENE_GRAPHICS_SCHEMA_HPP
#define MELEE_HOST_SCENE_GRAPHICS_SCHEMA_HPP

#include "assets/hsd_runtime_archive.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace melee::assets::schemas {

struct HsdVertexDescriptor {
    std::uint32_t attribute;
    std::uint32_t attribute_type;
    std::uint32_t component_count;
    std::uint32_t component_type;
    std::uint8_t fractional_bits;
    std::uint16_t stride;
    std::optional<HsdRuntimeNode> array;
};

struct HsdAffineTransform {
    float values[3][4];
};

struct HsdMaterial {
    std::uint32_t render_mode;
    std::array<std::uint8_t, 4> diffuse;
    bool has_texture;
    std::optional<HsdRuntimeNode> image_data;
    std::optional<HsdRuntimeNode> tlut_data;
    std::uint16_t texture_width;
    std::uint16_t texture_height;
    std::uint32_t texture_format;
    std::uint16_t tlut_entries;
    std::uint32_t tlut_format;
    std::uint32_t texture_wrap_s;
    std::uint32_t texture_wrap_t;
};

struct HsdPObjGeometry {
    HsdRuntimeNode descriptor;
    std::uint16_t flags;
    std::uint16_t display_list_blocks;
    HsdRuntimeNode display_list;
    HsdAffineTransform transform;
    HsdMaterial material;
    std::vector<HsdVertexDescriptor> vertices;
};

[[nodiscard]] HsdPObjGeometry find_first_scene_pobj(
    const HsdRuntimeArchive& archive, std::string_view public_symbol);
[[nodiscard]] std::vector<HsdPObjGeometry> find_scene_pobjs(
    const HsdRuntimeArchive& archive, std::string_view public_symbol);

} // namespace melee::assets::schemas

#endif
