#include "test.hpp"

#include <array>
#include <bit>
#include <cmath>

extern "C" {
#include <dolphin/gx/GXVert.h>
#include <dolphin/gx/GXGeometry.h>
#include <dolphin/gx/GXDispList.h>
#include <dolphin/gx/GXPixel.h>
#include <dolphin/gx/GXTev.h>
#include <dolphin/gx/GXCull.h>
#include <dolphin/gx/GXTexture.h>
#include <dolphin/gx/GXTransform.h>
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

TEST_CASE("a captured vertex is transformed by the position matrix in effect")
{
    melee_host_gx_state_reset();
    melee_host_gx_reset_command_log();

    // A translation loaded where GX_PNMTX0 lives, which is where the HSD
    // display path puts a rigid model's matrix.
    f32 translate[3][4] = {
        { 1.0F, 0.0F, 0.0F, 10.0F },
        { 0.0F, 1.0F, 0.0F, 20.0F },
        { 0.0F, 0.0F, 1.0F, 30.0F },
    };
    GXLoadPosMtxImm(translate, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);

    static const u8 positions[12] = { 0x3F, 0x80, 0, 0, 0x40, 0,
                                      0,    0,    0x40, 0x40, 0, 0 };
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, positions, 12);

    std::array<u8, 32> list{};
    list[0] = 0x90;
    list[2] = 0x03;
    GXCallDisplayList(list.data(), static_cast<u32>(list.size()));

    REQUIRE(melee_host_gx_triangle_count() == 1);
    MeleeHostGxTriangle triangle{};
    REQUIRE(melee_host_gx_triangle_at(0, &triangle));
    // (1, 2, 3) moved by the loaded matrix.
    REQUIRE(std::fabs(triangle.vertices[0].x - 11.0F) < 1.0e-5F);
    REQUIRE(std::fabs(triangle.vertices[0].y - 22.0F) < 1.0e-5F);
    REQUIRE(std::fabs(triangle.vertices[0].z - 33.0F) < 1.0e-5F);
}

TEST_CASE("a matrix index moves one vertex by its own joint matrix")
{
    // This is the envelope-skinning case: the PObj loads a matrix per joint and
    // names one per vertex, so a single transform applied to the whole draw
    // could not reproduce it.
    melee_host_gx_state_reset();
    melee_host_gx_reset_command_log();

    f32 first[3][4] = {
        { 1.0F, 0.0F, 0.0F, 100.0F },
        { 0.0F, 1.0F, 0.0F, 0.0F },
        { 0.0F, 0.0F, 1.0F, 0.0F },
    };
    f32 second[3][4] = {
        { 1.0F, 0.0F, 0.0F, 0.0F },
        { 0.0F, 1.0F, 0.0F, 200.0F },
        { 0.0F, 0.0F, 1.0F, 0.0F },
    };
    GXLoadPosMtxImm(first, GX_PNMTX0);
    GXLoadPosMtxImm(second, GX_PNMTX1);
    GXSetCurrentMtx(GX_PNMTX0);

    static const u8 positions[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_PNMTXIDX, GX_DIRECT);
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, positions, 12);

    // Three vertices at the origin: the first two name GX_PNMTX0 and
    // GX_PNMTX1, the third repeats the second.
    std::array<u8, 32> list{};
    list[0] = 0x90;
    list[2] = 0x03;
    list[3] = GX_PNMTX0;
    list[4] = 0;
    list[5] = GX_PNMTX1;
    list[6] = 0;
    list[7] = GX_PNMTX1;
    list[8] = 0;
    GXCallDisplayList(list.data(), static_cast<u32>(list.size()));

    REQUIRE(melee_host_gx_display_list_error_count() == 0);
    REQUIRE(melee_host_gx_triangle_count() == 1);
    MeleeHostGxTriangle triangle{};
    REQUIRE(melee_host_gx_triangle_at(0, &triangle));
    REQUIRE(std::fabs(triangle.vertices[0].x - 100.0F) < 1.0e-5F);
    REQUIRE(std::fabs(triangle.vertices[0].y) < 1.0e-5F);
    REQUIRE(std::fabs(triangle.vertices[1].x) < 1.0e-5F);
    REQUIRE(std::fabs(triangle.vertices[1].y - 200.0F) < 1.0e-5F);
    REQUIRE(std::fabs(triangle.vertices[2].y - 200.0F) < 1.0e-5F);
}

