#include "test.hpp"

#include "assets/hsd_runtime_archive.hpp"
#include "assets/schemas/db_common.hpp"

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
