#include "test.hpp"

#include "assets/hsd_runtime_archive.hpp"
#include "assets/schemas/db_common.hpp"
#include "assets/schemas/scene_graphics.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace {

void append_be32(std::vector<std::byte>& output, std::uint32_t value)
{
    output.push_back(static_cast<std::byte>(value >> 24U));
    output.push_back(static_cast<std::byte>(value >> 16U));
    output.push_back(static_cast<std::byte>(value >> 8U));
    output.push_back(static_cast<std::byte>(value));
}

void write_be16(std::vector<std::byte>& output, std::size_t offset,
                std::uint16_t value)
{
    output[offset] = static_cast<std::byte>(value >> 8U);
    output[offset + 1] = static_cast<std::byte>(value);
}

void write_be32(std::vector<std::byte>& output, std::size_t offset,
                std::uint32_t value)
{
    output[offset] = static_cast<std::byte>(value >> 24U);
    output[offset + 1] = static_cast<std::byte>(value >> 16U);
    output[offset + 2] = static_cast<std::byte>(value >> 8U);
    output[offset + 3] = static_cast<std::byte>(value);
}

std::vector<std::byte> make_db_common_archive()
{
    constexpr std::string_view symbol = "dbLoadCommonData";
    constexpr std::uint32_t data_size = 36;
    constexpr std::uint32_t file_size =
        32 + data_size + 24 + 8 +
        static_cast<std::uint32_t>(symbol.size()) + 1;
    std::vector<std::byte> bytes;
    for (const std::uint32_t value : std::array<std::uint32_t, 8>{
             file_size, data_size, 6, 1, 0, 0x01000000, 0, 0 }) {
        append_be32(bytes, value);
    }
    append_be32(bytes, 24);
    append_be32(bytes, 28);
    append_be32(bytes, 32);
    append_be32(bytes, 0);
    append_be32(bytes, 4);
    append_be32(bytes, 8);
    for (const char value : std::array{ 'o', 'n', 'e', '\0', 't', 'w', 'o',
                                        '\0', 't', 'r', 'i', '\0' }) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    append_be32(bytes, 0);
    append_be32(bytes, 4);
    append_be32(bytes, 8);
    append_be32(bytes, 12);
    append_be32(bytes, 16);
    append_be32(bytes, 20);
    append_be32(bytes, 12);
    append_be32(bytes, 0);
    for (const char value : symbol) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    bytes.push_back(std::byte{ 0 });
    return bytes;
}

std::vector<std::byte> make_scene_archive()
{
    constexpr std::string_view symbol = "scene";
    constexpr std::uint32_t data_size = 0x1C0;
    constexpr std::array<std::uint32_t, 12> relocations{
        0x00, 0x20, 0x30, 0x50, 0x88, 0x8C, 0xA8, 0xB0, 0xC8, 0xCC, 0xF4,
        0x1AC
    };
    constexpr std::uint32_t file_size =
        32 + data_size + static_cast<std::uint32_t>(relocations.size()) * 4 +
        8 + static_cast<std::uint32_t>(symbol.size()) + 1;

    std::vector<std::byte> bytes;
    for (const std::uint32_t value : std::array<std::uint32_t, 8>{
             file_size, data_size,
             static_cast<std::uint32_t>(relocations.size()), 1, 0,
             0x01000000, 0, 0 }) {
        append_be32(bytes, value);
    }
    const std::size_t data_begin = bytes.size();
    bytes.resize(data_begin + data_size);
    const auto field32 = [&](std::size_t offset, std::uint32_t value) {
        write_be32(bytes, data_begin + offset, value);
    };
    const auto field16 = [&](std::size_t offset, std::uint16_t value) {
        write_be16(bytes, data_begin + offset, value);
    };

    field32(0x00, 0x20);  // SceneDesc.models
    field32(0x20, 0x30);  // models[0]
    field32(0x30, 0x40);  // DynamicModelDesc.joint
    field32(0x44, 0);     // HSD_Joint.flags
    field32(0x50, 0x80);  // HSD_Joint.dobjdesc
    field32(0x88, 0xC0);  // HSD_DObjDesc.mobjdesc
    field32(0x60, 0x3F800000); // HSD_Joint.scale.x = 1
    field32(0x64, 0x3F800000); // HSD_Joint.scale.y = 1
    field32(0x68, 0x3F800000); // HSD_Joint.scale.z = 1
    field32(0x8C, 0xA0);  // HSD_DObjDesc.pobjdesc
    field32(0xA8, 0xE0);  // HSD_PObjDesc.verts
    field16(0xAC, 0x8000);
    field16(0xAE, 1);
    field32(0xB0, 0x140); // HSD_PObjDesc.display

    field32(0xC4, 0x10);  // HSD_MObjDesc.rendermode
    field32(0xC8, 0x160); // HSD_MObjDesc.texdesc
    field32(0xCC, 0x100); // HSD_MObjDesc.mat
    bytes[data_begin + 0x104] = std::byte{ 64 };  // diffuse R
    bytes[data_begin + 0x105] = std::byte{ 128 }; // diffuse G
    bytes[data_begin + 0x106] = std::byte{ 192 }; // diffuse B
    bytes[data_begin + 0x107] = std::byte{ 255 }; // diffuse A
    field32(0x10C, 0x3F000000); // HSD_Material.alpha = 0.5
    field32(0x1AC, 0x1B0); // HSD_TObjDesc.imagedesc
    field16(0x1B4, 32);    // HSD_ImageDesc.width
    field16(0x1B6, 16);    // HSD_ImageDesc.height
    field32(0x1B8, 5);     // HSD_ImageDesc.format = GX_RGB5A3

    field32(0xE0, 9);     // GX_VA_POS
    field32(0xE4, 2);     // GX_INDEX8
    field32(0xE8, 1);     // GX_POS_XYZ
    field32(0xEC, 3);     // GX_S16
    bytes[data_begin + 0xF0] = std::byte{ 2 };
    field16(0xF2, 6);
    field32(0xF4, 0x180); // vertex array
    field32(0xF8, 0xFF);  // descriptor terminator

    bytes[data_begin + 0x140] = std::byte{ 0x90 };
    bytes[data_begin + 0x142] = std::byte{ 3 };
    bytes[data_begin + 0x143] = std::byte{ 0 };
    bytes[data_begin + 0x144] = std::byte{ 1 };
    bytes[data_begin + 0x145] = std::byte{ 2 };

    for (const std::uint32_t relocation : relocations) {
        append_be32(bytes, relocation);
    }
    append_be32(bytes, 0);
    append_be32(bytes, 0);
    for (const char value : symbol) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    bytes.push_back(std::byte{ 0 });
    return bytes;
}

} // namespace