TEST_CASE("geometry drawn with no matrix loaded is left where it was")
{
    // Matrix memory resets to zeros rather than identity, so transforming by
    // an untouched row would collapse every vertex onto the origin.
    melee_host_gx_state_reset();
    melee_host_gx_reset_command_log();

    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GXPosition3f32(-1.0F, -1.0F, 0.0F);
    GXPosition3f32(1.0F, -1.0F, 0.0F);
    GXPosition3f32(0.0F, 1.0F, 0.0F);
    GXEnd();

    MeleeHostGxTriangle triangle{};
    REQUIRE(melee_host_gx_triangle_at(0, &triangle));
    REQUIRE(triangle.vertices[0].x == -1.0F);
    REQUIRE(triangle.vertices[2].y == 1.0F);
}

TEST_CASE("a captured vertex names the texture bound when its draw began")
{
    melee_host_gx_state_reset();
    melee_host_gx_reset_command_log();

    static u8 image[32] ATTRIBUTE_ALIGN(32) = { 0 };
    GXTexObj texture;
    GXInitTexObj(&texture, image, 8, 8, GX_TF_I8, GX_REPEAT, GX_CLAMP,
                 GX_FALSE);
    GXLoadTexObj(&texture, GX_TEXMAP0);

    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GXPosition3f32(0.0F, 0.0F, 0.0F);
    GXPosition3f32(1.0F, 0.0F, 0.0F);
    GXPosition3f32(0.0F, 1.0F, 0.0F);
    GXEnd();

    REQUIRE(melee_host_gx_captured_texture_count() == 1);
    MeleeHostGxTextureDesc desc{};
    REQUIRE(melee_host_gx_captured_texture_at(0, &desc));
    REQUIRE(desc.image == image);
    REQUIRE(desc.width == 8);
    REQUIRE(desc.wrap_s == GX_REPEAT);
    REQUIRE(desc.wrap_t == GX_CLAMP);

    MeleeHostGxCapturedVertex vertex{};
    REQUIRE(melee_host_gx_captured_vertex_at(0, &vertex));
    REQUIRE((vertex.attributes & MELEE_HOST_GX_VERTEX_TEXTURE_IMAGE) != 0);
    REQUIRE(vertex.texture_image == 0);

    // An untextured draw must not inherit the binding by leaving the id at a
    // value that is also a valid index.
    melee_host_gx_reset_command_log();
    melee_host_gx_state_reset();
    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GXPosition3f32(0.0F, 0.0F, 0.0F);
    GXPosition3f32(1.0F, 0.0F, 0.0F);
    GXPosition3f32(0.0F, 1.0F, 0.0F);
    GXEnd();
    REQUIRE(melee_host_gx_captured_texture_count() == 0);
    REQUIRE(melee_host_gx_captured_vertex_at(0, &vertex));
    REQUIRE((vertex.attributes & MELEE_HOST_GX_VERTEX_TEXTURE_IMAGE) == 0);
}

