#include "gx/tev.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

namespace melee::gx {
namespace {

constexpr mh_u32 kColorArgZero = 15;
constexpr mh_u32 kAlphaArgZero = 7;
constexpr mh_u32 kOpSub = 1;
constexpr mh_u32 kScaleDivide2 = 3;
constexpr mh_u32 kBiasAddHalf = 1;
constexpr mh_u32 kBiasSubHalf = 2;
constexpr mh_u32 kTexGenBump0 = 2;
constexpr mh_u32 kTexGenBump7 = 9;

/* The swap tables GXInit installs.  The only GXSetTevSwapModeTable call in the
 * code the host builds installs GXInit's own SWAP0, and the recorder stops by
 * name on any other table, so these are the tables every draw uses. */
constexpr std::array<std::array<std::size_t, 4>, 4> kSwapTables{ {
    { 0, 1, 2, 3 },
    { 0, 0, 0, 3 },
    { 1, 1, 1, 3 },
    { 2, 2, 2, 3 },
} };
constexpr std::array<const char*, 4> kSwapSwizzles{ "rgba", "rrra", "ggga",
                                                     "bbba" };

/* KCSEL/KASEL 0..7 name fixed fractions rather than a konst register. */
constexpr std::array<int, 8> kKonstFractions{ 255, 223, 191, 159,
                                               128, 96,  64,  32 };

std::size_t stage_count(const MeleeHostGxTevState& tev)
{
    return std::clamp<std::size_t>(tev.stage_count, 1,
                                   MELEE_HOST_GX_MAX_TEVSTAGE);
}

std::size_t swap_table(mh_u32 selection)
{
    return selection < kSwapTables.size() ? selection : 0;
}

/* GXSetTevOrder resolves a channel id to one of the two rasterized colours or
 * to zero, the same way the SDK packs it into the order register. */
int raster_channel(mh_u32 channel)
{
    switch (channel) {
    case 0: // GX_COLOR0
    case 2: // GX_ALPHA0
    case 4: // GX_COLOR0A0
        return 0;
    case 1:
    case 3:
    case 5:
        return 1;
    default:
        return -1;
    }
}

enum class TexelSource { Sampled, White, Zero };

/* A stage without a texture map reads white, unless no texture coordinate is
 * generated at all, in which case sampling yields zero. */
TexelSource texel_source(const MeleeHostGxTevState& tev,
                         const MeleeHostGxTevStage& stage)
{
    if (tev.texcoord_gen_count == 0) {
        return TexelSource::Zero;
    }
    return stage.texmap < MELEE_HOST_GX_MAX_TEXMAP ? TexelSource::Sampled
                                                   : TexelSource::White;
}

bool is_compare(mh_u32 op)
{
    return op > kOpSub;
}

/* Registers are 11 bits wide and signed; d reads all of them. */
int sign_extend_11(int value)
{
    return ((value & 0x7FF) ^ 0x400) - 0x400;
}

int left_shift_of(mh_u32 scale)
{
    return scale == 1 ? 2 : scale == 2 ? 4 : 1;
}

int combine_regular(int a, int b, int c, int d, mh_u32 op, mh_u32 bias,
                    mh_u32 scale)
{
    const int stretched = c + (c >> 7);
    const int multiplier = left_shift_of(scale);
    int lerp = (a * (256 - stretched) + b * stretched) * multiplier;
    lerp += scale == kScaleDivide2 ? 0 : (op == kOpSub ? 127 : 128);
    lerp >>= 8;
    if (op == kOpSub) {
        lerp = -lerp;
    }
    const int bias_value =
        bias == kBiasAddHalf ? 128 : bias == kBiasSubHalf ? -128 : 0;
    const int result = (d + bias_value) * multiplier + lerp;
    return scale == kScaleDivide2 ? result >> 1 : result;
}

int clamp_output(int value, bool clamp)
{
    return clamp ? std::clamp(value, 0, 255) : std::clamp(value, -1024, 1023);
}

/* The packed forms R8, GR16 and BGR24 read the colour inputs, even for the
 * alpha combiner. */
int packed(const std::array<int, 3>& value, mh_u32 mode)
{
    switch (mode) {
    case 0:
        return value[0];
    case 1:
        return (value[1] << 8) | value[0];
    default:
        return (value[2] << 16) | (value[1] << 8) | value[0];
    }
}

bool compare_holds(int a, int b, mh_u32 op)
{
    return (op & 1U) != 0 ? a == b : a > b;
}

} // namespace

std::array<int, 4> evaluate_tev(const MeleeHostGxTevState& tev,
                                const TevFragmentInputs& inputs)
{
    std::array<std::array<int, 4>, MELEE_HOST_GX_MAX_TEVREG> reg{};
    for (std::size_t index = 1; index < reg.size(); ++index) {
        for (std::size_t channel = 0; channel < 4; ++channel) {
            reg[index][channel] = tev.registers[index][channel];
        }
    }

    const std::size_t stages = stage_count(tev);
    for (std::size_t index = 0; index < stages; ++index) {
        const MeleeHostGxTevStage& stage = tev.stages[index];

        std::array<int, 4> texel{};
        switch (texel_source(tev, stage)) {
        case TexelSource::Sampled:
            texel = inputs.texmap[stage.texmap];
            break;
        case TexelSource::White:
            texel = { 255, 255, 255, 255 };
            break;
        case TexelSource::Zero:
            break;
        }
        std::array<int, 4> raster{};
        const int channel = raster_channel(stage.color_channel);
        if (channel >= 0) {
            raster = inputs.raster[static_cast<std::size_t>(channel)];
        }
        const auto swap = [](const std::array<int, 4>& value,
                             mh_u32 selection) {
            const auto& table = kSwapTables[swap_table(selection)];
            return std::array<int, 4>{ value[table[0]], value[table[1]],
                                       value[table[2]], value[table[3]] };
        };
        texel = swap(texel, stage.texture_swap);
        raster = swap(raster, stage.raster_swap);

        const auto konst_component = [&tev](mh_u32 selection) {
            const std::size_t konst = (selection - 16U) % 4U;
            const std::size_t component = (selection - 16U) / 4U;
            return static_cast<int>(tev.konst_colors[konst][component]);
        };
        std::array<int, 3> konst_color{};
        const mh_u32 kcsel = stage.konst_color_select;
        if (kcsel < kKonstFractions.size()) {
            konst_color.fill(kKonstFractions[kcsel]);
        } else if (kcsel >= 12 && kcsel <= 15) {
            for (std::size_t component = 0; component < 3; ++component) {
                konst_color[component] =
                    tev.konst_colors[kcsel - 12U][component];
            }
        } else if (kcsel >= 16 && kcsel <= 31) {
            konst_color.fill(konst_component(kcsel));
        }
        const mh_u32 kasel = stage.konst_alpha_select;
        int konst_alpha = 0;
        if (kasel < kKonstFractions.size()) {
            konst_alpha = kKonstFractions[kasel];
        } else if (kasel >= 16 && kasel <= 31) {
            konst_alpha = konst_component(kasel);
        }

        const auto color_arg = [&](mh_u32 arg, std::size_t component) {
            switch (arg) {
            case 0: case 2: case 4: case 6:
                return reg[arg / 2][component];
            case 1: case 3: case 5: case 7:
                return reg[arg / 2][3];
            case 8:
                return texel[component];
            case 9:
                return texel[3];
            case 10:
                return raster[component];
            case 11:
                return raster[3];
            case 12:
                return 255;
            case 13:
                return 128;
            case 14:
                return konst_color[component];
            default:
                return 0;
            }
        };
        const auto alpha_arg = [&](mh_u32 arg) {
            switch (arg) {
            case 0: case 1: case 2: case 3:
                return reg[arg][3];
            case 4:
                return texel[3];
            case 5:
                return raster[3];
            case 6:
                return konst_alpha;
            default:
                return 0;
            }
        };

        std::array<int, 3> ca{};
        std::array<int, 3> cb{};
        std::array<int, 3> cc{};
        std::array<int, 3> cd{};
        for (std::size_t component = 0; component < 3; ++component) {
            ca[component] = color_arg(stage.color_input[0], component) & 255;
            cb[component] = color_arg(stage.color_input[1], component) & 255;
            cc[component] = color_arg(stage.color_input[2], component) & 255;
            cd[component] =
                sign_extend_11(color_arg(stage.color_input[3], component));
        }
        const int aa = alpha_arg(stage.alpha_input[0]) & 255;
        const int ab = alpha_arg(stage.alpha_input[1]) & 255;
        const int ac = alpha_arg(stage.alpha_input[2]) & 255;
        const int ad = sign_extend_11(alpha_arg(stage.alpha_input[3]));

        std::array<int, 3> color{};
        if (!is_compare(stage.color_op)) {
            for (std::size_t component = 0; component < 3; ++component) {
                color[component] = combine_regular(
                    ca[component], cb[component], cc[component], cd[component],
                    stage.color_op, stage.color_bias, stage.color_scale);
            }
        } else {
            const mh_u32 mode = (stage.color_op >> 1U) & 3U;
            for (std::size_t component = 0; component < 3; ++component) {
                const bool holds =
                    mode == 3 ? compare_holds(ca[component], cb[component],
                                              stage.color_op)
                              : compare_holds(packed(ca, mode),
                                              packed(cb, mode),
                                              stage.color_op);
                color[component] = cd[component] + (holds ? cc[component] : 0);
            }
        }

        int alpha = 0;
        if (!is_compare(stage.alpha_op)) {
            alpha = combine_regular(aa, ab, ac, ad, stage.alpha_op,
                                    stage.alpha_bias, stage.alpha_scale);
        } else {
            const mh_u32 mode = (stage.alpha_op >> 1U) & 3U;
            const bool holds =
                mode == 3 ? compare_holds(aa, ab, stage.alpha_op)
                          : compare_holds(packed(ca, mode), packed(cb, mode),
                                          stage.alpha_op);
            alpha = ad + (holds ? ac : 0);
        }

        /* Both combiners read the registers as they were before this stage,
         * so the writes happen only after both results exist. */
        if (stage.color_out_reg < reg.size()) {
            for (std::size_t component = 0; component < 3; ++component) {
                reg[stage.color_out_reg][component] =
                    clamp_output(color[component], stage.color_clamp);
            }
        }
        if (stage.alpha_out_reg < reg.size()) {
            reg[stage.alpha_out_reg][3] =
                clamp_output(alpha, stage.alpha_clamp);
        }
    }
    return { reg[0][0] & 255, reg[0][1] & 255, reg[0][2] & 255,
             reg[0][3] & 255 };
}

bool alpha_test_passes(const MeleeHostGxDrawState& state, int alpha)
{
    const auto compare = [alpha](mh_u32 function, int reference) {
        switch (function) {
        case 0:
            return false;
        case 1:
            return alpha < reference;
        case 2:
            return alpha == reference;
        case 3:
            return alpha <= reference;
        case 4:
            return alpha > reference;
        case 5:
            return alpha != reference;
        case 6:
            return alpha >= reference;
        default:
            return true;
        }
    };
    const bool first = compare(state.alpha_compare_0, state.alpha_ref_0);
    const bool second = compare(state.alpha_compare_1, state.alpha_ref_1);
    switch (state.alpha_op) {
    case 0:
        return first && second;
    case 1:
        return first || second;
    case 2:
        return first != second;
    default:
        return first == second;
    }
}

std::uint32_t tev_unmodelled_features(const MeleeHostGxTevState& tev)
{
    std::uint32_t features = kTevUnmodelledNone;
    const std::size_t stages = stage_count(tev);
    for (std::size_t index = 0; index < stages; ++index) {
        const MeleeHostGxTevStage& stage = tev.stages[index];
        if (texel_source(tev, stage) == TexelSource::Sampled &&
            stage.texcoord < tev.texcoord_gen_count &&
            stage.texcoord < MELEE_HOST_GX_MAX_TEXCOORD)
        {
            const mh_u32 function = tev.texcoord_gens[stage.texcoord].function;
            if (function >= kTexGenBump0 && function <= kTexGenBump7) {
                features |= kTevUnmodelledBumpTexGen;
            }
        }
        for (std::size_t input = 0; input < 4; ++input) {
            if (stage.color_input[input] > kColorArgZero ||
                stage.alpha_input[input] > kAlphaArgZero)
            {
                features |= kTevUnmodelledArgument;
            }
        }
    }
    return features;
}

std::string describe_tev_unmodelled(std::uint32_t features)
{
    std::string text;
    const auto append = [&text](const char* name) {
        if (!text.empty()) {
            text += ", ";
        }
        text += name;
    };
    if ((features & kTevUnmodelledBumpTexGen) != 0) {
        append("bump texgen");
    }
    if ((features & kTevUnmodelledArgument) != 0) {
        append("undefined input");
    }
    return text.empty() ? "none" : text;
}

std::string tev_vertex_shader_source()
{
    return R"(#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color0;
layout(location = 2) in vec4 a_color1;
layout(location = 3) in vec3 a_texgen[8];
uniform mat4 u_mvp;
out vec4 v_color0;
out vec4 v_color1;
out vec3 v_texgen[8];
void main()
{
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_color0 = a_color0;
    v_color1 = a_color1;
    for (int i = 0; i < 8; ++i) {
        v_texgen[i] = a_texgen[i];
    }
}
)";
}

