#include "test.hpp"

#include "gx/view.hpp"

#include <cmath>

namespace {

struct ClipPoint {
    float x;
    float y;
    float z;
    float w;
};

ClipPoint transform(const melee::gx::ClipMatrix& matrix, float x, float y,
                    float z)
{
    return {
        matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12],
        matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13],
        matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14],
        matrix[3] * x + matrix[7] * y + matrix[11] * z + matrix[15],
    };
}

bool close_to(float value, float expected)
{
    return std::fabs(value - expected) < 1.0e-4F;
}

constexpr float kNear = 1.0F;
constexpr float kFar = 101.0F;

} // namespace

TEST_CASE("a GX perspective reaches GL clip space with near at -1 and far at 1")
{
    // The six numbers GXSetProjection keeps for what MTXPerspective builds.
    MeleeHostGxViewState view{};
    view.projection_type = 0; // GX_PERSPECTIVE
    view.projection[0] = 1.5F;
    view.projection[2] = 2.0F;
    view.projection[4] = -kNear / (kFar - kNear);
    view.projection[5] = -(kFar * kNear) / (kFar - kNear);
    const melee::gx::ClipMatrix matrix = melee::gx::clip_matrix(view);

    const ClipPoint at_near = transform(matrix, 0.5F, 0.25F, -kNear);
    REQUIRE(close_to(at_near.w, kNear));
    REQUIRE(close_to(at_near.x / at_near.w, 0.75F));
    REQUIRE(close_to(at_near.y / at_near.w, 0.5F));
    REQUIRE(close_to(at_near.z / at_near.w, -1.0F));

    const ClipPoint at_far = transform(matrix, 0.0F, 0.0F, -kFar);
    REQUIRE(close_to(at_far.z / at_far.w, 1.0F));
}

TEST_CASE("a GX orthographic projection keeps w and spans the same depth")
{
    // MTXOrtho over a 640 by 480 screen, top at 480.
    MeleeHostGxViewState view{};
    view.projection_type = 1; // GX_ORTHOGRAPHIC
    view.projection[0] = 2.0F / 640.0F;
    view.projection[1] = -1.0F;
    view.projection[2] = 2.0F / 480.0F;
    view.projection[3] = -1.0F;
    view.projection[4] = -1.0F / (kFar - kNear);
    view.projection[5] = -kFar / (kFar - kNear);
    const melee::gx::ClipMatrix matrix = melee::gx::clip_matrix(view);

    const ClipPoint center = transform(matrix, 320.0F, 240.0F, -kNear);
    REQUIRE(close_to(center.w, 1.0F));
    REQUIRE(close_to(center.x, 0.0F));
    REQUIRE(close_to(center.y, 0.0F));
    REQUIRE(close_to(center.z, -1.0F));
    REQUIRE(close_to(transform(matrix, 640.0F, 480.0F, -kFar).z, 1.0F));
    REQUIRE(close_to(transform(matrix, 640.0F, 480.0F, -kFar).x, 1.0F));
}

TEST_CASE("a GX framebuffer rectangle maps to a bottom-left window rectangle")
{
    const melee::gx::WindowRect full =
        melee::gx::window_rect(0.0F, 0.0F, 640.0F, 480.0F, 640, 480, 960, 720);
    REQUIRE(full.x == 0);
    REQUIRE(full.y == 0);
    REQUIRE(full.width == 960);
    REQUIRE(full.height == 720);

    // 50 pixels from the top of a 480-line buffer is 330 lines above the
    // bottom of this 100-line rectangle, at one and a half times the size.
    const melee::gx::WindowRect inner = melee::gx::window_rect(
        100.0F, 50.0F, 200.0F, 100.0F, 640, 480, 960, 720);
    REQUIRE(inner.x == 150);
    REQUIRE(inner.y == 495);
    REQUIRE(inner.width == 300);
    REQUIRE(inner.height == 150);

    const melee::gx::WindowRect unsized =
        melee::gx::window_rect(10.0F, 10.0F, 1.0F, 1.0F, 0, 0, 320, 240);
    REQUIRE(unsized.width == 320);
    REQUIRE(unsized.height == 240);
}