TEST_CASE("draws are grouped by the pixel state they ran under")
{
    melee_host_gx_state_reset();
    melee_host_gx_reset_command_log();

    const auto draw = []() {
        GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
        GXPosition3f32(0.0F, 0.0F, 0.0F);
        GXPosition3f32(1.0F, 0.0F, 0.0F);
        GXPosition3f32(0.0F, 1.0F, 0.0F);
        GXEnd();
    };

    // An opaque draw, then a translucent one, which is the pair HSD's render
    // modes produce.
    GXSetCullMode(GX_CULL_BACK);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    draw();

    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
                   GX_LO_CLEAR);
    draw();

    // A third draw repeating the first state must reuse its entry rather than
    // add another.
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    draw();

    REQUIRE(melee_host_gx_captured_draw_state_count() == 2);
    REQUIRE(melee_host_gx_triangle_count() == 3);

    MeleeHostGxDrawState opaque{};
    MeleeHostGxDrawState blended{};
    REQUIRE(melee_host_gx_captured_draw_state_at(0, &opaque));
    REQUIRE(melee_host_gx_captured_draw_state_at(1, &blended));
    REQUIRE(opaque.cull_mode == GX_CULL_BACK);
    REQUIRE(opaque.blend_mode == GX_BM_NONE);
    REQUIRE(opaque.z_update_enable);
    REQUIRE(opaque.z_func == GX_LEQUAL);
    REQUIRE(blended.blend_mode == GX_BM_BLEND);
    REQUIRE(blended.blend_src_factor == GX_BL_SRCALPHA);
    REQUIRE(blended.blend_dst_factor == GX_BL_INVSRCALPHA);
    REQUIRE(!blended.z_update_enable);

    MeleeHostGxCapturedTriangle triangle{};
    REQUIRE(melee_host_gx_captured_triangle_at(0, &triangle));
    REQUIRE(triangle.vertices[0].draw_state == 0);
    REQUIRE(melee_host_gx_captured_triangle_at(1, &triangle));
    REQUIRE(triangle.vertices[0].draw_state == 1);
    REQUIRE(melee_host_gx_captured_triangle_at(2, &triangle));
    REQUIRE(triangle.vertices[0].draw_state == 0);
}

TEST_CASE("a captured state carries the alpha compare the draw ran under")
{
    melee_host_gx_state_reset();
    melee_host_gx_reset_command_log();

    // The shape a cutout material uses: keep what passes one threshold, with
    // the second comparison left open.
    GXSetAlphaCompare(GX_GREATER, 128, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_FALSE);
    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GXPosition3f32(0.0F, 0.0F, 0.0F);
    GXPosition3f32(1.0F, 0.0F, 0.0F);
    GXPosition3f32(0.0F, 1.0F, 0.0F);
    GXEnd();

    MeleeHostGxDrawState state{};
    REQUIRE(melee_host_gx_captured_draw_state_count() == 1);
    REQUIRE(melee_host_gx_captured_draw_state_at(0, &state));
    REQUIRE(state.alpha_compare_0 == GX_GREATER);
    REQUIRE(state.alpha_ref_0 == 128);
    REQUIRE(state.alpha_op == GX_AOP_AND);
    REQUIRE(state.alpha_compare_1 == GX_ALWAYS);
    REQUIRE(state.color_update_enable);
    REQUIRE(!state.alpha_update_enable);
}

TEST_CASE("the alpha test reduces to the comparison that carries information")
{
    MeleeHostGxDrawState state{};
    mh_u32 compare = 0;
    mh_u8 reference = 0;

    // Both sides open: no test at all.
    state.alpha_compare_0 = GX_ALWAYS;
    state.alpha_compare_1 = GX_ALWAYS;
    state.alpha_op = GX_AOP_AND;
    REQUIRE(!melee_host_gx_resolve_alpha_test(&state, &compare, &reference));

    // The pair the game writes for a cutout: the same comparison twice.
    state.alpha_compare_0 = GX_GREATER;
    state.alpha_ref_0 = 0;
    state.alpha_compare_1 = GX_GREATER;
    state.alpha_ref_1 = 0;
    REQUIRE(melee_host_gx_resolve_alpha_test(&state, &compare, &reference));
    REQUIRE(compare == GX_GREATER);
    REQUIRE(reference == 0);

    // A band whose upper half is open, because alpha never exceeds 255.
    state.alpha_compare_0 = GX_GEQUAL;
    state.alpha_ref_0 = 102;
    state.alpha_compare_1 = GX_LEQUAL;
    state.alpha_ref_1 = 255;
    REQUIRE(melee_host_gx_resolve_alpha_test(&state, &compare, &reference));
    REQUIRE(compare == GX_GEQUAL);
    REQUIRE(reference == 102);

    // The same band written the other way round.
    state.alpha_compare_0 = GX_LEQUAL;
    state.alpha_ref_0 = 255;
    state.alpha_compare_1 = GX_GEQUAL;
    state.alpha_ref_1 = 102;
    REQUIRE(melee_host_gx_resolve_alpha_test(&state, &compare, &reference));
    REQUIRE(compare == GX_GEQUAL);
    REQUIRE(reference == 102);

    // A lower bound of zero is open too.
    state.alpha_compare_0 = GX_GEQUAL;
    state.alpha_ref_0 = 0;
    state.alpha_compare_1 = GX_LESS;
    state.alpha_ref_1 = 64;
    REQUIRE(melee_host_gx_resolve_alpha_test(&state, &compare, &reference));
    REQUIRE(compare == GX_LESS);
    REQUIRE(reference == 64);
}