namespace {

std::string color_arg_glsl(mh_u32 arg)
{
    switch (arg) {
    case 0: case 2: case 4: case 6:
        return "reg[" + std::to_string(arg / 2) + "].rgb";
    case 1: case 3: case 5: case 7:
        return "ivec3(reg[" + std::to_string(arg / 2) + "].a)";
    case 8:
        return "tex.rgb";
    case 9:
        return "ivec3(tex.a)";
    case 10:
        return "ras.rgb";
    case 11:
        return "ivec3(ras.a)";
    case 12:
        return "ivec3(255)";
    case 13:
        return "ivec3(128)";
    case 14:
        return "konst.rgb";
    default:
        return "ivec3(0)";
    }
}

std::string alpha_arg_glsl(mh_u32 arg)
{
    switch (arg) {
    case 0: case 1: case 2: case 3:
        return "reg[" + std::to_string(arg) + "].a";
    case 4:
        return "tex.a";
    case 5:
        return "ras.a";
    case 6:
        return "konst.a";
    default:
        return "0";
    }
}

std::string konst_component_glsl(mh_u32 selection)
{
    constexpr std::array<char, 4> kComponents{ 'r', 'g', 'b', 'a' };
    return "u_konst[" + std::to_string((selection - 16U) % 4U) + "]." +
           kComponents[(selection - 16U) / 4U];
}

std::string konst_color_glsl(mh_u32 selection)
{
    if (selection < kKonstFractions.size()) {
        return "ivec3(" + std::to_string(kKonstFractions[selection]) + ")";
    }
    if (selection >= 12 && selection <= 15) {
        return "u_konst[" + std::to_string(selection - 12U) + "].rgb";
    }
    if (selection >= 16 && selection <= 31) {
        return "ivec3(" + konst_component_glsl(selection) + ")";
    }
    return "ivec3(0)";
}

std::string konst_alpha_glsl(mh_u32 selection)
{
    if (selection < kKonstFractions.size()) {
        return std::to_string(kKonstFractions[selection]);
    }
    if (selection >= 16 && selection <= 31) {
        return konst_component_glsl(selection);
    }
    return "0";
}

std::string packed_glsl(const char* value, mh_u32 mode)
{
    const std::string v = value;
    switch (mode) {
    case 0:
        return v + ".r";
    case 1:
        return "((" + v + ".g << 8) | " + v + ".r)";
    default:
        return "((" + v + ".b << 16) | (" + v + ".g << 8) | " + v + ".r)";
    }
}

std::string regular_glsl(const char* type, const char* a, const char* b,
                         const char* c, const char* d, mh_u32 op, mh_u32 bias,
                         mh_u32 scale)
{
    const std::string t = type;
    const int multiplier = left_shift_of(scale);
    const int rounding =
        scale == kScaleDivide2 ? 0 : (op == kOpSub ? 127 : 128);
    const int bias_value =
        bias == kBiasAddHalf ? 128 : bias == kBiasSubHalf ? -128 : 0;
    std::string code;
    code += "        " + t + " stretched = " + c + " + (" + c + " >> 7);\n";
    code += "        " + t + " lerp = ((" + a + " * (256 - stretched) + " + b +
            " * stretched) * " + std::to_string(multiplier) + " + " +
            std::to_string(rounding) + ") >> 8;\n";
    std::string result = "((" + std::string(d) + " + (" +
                         std::to_string(bias_value) + ")) * " +
                         std::to_string(multiplier) +
                         (op == kOpSub ? " - " : " + ") + "lerp)";
    if (scale == kScaleDivide2) {
        result = "(" + result + " >> 1)";
    }
    code += "        result = " + result + ";\n";
    return code;
}

} // namespace

