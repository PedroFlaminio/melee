#include "gx/view.hpp"

#include <dolphin/gx/GXEnum.h>

#include <cmath>
#include <cstddef>

namespace melee::gx {

ClipMatrix clip_matrix(const MeleeHostGxViewState& view)
{
    /* The GX matrix, row by row, as GXSetProjection's six numbers describe
     * it. */
    std::array<std::array<float, 4>, 4> rows{};
    rows[0][0] = view.projection[0];
    rows[1][1] = view.projection[2];
    rows[2][2] = view.projection[4];
    rows[2][3] = view.projection[5];
    if (view.projection_type == static_cast<mh_u32>(GX_PERSPECTIVE)) {
        rows[0][2] = view.projection[1];
        rows[1][2] = view.projection[3];
        rows[3][2] = -1.0F;
    } else {
        rows[0][3] = view.projection[1];
        rows[1][3] = view.projection[3];
        rows[3][3] = 1.0F;
    }
    for (std::size_t column = 0; column < 4; ++column) {
        rows[2][column] = 2.0F * rows[2][column] + rows[3][column];
    }

    ClipMatrix matrix{};
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            matrix[column * 4 + row] = rows[row][column];
        }
    }
    return matrix;
}

WindowRect window_rect(float left, float top, float width, float height,
                       int framebuffer_width, int framebuffer_height,
                       int target_width, int target_height)
{
    if (framebuffer_width <= 0 || framebuffer_height <= 0) {
        return { 0, 0, target_width, target_height };
    }
    const float scale_x = static_cast<float>(target_width) /
                          static_cast<float>(framebuffer_width);
    const float scale_y = static_cast<float>(target_height) /
                          static_cast<float>(framebuffer_height);
    const long x_left = std::lround(left * scale_x);
    const long x_right = std::lround((left + width) * scale_x);
    const long y_top = std::lround(top * scale_y);
    const long y_bottom = std::lround((top + height) * scale_y);
    return {
        static_cast<int>(x_left),
        static_cast<int>(static_cast<long>(target_height) - y_bottom),
        static_cast<int>(x_right - x_left),
        static_cast<int>(y_bottom - y_top),
    };
}

} // namespace melee::gx