namespace {

/* Builds the single-stage program shape HSD's expression compiler emits, with
 * the arithmetic left at its neutral settings. */
MeleeHostGxTevState one_stage(mh_u32 color_a, mh_u32 color_b, mh_u32 color_c,
                              mh_u32 color_d, mh_u32 alpha_a, mh_u32 alpha_b,
                              mh_u32 alpha_c, mh_u32 alpha_d)
{
    MeleeHostGxTevState tev{};
    tev.stage_count = 1;
    tev.texcoord_gen_count = 1;
    tev.channel_count = 1;
    MeleeHostGxTevStage& stage = tev.stages[0];
    stage.mode = MELEE_HOST_GX_TEV_MODE_CUSTOM;
    stage.color_input[0] = color_a;
    stage.color_input[1] = color_b;
    stage.color_input[2] = color_c;
    stage.color_input[3] = color_d;
    stage.alpha_input[0] = alpha_a;
    stage.alpha_input[1] = alpha_b;
    stage.alpha_input[2] = alpha_c;
    stage.alpha_input[3] = alpha_d;
    stage.color_op = GX_TEV_ADD;
    stage.alpha_op = GX_TEV_ADD;
    stage.color_out_reg = GX_TEVPREV;
    stage.alpha_out_reg = GX_TEVPREV;
    return tev;
}

} // namespace

TEST_CASE("the material program the game emits most reads as texture x colour")
{
    // GX computes d + ((1 - c) * a + c * b), so a = d = zero makes the stage
    // b scaled by c: the texture scaled by the rasterized colour.  The alpha
    // side multiplies the rasterized alpha by the material alpha HSD leaves in
    // the first TEV register.  This is the program 651 of the disc's
    // single-stage materials use.
    MeleeHostGxTevState tev =
        one_stage(GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO, GX_CA_ZERO,
                  GX_CA_A0, GX_CA_RASA, GX_CA_ZERO);
    tev.registers[GX_TEVREG0][3] = 128;

    MeleeHostGxResolvedShading shading{};
    REQUIRE(melee_host_gx_resolve_shading(&tev, &shading));
    REQUIRE(shading.kind == MELEE_HOST_GX_SHADING_TEXTURE_TIMES_COLOR);
    REQUIRE(shading.constant_alpha == 128);
    REQUIRE(shading.uses_raster_alpha);
}

TEST_CASE("an untextured material reads as its constant colour times colour")
{
    MeleeHostGxTevState tev =
        one_stage(GX_CC_ZERO, GX_CC_KONST, GX_CC_RASC, GX_CC_ZERO, GX_CA_ZERO,
                  GX_CA_A0, GX_CA_RASA, GX_CA_ZERO);
    tev.stages[0].konst_color_select = GX_TEV_KCSEL_K1;
    tev.konst_colors[1][0] = 10;
    tev.konst_colors[1][1] = 20;
    tev.konst_colors[1][2] = 30;
    tev.konst_colors[1][3] = 40;
    tev.registers[GX_TEVREG0][3] = 255;

    MeleeHostGxResolvedShading shading{};
    REQUIRE(melee_host_gx_resolve_shading(&tev, &shading));
    REQUIRE(shading.kind == MELEE_HOST_GX_SHADING_KONST_TIMES_COLOR);
    REQUIRE(shading.konst_color[0] == 10);
    REQUIRE(shading.konst_color[3] == 40);

    // A selector naming a single component or a constant fraction is not a
    // whole colour, so it cannot be read off this way.
    tev.stages[0].konst_color_select = GX_TEV_KCSEL_1_2;
    REQUIRE(melee_host_gx_resolve_shading(&tev, &shading));
    REQUIRE(shading.kind == MELEE_HOST_GX_SHADING_APPROXIMATED);
}

