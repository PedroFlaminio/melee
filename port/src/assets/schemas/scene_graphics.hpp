#ifndef MELEE_HOST_SCENE_GRAPHICS_SCHEMA_HPP
#define MELEE_HOST_SCENE_GRAPHICS_SCHEMA_HPP

#include "assets/hsd_runtime_archive.hpp"

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

struct HsdPObjGeometry {
    HsdRuntimeNode descriptor;
    std::uint16_t flags;
    std::uint16_t display_list_blocks;
    HsdRuntimeNode display_list;
    HsdAffineTransform transform;
    std::vector<HsdVertexDescriptor> vertices;
};

[[nodiscard]] HsdPObjGeometry find_first_scene_pobj(
    const HsdRuntimeArchive& archive, std::string_view public_symbol);
[[nodiscard]] std::vector<HsdPObjGeometry> find_scene_pobjs(
    const HsdRuntimeArchive& archive, std::string_view public_symbol);

} // namespace melee::assets::schemas

#endif
