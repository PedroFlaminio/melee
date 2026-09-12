#include "test.hpp"

#include "assets/hsd_archive.hpp"
#include "assets/hsd_runtime_archive.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

void append_be32(std::vector<std::byte>& output, std::uint32_t value)
{
    output.push_back(static_cast<std::byte>((value >> 24U) & 0xFFU));
    output.push_back(static_cast<std::byte>((value >> 16U) & 0xFFU));
    output.push_back(static_cast<std::byte>((value >> 8U) & 0xFFU));
    output.push_back(static_cast<std::byte>(value & 0xFFU));
}

std::vector<std::byte> make_archive()
{
    constexpr std::uint32_t data_size = 8;
    constexpr std::uint32_t file_size = 32 + data_size + 4 + 8 + 5;

    std::vector<std::byte> bytes;
    append_be32(bytes, file_size);
    append_be32(bytes, data_size);
    append_be32(bytes, 1);
    append_be32(bytes, 1);
    append_be32(bytes, 0);
    append_be32(bytes, 0x01000000);
    append_be32(bytes, 0);
    append_be32(bytes, 0);

    append_be32(bytes, 4);
    append_be32(bytes, 0x12345678);
    append_be32(bytes, 0);
    append_be32(bytes, 4);
    append_be32(bytes, 0);
    for (const char value : std::array{ 'r', 'o', 'o', 't', '\0' }) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

} // namespace

TEST_CASE("HSD archive preserves 32-bit offsets on a 64-bit host")
{
    const auto bytes = make_archive();
    const melee::assets::HsdArchiveView archive(bytes);

    REQUIRE(archive.header().file_size == bytes.size());
    REQUIRE(archive.header().data_size == 8);
    REQUIRE(archive.relocation_fields().size() == 1);
    REQUIRE(archive.relocation_fields()[0] == 0);
    REQUIRE(archive.relocated_target(0) == 4);
    REQUIRE(archive.public_data_offset("root") == 4);
    const auto public_symbols = archive.public_symbols();
    REQUIRE(public_symbols.size() == 1);
    REQUIRE(public_symbols[0].name == "root");
    REQUIRE(public_symbols[0].data_offset == 4);
}

TEST_CASE("HSD archive rejects a truncated header")
{
    const std::array<std::byte, 4> bytes{};
    bool threw = false;
    try {
        static_cast<void>(melee::assets::HsdArchiveView(bytes));
    } catch (const melee::assets::HsdArchiveError&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("HSD runtime archive preserves a relocatable graph as offsets")
{
    const auto bytes = make_archive();
    const melee::assets::HsdRuntimeArchive archive(bytes);
    REQUIRE(archive.public_root("root").data_offset == 4);
    REQUIRE(archive.read_u32(archive.public_root("root"), 0) == 0x12345678U);
    REQUIRE(archive.reference_at({ 0 }, 0).data_offset == 4);
    const auto references = archive.internal_references();
    REQUIRE(references.size() == 1);
    REQUIRE(references[0].field_offset == 0);
    REQUIRE(references[0].target.data_offset == 4);
}