TEST_CASE("a program the host cannot read off says so rather than guessing")
{
    // Several stages combine results the host does not track separately, and
    // the reduction must not pretend otherwise.
    MeleeHostGxTevState tev =
        one_stage(GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO, GX_CA_ZERO,
                  GX_CA_A0, GX_CA_RASA, GX_CA_ZERO);
    tev.stage_count = 3;
    MeleeHostGxResolvedShading shading{};
    REQUIRE(melee_host_gx_resolve_shading(&tev, &shading));
    REQUIRE(shading.kind == MELEE_HOST_GX_SHADING_APPROXIMATED);

    // A single stage whose arithmetic is not neutral is out of reach too: a
    // scale or a bias changes the result.
    tev = one_stage(GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO,
                    GX_CA_ZERO, GX_CA_A0, GX_CA_RASA, GX_CA_ZERO);
    tev.stages[0].color_scale = GX_CS_SCALE_2;
    REQUIRE(melee_host_gx_resolve_shading(&tev, &shading));
    REQUIRE(shading.kind == MELEE_HOST_GX_SHADING_APPROXIMATED);

    // So is a stage that writes somewhere other than the final register.
    tev = one_stage(GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO,
                    GX_CA_ZERO, GX_CA_A0, GX_CA_RASA, GX_CA_ZERO);
    tev.stages[0].color_out_reg = GX_TEVREG1;
    REQUIRE(melee_host_gx_resolve_shading(&tev, &shading));
    REQUIRE(shading.kind == MELEE_HOST_GX_SHADING_APPROXIMATED);
}

TEST_CASE("a stage that only passes a value through reads as that value")
{
    MeleeHostGxTevState tev =
        one_stage(GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC, GX_CA_ZERO,
                  GX_CA_A0, GX_CA_RASA, GX_CA_ZERO);
    MeleeHostGxResolvedShading shading{};
    REQUIRE(melee_host_gx_resolve_shading(&tev, &shading));
    REQUIRE(shading.kind == MELEE_HOST_GX_SHADING_TEXTURE);

    tev = one_stage(GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC,
                    GX_CA_ZERO, GX_CA_A0, GX_CA_RASA, GX_CA_ZERO);
    REQUIRE(melee_host_gx_resolve_shading(&tev, &shading));
    REQUIRE(shading.kind == MELEE_HOST_GX_SHADING_COLOR);
}

TEST_CASE("draws under different material programs are captured separately")
{
    melee_host_gx_state_reset();
    melee_host_gx_reset_command_log();

    const auto draw = []() {
        GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
        GXPosition3f32(0.0F, 0.0F, 0.0F);
        GXPosition3f32(1.0F, 0.0F, 0.0F);
        GXPosition3f32(0.0F, 1.0F, 0.0F);
        GXEnd();
    };

    GXSetNumTevStages(1);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC,
                    GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
                    GX_TRUE, GX_TEVPREV);
    draw();

    // The same stage with a different konst colour is a different program.
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_KONST, GX_CC_RASC,
                    GX_CC_ZERO);
    draw();
    // And repeating the first one must reuse its entry.
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC,
                    GX_CC_ZERO);
    draw();

    REQUIRE(melee_host_gx_captured_tev_state_count() == 2);
    MeleeHostGxCapturedTriangle triangle{};
    REQUIRE(melee_host_gx_captured_triangle_at(0, &triangle));
    REQUIRE(triangle.vertices[0].tev_state == 0);
    REQUIRE(melee_host_gx_captured_triangle_at(1, &triangle));
    REQUIRE(triangle.vertices[0].tev_state == 1);
    REQUIRE(melee_host_gx_captured_triangle_at(2, &triangle));
    REQUIRE(triangle.vertices[0].tev_state == 0);
}
