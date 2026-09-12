#include "test.hpp"

#include <array>
#include <bit>
#include <cmath>

extern "C" {
#include <dolphin/gx/GXVert.h>
#include <dolphin/gx/GXGeometry.h>
#include <dolphin/gx/GXDispList.h>
#include <melee_host/gx.h>
}

TEST_CASE("host GX assembles a position-only GX_TRIANGLES primitive")
{
    melee_host_gx_reset_command_log();
    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GXPosition3f32(-1.0F, -1.0F, 0.0F);
    GXPosition3f32(1.0F, -1.0F, 0.0F);
    GXPosition3f32(0.0F, 1.0F, 0.0F);
    GXEnd();

    REQUIRE(melee_host_gx_command_count() == 11);
    REQUIRE(melee_host_gx_triangle_count() == 1);
    MeleeHostGxTriangle triangle{};
    REQUIRE(melee_host_gx_triangle_at(0, &triangle));
    REQUIRE(triangle.vertices[0].x == -1.0F);
    REQUIRE(triangle.vertices[1].x == 1.0F);
    REQUIRE(triangle.vertices[2].x == 0.0F);
    REQUIRE(triangle.vertices[2].y == 1.0F);
}

TEST_CASE("host GX vertex writes never access the GameCube FIFO address")
{
    melee_host_gx_reset_command_log();
    GXCmd1u8(0x98U);
    GXPosition3f32(1.0F, -2.5F, 3.25F);
    GXColor4u8(1U, 2U, 3U, 4U);

    REQUIRE(melee_host_gx_command_count() == 8);

    MeleeHostGxCommand command{};
    REQUIRE(melee_host_gx_command_at(0, &command));
    REQUIRE(command.type == MELEE_HOST_GX_U8);
    REQUIRE(command.bits == 0x98U);

    REQUIRE(melee_host_gx_command_at(2, &command));
    REQUIRE(command.type == MELEE_HOST_GX_F32);
    REQUIRE(command.bits == std::bit_cast<mh_u32>(-2.5F));

    REQUIRE(melee_host_gx_command_at(7, &command));
    REQUIRE(command.type == MELEE_HOST_GX_U8);
    REQUIRE(command.bits == 4U);
}

TEST_CASE("host GX converts strips fans and quads to triangle lists")
{
    MeleeHostGxTriangle triangle{};

    melee_host_gx_reset_command_log();
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
    for (int x = 0; x < 4; ++x) {
        GXPosition3f32(static_cast<float>(x), 0.0F, 0.0F);
    }
    REQUIRE(melee_host_gx_triangle_count() == 2);
    REQUIRE(melee_host_gx_triangle_at(1, &triangle));
    REQUIRE(triangle.vertices[0].x == 2.0F);
    REQUIRE(triangle.vertices[1].x == 1.0F);
    REQUIRE(triangle.vertices[2].x == 3.0F);

    melee_host_gx_reset_command_log();
    GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, 4);
    for (int x = 0; x < 4; ++x) {
        GXPosition3f32(static_cast<float>(x), 0.0F, 0.0F);
    }
    REQUIRE(melee_host_gx_triangle_count() == 2);
    REQUIRE(melee_host_gx_triangle_at(1, &triangle));
    REQUIRE(triangle.vertices[0].x == 0.0F);
    REQUIRE(triangle.vertices[1].x == 2.0F);
    REQUIRE(triangle.vertices[2].x == 3.0F);

    melee_host_gx_reset_command_log();
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    for (int x = 0; x < 4; ++x) {
        GXPosition3f32(static_cast<float>(x), 0.0F, 0.0F);
    }
    REQUIRE(melee_host_gx_triangle_count() == 2);
    REQUIRE(melee_host_gx_triangle_at(1, &triangle));
    REQUIRE(triangle.vertices[0].x == 0.0F);
    REQUIRE(triangle.vertices[1].x == 2.0F);
    REQUIRE(triangle.vertices[2].x == 3.0F);
}