TEST_CASE("DbCo schema decodes its three pointer fields without native casts")
{
    const auto bytes = make_db_common_archive();
    const melee::assets::HsdRuntimeArchive archive(bytes);
    const auto schema = melee::assets::schemas::decode_db_load_common_data(archive);
    REQUIRE(schema.bonus_names.data_offset == 0);
    REQUIRE(schema.motion_state_names.data_offset == 4);
    REQUIRE(schema.submotion_names.data_offset == 8);
    REQUIRE(archive.read_c_string(
                archive.reference_at(schema.bonus_names, 0)) == "one");
    REQUIRE(archive.read_c_string(
                archive.reference_at(schema.motion_state_names, 0)) == "two");
    REQUIRE(archive.read_c_string(
                archive.reference_at(schema.submotion_names, 0)) == "tri");
    const auto tables =
        melee::assets::schemas::decode_db_common_name_tables(archive);
    REQUIRE(tables.bonus_names.size() == 1);
    REQUIRE(tables.motion_state_names.size() == 1);
    REQUIRE(tables.submotion_names.size() == 1);
    REQUIRE(tables.bonus_names[0] == "one");
    REQUIRE(tables.motion_state_names[0] == "two");
    REQUIRE(tables.submotion_names[0] == "tri");
}

TEST_CASE("scene schema traverses Joint DObj and PObj as validated offsets")
{
    const auto bytes = make_scene_archive();
    const melee::assets::HsdRuntimeArchive archive(bytes);
    const auto geometry =
        melee::assets::schemas::find_first_scene_pobj(archive, "scene");
    const auto geometries =
        melee::assets::schemas::find_scene_pobjs(archive, "scene");

    REQUIRE(geometries.size() == 1);
    REQUIRE(geometry.descriptor.data_offset == 0xA0);
    REQUIRE(geometry.flags == 0x8000);
    REQUIRE(geometry.display_list_blocks == 1);
    REQUIRE(geometry.display_list.data_offset == 0x140);
    REQUIRE(geometry.vertices.size() == 1);
    REQUIRE(geometry.vertices[0].attribute == 9);
    REQUIRE(geometry.vertices[0].attribute_type == 2);
    REQUIRE(geometry.vertices[0].component_type == 3);
    REQUIRE(geometry.vertices[0].fractional_bits == 2);
    REQUIRE(geometry.vertices[0].stride == 6);
    REQUIRE(geometry.vertices[0].array.has_value());
    REQUIRE(geometry.vertices[0].array->data_offset == 0x180);
    REQUIRE(geometry.transform.values[0][0] == 1.0F);
    REQUIRE(geometry.transform.values[1][1] == 1.0F);
    REQUIRE(geometry.transform.values[2][2] == 1.0F);
    REQUIRE(geometry.material.render_mode == 0x10);
    REQUIRE(geometry.material.has_texture);
    REQUIRE(geometry.material.diffuse[0] == 64);
    REQUIRE(geometry.material.diffuse[1] == 128);
    REQUIRE(geometry.material.diffuse[2] == 192);
    REQUIRE(geometry.material.diffuse[3] == 128);
    REQUIRE(!geometry.material.image_data.has_value());
    REQUIRE(geometry.material.texture_width == 32);
    REQUIRE(geometry.material.texture_height == 16);
    REQUIRE(geometry.material.texture_format == 5);
    REQUIRE(geometry.material.texture_wrap_s == 0);
    REQUIRE(geometry.material.texture_wrap_t == 0);
    REQUIRE(archive.has_reference_at({ 0xE0 }, 0x14));
    REQUIRE(archive.read_u16({ 0xA0 }, 0xE) == 1);
    REQUIRE(archive.bytes_at(geometry.display_list, 3).size() == 3);
}
