#ifndef MELEE_HOST_GX_VIEW_HPP
#define MELEE_HOST_GX_VIEW_HPP

#include <melee_host/gx.h>

#include <array>

namespace melee::gx {

/* A 4x4 matrix in column-major order, as OpenGL reads it. */
using ClipMatrix = std::array<float, 16>;

/* The clip-space matrix for positions already in GX eye space, from the
 * projection a draw ran under.  GX keeps a projection as six numbers and puts
 * clip depth in [-w, 0], with the near plane at -w; OpenGL wants [-w, w].
 * Depth is remapped to 2z + w, so near and far land at -1 and +1 and the
 * viewport's depth range still spans them in the same order. */
[[nodiscard]] ClipMatrix clip_matrix(const MeleeHostGxViewState& view);

/* A rectangle in window pixels with its origin at the bottom left, as
 * glViewport and glScissor take it. */
struct WindowRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

/* Maps a rectangle in GX framebuffer pixels, origin at the top left of a
 * framebuffer_width by framebuffer_height buffer, onto a target of
 * target_width by target_height pixels.  A framebuffer without a size maps to
 * the whole target. */
[[nodiscard]] WindowRect window_rect(float left, float top, float width,
                                     float height, int framebuffer_width,
                                     int framebuffer_height, int target_width,
                                     int target_height);

} // namespace melee::gx

#endif