TEST_CASE("host GX captures direct position normal color and UV attributes")
{
    melee_host_gx_reset_command_log();
    GXBegin(GX_POINTS, GX_VTXFMT0, 1);
    GXPosition3f32(1.0F, 2.0F, 3.0F);
    GXNormal3f32(0.0F, 0.0F, 1.0F);
    GXColor4u8(10, 20, 30, 40);
    GXTexCoord2f32(0.25F, 0.75F);
    GXEnd();

    REQUIRE(melee_host_gx_captured_vertex_count() == 1);
    MeleeHostGxCapturedVertex vertex{};
    REQUIRE(melee_host_gx_captured_vertex_at(0, &vertex));
    REQUIRE(vertex.attributes ==
            (MELEE_HOST_GX_VERTEX_POSITION | MELEE_HOST_GX_VERTEX_NORMAL |
             MELEE_HOST_GX_VERTEX_COLOR | MELEE_HOST_GX_VERTEX_TEXCOORD));
    REQUIRE(vertex.position.y == 2.0F);
    REQUIRE(vertex.normal.z == 1.0F);
    REQUIRE(vertex.color[2] == 30);
    REQUIRE(vertex.texcoord[0] == 0.25F);
    REQUIRE(vertex.texcoord[1] == 0.75F);
}

TEST_CASE("host GX keeps complete attributes when assembling a triangle")
{
    melee_host_gx_reset_command_log();
    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    for (int index = 0; index < 3; ++index) {
        GXPosition3f32(static_cast<float>(index), 0.0F, 0.0F);
        GXNormal3f32(0.0F, 0.0F, 1.0F);
        GXColor4u8(static_cast<u8>(10 + index), 20, 30, 40);
        GXTexCoord2f32(static_cast<float>(index) * 0.5F, 0.25F);
    }
    GXEnd();

    MeleeHostGxCapturedTriangle triangle{};
    REQUIRE(melee_host_gx_captured_triangle_at(0, &triangle));
    REQUIRE(triangle.vertices[0].normal.z == 1.0F);
    REQUIRE(triangle.vertices[1].color[0] == 11);
    REQUIRE(triangle.vertices[2].texcoord[0] == 1.0F);
    REQUIRE(triangle.vertices[2].texcoord[1] == 0.25F);
}

TEST_CASE("host GX applies an affine transform to completed geometry")
{
    melee_host_gx_reset_command_log();
    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GXPosition3f32(0.0F, 0.0F, 0.0F);
    GXNormal3f32(1.0F, 1.0F, 0.0F);
    GXPosition3f32(1.0F, 0.0F, 0.0F);
    GXNormal3f32(1.0F, 1.0F, 0.0F);
    GXPosition3f32(0.0F, 1.0F, 0.0F);
    GXNormal3f32(1.0F, 1.0F, 0.0F);
    GXEnd();

    MeleeHostGxAffineTransform transform{ {
        { 2.0F, 0.0F, 0.0F, 10.0F },
        { 0.0F, 3.0F, 0.0F, 20.0F },
        { 0.0F, 0.0F, 4.0F, 30.0F },
    } };
    melee_host_gx_transform_vertices(0, 3, &transform);

    MeleeHostGxCapturedTriangle triangle{};
    REQUIRE(melee_host_gx_captured_triangle_at(0, &triangle));
    REQUIRE(triangle.vertices[0].position.x == 10.0F);
    REQUIRE(triangle.vertices[1].position.x == 12.0F);
    REQUIRE(triangle.vertices[2].position.y == 23.0F);
    MeleeHostGxTriangle positions{};
    REQUIRE(melee_host_gx_triangle_at(0, &positions));
    REQUIRE(positions.vertices[2].y == 23.0F);
    MeleeHostGxCapturedVertex vertex{};
    REQUIRE(melee_host_gx_captured_vertex_at(0, &vertex));
    REQUIRE(std::fabs(vertex.normal.x - 3.0F / std::sqrt(13.0F)) < 0.0001F);
    REQUIRE(std::fabs(vertex.normal.y - 2.0F / std::sqrt(13.0F)) < 0.0001F);
}

