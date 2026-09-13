#include "test.hpp"

#include "assets/gx_texture.hpp"

#include <array>
#include <cstddef>

TEST_CASE("GX I4 decoder follows the 8 by 8 tiled layout")
{
    std::array<std::byte, 32> data{};
    data[0] = std::byte{ 0x0F };
    data[1] = std::byte{ 0x8A };
    const auto image = melee::assets::decode_gx_texture(data, 8, 8, 0);

    REQUIRE(image.rgba.size() == 8U * 8U * 4U);
    REQUIRE(image.rgba[0] == 0);
    REQUIRE(image.rgba[4] == 255);
    REQUIRE(image.rgba[8] == 136);
    REQUIRE(image.rgba[12] == 170);
    // GX gives an intensity texel the same value in alpha.
    REQUIRE(image.rgba[3] == 0);
    REQUIRE(image.rgba[7] == 255);
    REQUIRE(image.rgba[15] == 170);
}

TEST_CASE("GX I8 decoder carries intensity into alpha")
{
    std::array<std::byte, 32> data{};
    data[0] = std::byte{ 0x40 };
    data[9] = std::byte{ 0xC8 };
    const auto image = melee::assets::decode_gx_texture(data, 8, 4, 1);

    REQUIRE(image.rgba[0] == 0x40);
    REQUIRE(image.rgba[3] == 0x40);
    REQUIRE(image.rgba[(8U + 1U) * 4U] == 0xC8);
    REQUIRE(image.rgba[(8U + 1U) * 4U + 3U] == 0xC8);
}

TEST_CASE("GX IA4 decoder follows the 8 by 4 tiled layout")
{
    std::array<std::byte, 32> data{};
    data[0] = std::byte{ 0xA3 };
    data[7] = std::byte{ 0x24 };
    data[8] = std::byte{ 0xF5 };
    const auto image = melee::assets::decode_gx_texture(data, 8, 4, 2);

    REQUIRE(image.rgba[0] == 51);
    REQUIRE(image.rgba[3] == 170);
    REQUIRE(image.rgba[7U * 4U] == 68);
    REQUIRE(image.rgba[8U * 4U + 3] == 255);
    REQUIRE(melee::assets::gx_texture_data_size(8, 4, 2) == 32);
}

TEST_CASE("GX RGB5A3 decoder handles opaque and transparent encodings")
{
    std::array<std::byte, 32> data{};
    data[0] = std::byte{ 0xFC };
    data[1] = std::byte{ 0x00 };
    data[2] = std::byte{ 0x0F };
    data[3] = std::byte{ 0xFF };
    const auto image = melee::assets::decode_gx_texture(data, 4, 4, 5);

    REQUIRE(image.rgba[0] == 255);
    REQUIRE(image.rgba[1] == 0);
    REQUIRE(image.rgba[3] == 255);
    REQUIRE(image.rgba[4] == 255);
    REQUIRE(image.rgba[5] == 255);
    REQUIRE(image.rgba[7] == 0);
}

TEST_CASE("GX RGB565 and RGBA8 decoders follow their 4 by 4 tiles")
{
    std::array<std::byte, 32> rgb565{};
    rgb565[0] = std::byte{ 0x07 };
    rgb565[1] = std::byte{ 0xE0 };
    const auto rgb = melee::assets::decode_gx_texture(rgb565, 4, 4, 4);
    REQUIRE(rgb.rgba[0] == 0);
    REQUIRE(rgb.rgba[1] == 255);
    REQUIRE(rgb.rgba[2] == 0);

    std::array<std::byte, 64> rgba8{};
    rgba8[0] = std::byte{ 12 };
    rgba8[1] = std::byte{ 34 };
    rgba8[32] = std::byte{ 56 };
    rgba8[33] = std::byte{ 78 };
    const auto rgba = melee::assets::decode_gx_texture(rgba8, 4, 4, 6);
    REQUIRE(rgba.rgba[0] == 34);
    REQUIRE(rgba.rgba[1] == 56);
    REQUIRE(rgba.rgba[2] == 78);
    REQUIRE(rgba.rgba[3] == 12);
}

TEST_CASE("GX IA8 decoder preserves intensity and alpha")
{
    std::array<std::byte, 32> data{};
    data[0] = std::byte{ 25 };
    data[1] = std::byte{ 200 };
    const auto image = melee::assets::decode_gx_texture(data, 4, 4, 3);

    REQUIRE(image.rgba[0] == 200);
    REQUIRE(image.rgba[1] == 200);
    REQUIRE(image.rgba[2] == 200);
    REQUIRE(image.rgba[3] == 25);
}

TEST_CASE("GX CMPR decoder follows the 8 by 8 tiled subblocks")
{
    std::array<std::byte, 32> data{};
    data[0] = std::byte{ 0xF8 };
    data[1] = std::byte{ 0x00 };
    data[2] = std::byte{ 0x07 };
    data[3] = std::byte{ 0xE0 };
    const auto image = melee::assets::decode_gx_texture(data, 8, 8, 14);

    REQUIRE(image.rgba[0] == 255);
    REQUIRE(image.rgba[1] == 0);
    REQUIRE(image.rgba[3] == 255);
    REQUIRE(melee::assets::gx_texture_data_size(8, 8, 14) == 32);
}

TEST_CASE("GX C4 decoder uses a RGB565 TLUT in tiled order")
{
    std::array<std::byte, 32> data{};
    data[0] = std::byte{ 0x01 };
    std::array<std::byte, 32> tlut{};
    tlut[2] = std::byte{ 0xF8 };
    tlut[3] = std::byte{ 0x00 };
    const auto image = melee::assets::decode_gx_texture_with_tlut(
        data, 8, 8, 8, tlut, 1);

    REQUIRE(image.rgba[0] == 0);
    REQUIRE(image.rgba[4] == 255);
    REQUIRE(image.rgba[5] == 0);
    REQUIRE(image.rgba[6] == 0);
    REQUIRE(image.rgba[7] == 255);
    REQUIRE(melee::assets::gx_texture_data_size(8, 8, 8) == 32);
}

TEST_CASE("GX C8 and C14X2 decoders preserve TLUT alpha")
{
    std::array<std::byte, 32> c8{};
    c8[0] = std::byte{ 1 };
    std::array<std::byte, 4> ia8_tlut{
        std::byte{ 0 }, std::byte{ 0 }, std::byte{ 64 }, std::byte{ 200 }
    };
    const auto indexed = melee::assets::decode_gx_texture_with_tlut(
        c8, 8, 4, 9, ia8_tlut, 0);
    REQUIRE(indexed.rgba[0] == 200);
    REQUIRE(indexed.rgba[3] == 64);

    std::array<std::byte, 32> c14{};
    c14[0] = std::byte{ 0x00 };
    c14[1] = std::byte{ 0x01 };
    const auto wide = melee::assets::decode_gx_texture_with_tlut(
        c14, 4, 4, 10, ia8_tlut, 0);
    REQUIRE(wide.rgba[0] == 200);
    REQUIRE(wide.rgba[3] == 64);
    REQUIRE(melee::assets::gx_texture_data_size(4, 4, 10) == 32);
}
