#ifndef MELEE_HOST_GX_TEV_HPP
#define MELEE_HOST_GX_TEV_HPP

#include <melee_host/gx.h>

#include <array>
#include <cstdint>
#include <string>

namespace melee::gx {

/* What reaches the TEV for one fragment: the two rasterized colours and the
 * texel each texture map yields there, as 8-bit components.  Sampling is not
 * part of this model; the caller has already filtered the textures. */
struct TevFragmentInputs {
    std::array<std::array<int, 4>, 2> raster{};
    std::array<std::array<int, 4>, MELEE_HOST_GX_MAX_TEXMAP> texmap{};
};

/* Runs a captured TEV program on one fragment the way the hardware combines
 * it: integer arithmetic on 8-bit inputs and 11-bit signed registers, the
 * lerp that stretches c from 255 to 256, the per-operation rounding, and the
 * comparison modes.  Returns the RGBA the fragment leaves the last stage
 * with, before the alpha test.
 *
 * This is the reference the shader generator below is held to.  The formulas
 * follow the software rasterizer in Dolphin, which was derived from hardware
 * tests; nothing here is taken from its code. */
std::array<int, 4> evaluate_tev(const MeleeHostGxTevState& tev,
                                const TevFragmentInputs& inputs);

/* Both alpha comparisons and the logic that combines them, against the alpha
 * the TEV produced.  No reduction: every GXAlphaOp is evaluated as written. */
bool alpha_test_passes(const MeleeHostGxDrawState& state, int alpha);

/* Parts of a program the per-fragment path does not reproduce.  A program
 * with none of these is evaluated exactly, up to texture filtering. */
enum TevUnmodelled : std::uint32_t {
    kTevUnmodelledNone = 0,
    /* A stage samples through a GX_TG_BUMPn coordinate, which needs the light
     * direction projected on tangent and binormal. */
    kTevUnmodelledBumpTexGen = 1U << 0,
    /* An input selector the hardware does not define. */
    kTevUnmodelledArgument = 1U << 1,
};

std::uint32_t tev_unmodelled_features(const MeleeHostGxTevState& tev);
std::string describe_tev_unmodelled(std::uint32_t features);

/* GLSL implementing the same program.  The structure of a program becomes
 * code and its constants become uniforms, so two draws that differ only in
 * register or konst values share one shader, and the source text itself is a
 * sufficient cache key.
 *
 * Contract with the consumer:
 *   attributes  0 a_position vec3, 1 a_color0 vec4, 2 a_color1 vec4,
 *               3..10 a_texgen vec3[8] (s, t, q after texgen)
 *   uniforms    u_mvp mat4; u_texmap sampler2D[8];
 *               u_register ivec4[4] (index 1..3 are C0..C2);
 *               u_konst ivec4[4];
 *               u_alpha_test ivec4 (comp0, ref0, op, comp1);
 *               u_alpha_ref1 int
 *   output      frag_color, the final register over 255; fragments failing
 *               the alpha test are discarded. */
std::string tev_vertex_shader_source();
std::string tev_fragment_shader_source(const MeleeHostGxTevState& tev);

} // namespace melee::gx

#endif