TEST_CASE("host GX modulates captured vertex colors with a material")
{
    melee_host_gx_reset_command_log();
    GXBegin(GX_POINTS, GX_VTXFMT0, 1);
    GXPosition3f32(0.0F, 0.0F, 0.0F);
    GXColor4u8(128, 255, 64, 255);
    GXEnd();
    const mh_u8 diffuse[4]{ 128, 64, 255, 128 };
    melee_host_gx_apply_material(0, 1, diffuse, MELEE_HOST_GX_NO_TEXTURE,
                                 1U << 30U);

    MeleeHostGxCapturedVertex vertex{};
    REQUIRE(melee_host_gx_captured_vertex_at(0, &vertex));
    REQUIRE(vertex.color[0] == 64);
    REQUIRE(vertex.color[1] == 64);
    REQUIRE(vertex.color[2] == 64);
    REQUIRE(vertex.color[3] == 128);
    REQUIRE(vertex.render_mode == (1U << 30U));
}

TEST_CASE("host GX resolves indexed big-endian VCD and VAT attributes")
{
    const std::array<u8, 18> positions{
        0xFF, 0xFC, 0xFF, 0xF8, 0x00, 0x00,
        0x00, 0x04, 0xFF, 0xF8, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
    };
    const std::array<u8, 9> normals{
        0x00, 0x00, 0x7F,
        0x00, 0x00, 0x7F,
        0x00, 0x00, 0x7F,
    };
    const std::array<u8, 12> colors{
        10, 20, 30, 40,
        50, 60, 70, 80,
        90, 100, 110, 120,
    };
    const std::array<u8, 12> texcoords{
        0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x01, 0x00,
    };

    const GXVtxDescList descriptors[]{
        { GX_VA_POS, GX_INDEX16 },
        { GX_VA_NRM, GX_INDEX8 },
        { GX_VA_CLR0, GX_INDEX8 },
        { GX_VA_TEX0, GX_INDEX16 },
        { GX_VA_NULL, GX_NONE },
    };
    const GXVtxAttrFmtList formats[]{
        { GX_VA_POS, GX_POS_XYZ, GX_S16, 2 },
        { GX_VA_NRM, GX_NRM_XYZ, GX_S8, 7 },
        { GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
        { GX_VA_TEX0, GX_TEX_ST, GX_U16, 8 },
        { GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
    };

    melee_host_gx_reset_command_log();
    GXClearVtxDesc();
    GXSetVtxDescv(descriptors);
    GXSetVtxAttrFmtv(GX_VTXFMT3, formats);
    GXSetArray(GX_VA_POS, positions.data(), 6);
    GXSetArray(GX_VA_NRM, normals.data(), 3);
    GXSetArray(GX_VA_CLR0, colors.data(), 4);
    GXSetArray(GX_VA_TEX0, texcoords.data(), 4);

    GXBegin(GX_TRIANGLES, GX_VTXFMT3, 3);
    for (u16 index = 0; index < 3; ++index) {
        GXPosition1x16(index);
        GXNormal1x8(static_cast<u8>(index));
        GXColor1x8(static_cast<u8>(index));
        GXTexCoord1x16(index);
    }
    GXEnd();

    REQUIRE(melee_host_gx_command_count() == 14);
    REQUIRE(melee_host_gx_captured_vertex_count() == 3);
    REQUIRE(melee_host_gx_triangle_count() == 1);

    MeleeHostGxCapturedVertex vertex{};
    REQUIRE(melee_host_gx_captured_vertex_at(0, &vertex));
    REQUIRE(vertex.attributes ==
            (MELEE_HOST_GX_VERTEX_POSITION | MELEE_HOST_GX_VERTEX_NORMAL |
             MELEE_HOST_GX_VERTEX_COLOR | MELEE_HOST_GX_VERTEX_TEXCOORD));
    REQUIRE(vertex.position.x == -1.0F);
    REQUIRE(vertex.position.y == -2.0F);
    REQUIRE(std::fabs(vertex.normal.z - 127.0F / 128.0F) < 0.0001F);
    REQUIRE(vertex.color[0] == 10);
    REQUIRE(vertex.color[3] == 40);

    REQUIRE(melee_host_gx_captured_vertex_at(2, &vertex));
    REQUIRE(vertex.position.y == 2.0F);
    REQUIRE(vertex.color[2] == 110);
    REQUIRE(vertex.texcoord[0] == 0.5F);
    REQUIRE(vertex.texcoord[1] == 1.0F);

    MeleeHostGxCommand command{};
    REQUIRE(melee_host_gx_command_at(2, &command));
    REQUIRE(command.type == MELEE_HOST_GX_U16);
    REQUIRE(command.bits == 0);
    REQUIRE(melee_host_gx_command_at(3, &command));
    REQUIRE(command.type == MELEE_HOST_GX_U8);
}

TEST_CASE("host GX decodes packed indexed color formats")
{
    const std::array<u8, 2> rgba4{ 0xF0, 0x08 };
    const std::array<u8, 6> position{ 0, 0, 0, 0, 0, 0 };

    melee_host_gx_reset_command_log();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA4, 0);
    GXSetArray(GX_VA_POS, position.data(), 6);
    GXSetArray(GX_VA_CLR0, rgba4.data(), 2);

    GXBegin(GX_POINTS, GX_VTXFMT0, 1);
    GXPosition1x8(0);
    GXColor1x8(0);
    GXEnd();

    MeleeHostGxCapturedVertex vertex{};
    REQUIRE(melee_host_gx_captured_vertex_at(0, &vertex));
    REQUIRE(vertex.color[0] == 255);
    REQUIRE(vertex.color[1] == 0);
    REQUIRE(vertex.color[2] == 0);
    REQUIRE(vertex.color[3] == 136);
}

TEST_CASE("host GX executes an indexed PObj-style display list")
{
    const std::array<u8, 18> positions{
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x01, 0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    };
    alignas(32) std::array<u8, 32> display_list{};
    display_list[0] = static_cast<u8>(static_cast<u8>(GX_TRIANGLES) |
                                      static_cast<u8>(GX_VTXFMT2));
    display_list[1] = 0;
    display_list[2] = 3;
    display_list[3] = 0;
    display_list[4] = 1;
    display_list[5] = 2;

    melee_host_gx_reset_command_log();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GXSetArray(GX_VA_POS, positions.data(), 6);
    GXCallDisplayList(display_list.data(), display_list.size());

    REQUIRE(melee_host_gx_display_list_error_count() == 0);
    REQUIRE(melee_host_gx_captured_vertex_count() == 3);
    REQUIRE(melee_host_gx_triangle_count() == 1);
    MeleeHostGxTriangle triangle{};
    REQUIRE(melee_host_gx_triangle_at(0, &triangle));
    REQUIRE(triangle.vertices[0].x == -1.0F);
    REQUIRE(triangle.vertices[1].x == 1.0F);
    REQUIRE(triangle.vertices[2].y == 1.0F);
}

TEST_CASE("host GX rejects a truncated display list without over-reading")
{
    const std::array<u8, 6> positions{ 0, 0, 0, 0, 0, 0 };
    std::array<u8, 4> display_list{
        static_cast<u8>(static_cast<u8>(GX_TRIANGLES) |
                        static_cast<u8>(GX_VTXFMT0)),
        0, 3, 0
    };

    melee_host_gx_reset_command_log();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GXSetArray(GX_VA_POS, positions.data(), 6);
    GXCallDisplayList(display_list.data(), display_list.size());

    REQUIRE(melee_host_gx_display_list_error_count() == 1);
    REQUIRE(melee_host_gx_triangle_count() == 0);
}

TEST_CASE("host GX rejects an indexed vertex outside a bounded array")
{
    const std::array<u8, 6> positions{ 0, 1, 0, 2, 0, 3 };

    melee_host_gx_reset_command_log();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    melee_host_gx_set_array_bounded(GX_VA_POS, positions.data(),
                                    positions.size(), 6);

    GXBegin(GX_POINTS, GX_VTXFMT0, 1);
    GXPosition1x8(1);
    GXEnd();

    REQUIRE(melee_host_gx_captured_vertex_count() == 0);
}

TEST_CASE("host GX preserves the primary normal from a direct NBT stream")
{
    alignas(32) std::array<u8, 32> display_list{};
    display_list[0] = static_cast<u8>(static_cast<u8>(GX_POINTS) |
                                      static_cast<u8>(GX_VTXFMT0));
    display_list[1] = 0;
    display_list[2] = 1;
    display_list[3] = 0x3F; // position x = 1.0F, big-endian
    display_list[4] = 0x80;
    display_list[5] = 0;
    display_list[6] = 0;
    display_list[7] = 0;
    display_list[8] = 0;
    display_list[9] = 0;
    display_list[10] = 0;
    display_list[11] = 0;
    display_list[12] = 0;
    display_list[13] = 0;
    display_list[15] = 1;  // normal
    display_list[16] = 2;
    display_list[17] = 3;
    display_list[18] = 4;  // tangent
    display_list[19] = 5;
    display_list[20] = 6;
    display_list[21] = 7;  // binormal
    display_list[22] = 8;
    display_list[23] = 9;

    melee_host_gx_reset_command_log();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_NBT, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NBT, GX_NRM_NBT, GX_S8, 0);
    GXCallDisplayList(display_list.data(), display_list.size());

    MeleeHostGxCapturedVertex vertex{};
    REQUIRE(melee_host_gx_display_list_error_count() == 0);
    REQUIRE(melee_host_gx_captured_vertex_at(0, &vertex));
    REQUIRE((vertex.attributes & MELEE_HOST_GX_VERTEX_NORMAL) != 0);
    REQUIRE(vertex.normal.x == 1.0F);
    REQUIRE(vertex.normal.y == 2.0F);
    REQUIRE(vertex.normal.z == 3.0F);
    REQUIRE((vertex.attributes & MELEE_HOST_GX_VERTEX_TANGENT) != 0);
    REQUIRE((vertex.attributes & MELEE_HOST_GX_VERTEX_BINORMAL) != 0);
    REQUIRE(vertex.tangent.x == 4.0F);
    REQUIRE(vertex.tangent.y == 5.0F);
    REQUIRE(vertex.tangent.z == 6.0F);
    REQUIRE(vertex.binormal.x == 7.0F);
    REQUIRE(vertex.binormal.y == 8.0F);
    REQUIRE(vertex.binormal.z == 9.0F);
}

