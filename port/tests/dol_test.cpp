#include "test.hpp"

#include <melee_host/dol.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

void put_be32(std::vector<unsigned char>& bytes, std::size_t at,
              std::uint32_t value)
{
    bytes[at] = static_cast<unsigned char>(value >> 24U);
    bytes[at + 1] = static_cast<unsigned char>(value >> 16U);
    bytes[at + 2] = static_cast<unsigned char>(value >> 8U);
    bytes[at + 3] = static_cast<unsigned char>(value);
}

/* One text section and one data section, the data section carrying a
 * recognisable byte pattern. */
std::filesystem::path write_dol()
{
    std::vector<unsigned char> bytes(0x160, 0);
    put_be32(bytes, 0x00, 0x100);        // text 0 file offset
    put_be32(bytes, 0x48, 0x80003100U);  // text 0 address
    put_be32(bytes, 0x90, 0x20);         // text 0 size
    put_be32(bytes, 0x1C, 0x120);        // data 0 file offset
    put_be32(bytes, 0x64, 0x80400000U);  // data 0 address
    put_be32(bytes, 0xAC, 0x40);         // data 0 size
    for (std::size_t index = 0; index < 0x40; ++index) {
        bytes[0x120 + index] = static_cast<unsigned char>(0xA0 + index);
    }
    const auto path =
        std::filesystem::temp_directory_path() /
        ("melee-dol-test-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()) +
         ".dol");
    std::ofstream(path, std::ios::binary)
        .write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    return path;
}

} // namespace

TEST_CASE("a DOL range is read by the console address the game uses")
{
    const auto path = write_dol();
    const std::string name = path.string();

    std::array<unsigned char, 4> bytes{};
    REQUIRE(melee_host_dol_read(name.c_str(), 0x80400010U, bytes.data(),
                                bytes.size()) == MELEE_HOST_OK);
    REQUIRE(bytes[0] == 0xB0);
    REQUIRE(bytes[3] == 0xB3);

    // The last bytes of a section, then one byte past it.
    REQUIRE(melee_host_dol_read(name.c_str(), 0x8040003CU, bytes.data(),
                                bytes.size()) == MELEE_HOST_OK);
    REQUIRE(melee_host_dol_read(name.c_str(), 0x8040003DU, bytes.data(),
                                bytes.size()) == MELEE_HOST_INVALID_ARGUMENT);
    // An address no section covers.
    REQUIRE(melee_host_dol_read(name.c_str(), 0x80500000U, bytes.data(),
                                bytes.size()) == MELEE_HOST_INVALID_ARGUMENT);

    std::filesystem::remove(path);
    REQUIRE(melee_host_dol_read(name.c_str(), 0x80400010U, bytes.data(),
                                bytes.size()) == MELEE_HOST_IO_ERROR);
}