std::string tev_fragment_shader_source(const MeleeHostGxTevState& tev)
{
    std::string code = R"(#version 330 core
in vec4 v_color0;
in vec4 v_color1;
in vec3 v_texgen[8];
uniform sampler2D u_texmap[8];
uniform ivec4 u_register[4];
uniform ivec4 u_konst[4];
uniform ivec4 u_alpha_test;
uniform int u_alpha_ref1;
out vec4 frag_color;

bool gx_compare(int function, int value, int reference)
{
    if (function == 0) return false;
    if (function == 1) return value < reference;
    if (function == 2) return value == reference;
    if (function == 3) return value <= reference;
    if (function == 4) return value > reference;
    if (function == 5) return value != reference;
    if (function == 6) return value >= reference;
    return true;
}

ivec3 sign_extend_11(ivec3 value) { return ((value & 2047) ^ 1024) - 1024; }
int sign_extend_11(int value) { return ((value & 2047) ^ 1024) - 1024; }

ivec4 sample_texmap(int texmap, vec3 coord)
{
    float q = coord.z == 0.0 ? 1.0 : coord.z;
    return ivec4(round(texture(u_texmap[texmap], coord.xy / q) * 255.0));
}

void main()
{
    ivec4 raster0 = ivec4(round(clamp(v_color0, 0.0, 1.0) * 255.0));
    ivec4 raster1 = ivec4(round(clamp(v_color1, 0.0, 1.0) * 255.0));
    ivec4 reg[4];
    reg[0] = ivec4(0);
    reg[1] = u_register[1];
    reg[2] = u_register[2];
    reg[3] = u_register[3];
)";

    const std::size_t stages = stage_count(tev);
    for (std::size_t index = 0; index < stages; ++index) {
        const MeleeHostGxTevStage& stage = tev.stages[index];
        code += "    // stage " + std::to_string(index) + "\n    {\n";

        std::string texel;
        switch (texel_source(tev, stage)) {
        case TexelSource::Sampled: {
            /* A coordinate past the generated ones samples at the origin. */
            const std::string coord =
                stage.texcoord < tev.texcoord_gen_count &&
                        stage.texcoord < MELEE_HOST_GX_MAX_TEXCOORD
                    ? "v_texgen[" + std::to_string(stage.texcoord) + "]"
                    : std::string("vec3(0.0, 0.0, 1.0)");
            texel = "sample_texmap(" + std::to_string(stage.texmap) + ", " +
                    coord + ")";
            break;
        }
        case TexelSource::White:
            texel = "ivec4(255)";
            break;
        case TexelSource::Zero:
            texel = "ivec4(0)";
            break;
        }
        const int channel = raster_channel(stage.color_channel);
        const std::string raster =
            channel < 0 ? "ivec4(0)"
                        : "raster" + std::to_string(channel);
        code += "        ivec4 tex = (" + texel + ")." +
                kSwapSwizzles[swap_table(stage.texture_swap)] + ";\n";
        code += "        ivec4 ras = (" + raster + ")." +
                kSwapSwizzles[swap_table(stage.raster_swap)] + ";\n";
        code += "        ivec4 konst = ivec4(" +
                konst_color_glsl(stage.konst_color_select) + ", " +
                konst_alpha_glsl(stage.konst_alpha_select) + ");\n";
        code += "        ivec3 ca = " + color_arg_glsl(stage.color_input[0]) +
                " & 255;\n";
        code += "        ivec3 cb = " + color_arg_glsl(stage.color_input[1]) +
                " & 255;\n";
        code += "        ivec3 cc = " + color_arg_glsl(stage.color_input[2]) +
                " & 255;\n";
        code += "        ivec3 cd = sign_extend_11(" +
                color_arg_glsl(stage.color_input[3]) + ");\n";
        code += "        int aa = " + alpha_arg_glsl(stage.alpha_input[0]) +
                " & 255;\n";
        code += "        int ab = " + alpha_arg_glsl(stage.alpha_input[1]) +
                " & 255;\n";
        code += "        int ac = " + alpha_arg_glsl(stage.alpha_input[2]) +
                " & 255;\n";
        code += "        int ad = sign_extend_11(" +
                alpha_arg_glsl(stage.alpha_input[3]) + ");\n";

        code += "        ivec3 color;\n        {\n            ivec3 result;\n";
        if (!is_compare(stage.color_op)) {
            std::string block =
                regular_glsl("ivec3", "ca", "cb", "cc", "cd", stage.color_op,
                             stage.color_bias, stage.color_scale);
            code += block;
        } else {
            const mh_u32 mode = (stage.color_op >> 1U) & 3U;
            const char* relation = (stage.color_op & 1U) != 0 ? "equal"
                                                              : "greaterThan";
            if (mode == 3) {
                code += "            result = cd + cc * ivec3(" +
                        std::string(relation) + "(ca, cb));\n";
            } else {
                const char* scalar =
                    (stage.color_op & 1U) != 0 ? " == " : " > ";
                code += "            result = cd + ((" + packed_glsl("ca", mode) +
                        scalar + packed_glsl("cb", mode) +
                        ") ? cc : ivec3(0));\n";
            }
        }
        code += "            color = result;\n        }\n";

        code += "        int alpha;\n        {\n            int result;\n";
        if (!is_compare(stage.alpha_op)) {
            code += regular_glsl("int", "aa", "ab", "ac", "ad", stage.alpha_op,
                                 stage.alpha_bias, stage.alpha_scale);
        } else {
            const mh_u32 mode = (stage.alpha_op >> 1U) & 3U;
            const char* scalar = (stage.alpha_op & 1U) != 0 ? " == " : " > ";
            const std::string left =
                mode == 3 ? std::string("aa") : packed_glsl("ca", mode);
            const std::string right =
                mode == 3 ? std::string("ab") : packed_glsl("cb", mode);
            code += "            result = ad + ((" + left + scalar + right +
                    ") ? ac : 0);\n";
        }
        code += "            alpha = result;\n        }\n";

        const auto clamp_glsl = [](const char* value, bool clamp) {
            return clamp ? "clamp(" + std::string(value) + ", 0, 255)"
                         : "clamp(" + std::string(value) + ", -1024, 1023)";
        };
        if (stage.color_out_reg < MELEE_HOST_GX_MAX_TEVREG) {
            code += "        reg[" + std::to_string(stage.color_out_reg) +
                    "].rgb = " + clamp_glsl("color", stage.color_clamp) +
                    ";\n";
        }
        if (stage.alpha_out_reg < MELEE_HOST_GX_MAX_TEVREG) {
            code += "        reg[" + std::to_string(stage.alpha_out_reg) +
                    "].a = " + clamp_glsl("alpha", stage.alpha_clamp) + ";\n";
        }
        code += "    }\n";
    }

    code += R"(    ivec4 final_color = reg[0] & 255;
    bool first = gx_compare(u_alpha_test.x, final_color.a, u_alpha_test.y);
    bool second = gx_compare(u_alpha_test.w, final_color.a, u_alpha_ref1);
    bool passes;
    if (u_alpha_test.z == 0) passes = first && second;
    else if (u_alpha_test.z == 1) passes = first || second;
    else if (u_alpha_test.z == 2) passes = first != second;
    else passes = first == second;
    if (!passes) {
        discard;
    }
    frag_color = vec4(final_color) / 255.0;
}
)";
    return code;
}

} // namespace melee::gx