TEST_CASE("host GX resolves the three indexed NBT3 vectors")
{
    const std::array<u8, 12> positions{
        0x3F, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    const std::array<u8, 9> nbt{
        1, 2, 3,
        4, 5, 6,
        7, 8, 9,
    };
    alignas(32) std::array<u8, 32> display_list{};
    display_list[0] = static_cast<u8>(static_cast<u8>(GX_POINTS) |
                                      static_cast<u8>(GX_VTXFMT0));
    display_list[1] = 0;
    display_list[2] = 1;
    display_list[3] = 0;
    display_list[4] = 0;
    display_list[5] = 1;
    display_list[6] = 2;

    melee_host_gx_reset_command_log();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxDesc(GX_VA_NBT, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NBT, GX_NRM_NBT3, GX_S8, 0);
    melee_host_gx_set_array_bounded(GX_VA_POS, positions.data(),
                                    positions.size(), 12);
    melee_host_gx_set_array_bounded(GX_VA_NBT, nbt.data(), nbt.size(), 3);
    GXCallDisplayList(display_list.data(), display_list.size());

    MeleeHostGxCapturedVertex vertex{};
    REQUIRE(melee_host_gx_display_list_error_count() == 0);
    REQUIRE(melee_host_gx_captured_vertex_at(0, &vertex));
    REQUIRE(vertex.normal.z == 3.0F);
    REQUIRE(vertex.tangent.y == 5.0F);
    REQUIRE(vertex.binormal.x == 7.0F);
}
