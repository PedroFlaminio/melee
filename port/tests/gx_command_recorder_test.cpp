#include "test.hpp"

#include <bit>

extern "C" {
#include <dolphin/gx/GXVert.h>
#include <dolphin/gx/GXGeometry.h>
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
