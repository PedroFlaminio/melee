#include <melee_host/gx.h>

#include <dolphin/gx/GXGeometry.h>
#include <dolphin/gx/GXCommandList.h>
#include <dolphin/gx/GXDispList.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

namespace {

std::mutex command_mutex;
std::vector<MeleeHostGxCommand> commands;
std::vector<MeleeHostGxTriangle> triangles;
std::vector<MeleeHostGxCapturedVertex> captured_vertices;
std::vector<std::array<std::size_t, 3>> captured_triangle_indices;

struct ActiveDraw {
    bool active = false;
    mh_u8 primitive = 0;
    mh_u8 vertex_format = 0;
    mh_u16 expected_vertices = 0;
    std::size_t captured_vertex_start = 0;
    /* The position matrix the draw starts from, and the one the next vertex
     * will use.  They differ when the stream carries GX_VA_PNMTXIDX, which is
     * how a skinned PObj addresses a different joint per vertex. */
    mh_u32 draw_matrix_row = 0;
    mh_u32 vertex_matrix_row = 0;
    /* The texture bound when the draw began, as an index into the captured
     * texture table, or MELEE_HOST_GX_NO_TEXTURE. */
    mh_u32 texture_image = MELEE_HOST_GX_NO_TEXTURE;
    /* Index into the captured draw-state table. */
    mh_u32 draw_state = 0;
    /* Index into the captured TEV-state table. */
    mh_u32 tev_state = 0;
    /* Index into the captured texture-set table. */
    mh_u32 texture_set = 0;
    /* Texture-matrix rows named by GX_VA_TEXnMTXIDX for the next vertex.  Like
     * the position index they precede the position in the stream and apply to
     * one vertex only. */
    std::array<mh_u32, MELEE_HOST_GX_MAX_TEXCOORD> vertex_texture_matrix_rows{};
    std::vector<MeleeHostGxPosition3f32> positions;
    std::vector<MeleeHostGxCapturedVertex> vertices;
};

constexpr mh_u32 kNoMatrixIndex = 0xFFFFFFFFU;
constexpr mh_u32 kIdentityTexMatrix = 60;     // GX_IDENTITY
constexpr mh_u32 kIdentityPostMatrix = 125;   // GX_PTIDENTITY

/* What texgen reads that the public vertex does not keep: every raw texture
 * coordinate the stream carried, and the texture matrix each vertex named.
 * Parallel to captured_vertices. */
struct RawVertex {
    std::array<std::array<mh_f32, 2>, MELEE_HOST_GX_MAX_TEXCOORD> texcoord{};
    std::array<mh_u32, MELEE_HOST_GX_MAX_TEXCOORD> texture_matrix_row{};
};
std::vector<RawVertex> captured_raw_vertices;

/* The texture each texture map held for a draw, as ids into the captured
 * texture table, deduplicated like the other tables. */
using TextureSet = std::array<mh_u32, MELEE_HOST_GX_MAX_TEXMAP>;
std::vector<TextureSet> captured_texture_sets;

/* Distinct textures seen while capturing, in the order they were first bound.
 * A consumer uploads each once and indexes it by the id carried on the
 * vertex. */
std::vector<MeleeHostGxTextureDesc> captured_textures;
/* Distinct pixel states the draws ran under, in first-use order. */
std::vector<MeleeHostGxDrawState> captured_draw_states;
/* Distinct TEV configurations, likewise. */
std::vector<MeleeHostGxTevState> captured_tev_states;
/* The position matrix row each captured vertex was drawn with. */
std::vector<mh_u32> captured_vertex_matrix_rows;

ActiveDraw active_draw;

struct AttributeFormat {
    GXCompCnt component_count = GX_POS_XYZ;
    GXCompType component_type = GX_F32;
    mh_u8 fractional_bits = 0;
};

struct AttributeArray {
    const std::byte* base = nullptr;
    mh_u8 stride = 0;
    std::size_t byte_length = 0;
};

constexpr std::size_t kAttributeCount = static_cast<std::size_t>(GX_VA_MAX_ATTR);
constexpr std::size_t kVertexFormatCount =
    static_cast<std::size_t>(GX_MAX_VTXFMT);
/* The order vertex data appears in a display list, which is the hardware's
 * layout and not the numeric order of GXAttr.  GX_VA_NBT sits in the normal
 * slot: it describes the same field with three vectors instead of one, and the
 * two are mutually exclusive, so a stream carrying NBT puts its index where a
 * normal index would go.  Walking GXAttr numerically instead would consume the
 * NBT index after the texture coordinates and desynchronize every vertex that
 * has both. */
constexpr std::array<GXAttr, 22> kVertexLayoutOrder{
    GX_VA_PNMTXIDX,  GX_VA_TEX0MTXIDX, GX_VA_TEX1MTXIDX, GX_VA_TEX2MTXIDX,
    GX_VA_TEX3MTXIDX, GX_VA_TEX4MTXIDX, GX_VA_TEX5MTXIDX, GX_VA_TEX6MTXIDX,
    GX_VA_TEX7MTXIDX, GX_VA_POS,       GX_VA_NRM,        GX_VA_NBT,
    GX_VA_CLR0,      GX_VA_CLR1,       GX_VA_TEX0,       GX_VA_TEX1,
    GX_VA_TEX2,      GX_VA_TEX3,       GX_VA_TEX4,       GX_VA_TEX5,
    GX_VA_TEX6,      GX_VA_TEX7,
};

constexpr std::size_t kUnboundedArraySize =
    std::numeric_limits<std::size_t>::max();

std::array<GXAttrType, kAttributeCount> vertex_descriptors{};
std::array<std::array<AttributeFormat, kAttributeCount>, kVertexFormatCount>
    attribute_formats{};
std::array<AttributeArray, kAttributeCount> attribute_arrays{};

/* Regions the host owns and knows the extent of, which is how an array set
 * through the original GXSetArray still gets a bound.  On the console the
 * array simply ran into whatever followed it in the archive; the host has the
 * archive's extent, so it can stop at the end of the file instead of reading
 * past it. */
struct ArrayRegion {
    const std::byte* base;
    std::size_t byte_length;
};

std::vector<ArrayRegion> array_regions;

/* Indexed reads refused because they fell outside a bounded array. */
std::size_t rejected_index_count = 0;

std::size_t bounded_length_locked(const std::byte* base)
{
    for (const ArrayRegion& region : array_regions) {
        if (base >= region.base && base < region.base + region.byte_length) {
            return static_cast<std::size_t>(region.base + region.byte_length -
                                            base);
        }
    }
    return kUnboundedArraySize;
}
std::size_t display_list_errors = 0;

constexpr mh_u8 kGxQuads = 0x80U;
constexpr mh_u8 kGxTriangles = 0x90U;
constexpr mh_u8 kGxTriangleStrip = 0x98U;
constexpr mh_u8 kGxTriangleFan = 0xA0U;

void append_triangle(std::size_t first, std::size_t second, std::size_t third)
{
    triangles.push_back({ { active_draw.positions[first],
                           active_draw.positions[second],
                           active_draw.positions[third] } });
    captured_triangle_indices.push_back(
        { active_draw.captured_vertex_start + first,
          active_draw.captured_vertex_start + second,
          active_draw.captured_vertex_start + third });
}

void assemble_latest_position()
{
    const std::size_t count = active_draw.positions.size();
    if (active_draw.primitive == kGxTriangles && count % 3 == 0) {
        append_triangle(count - 3, count - 2, count - 1);
    } else if (active_draw.primitive == kGxTriangleStrip && count >= 3) {
        if ((count - 3) % 2 == 0) {
            append_triangle(count - 3, count - 2, count - 1);
        } else {
            append_triangle(count - 2, count - 3, count - 1);
        }
    } else if (active_draw.primitive == kGxTriangleFan && count >= 3) {
        append_triangle(0, count - 2, count - 1);
    } else if (active_draw.primitive == kGxQuads && count % 4 == 0) {
        append_triangle(count - 4, count - 3, count - 2);
        append_triangle(count - 4, count - 2, count - 1);
    }
}

void submit_locked(MeleeHostGxValueType type, mh_u32 bits)
{
    commands.push_back({ type, bits });
}

void submit(MeleeHostGxValueType type, mh_u32 bits)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(type, bits);
}

/* Applies an affine transform to one captured vertex: the position through the
 * full matrix, the normal through its normalized inverse-transpose, and the
 * tangent and binormal as directions. */
bool same_draw_state(const MeleeHostGxDrawState& left,
                     const MeleeHostGxDrawState& right)
{
    return left.cull_mode == right.cull_mode &&
           left.z_compare_enable == right.z_compare_enable &&
           left.z_update_enable == right.z_update_enable &&
           left.z_func == right.z_func &&
           left.blend_mode == right.blend_mode &&
           left.blend_src_factor == right.blend_src_factor &&
           left.blend_dst_factor == right.blend_dst_factor &&
           left.blend_logic_op == right.blend_logic_op &&
           left.color_update_enable == right.color_update_enable &&
           left.alpha_update_enable == right.alpha_update_enable &&
           left.alpha_compare_0 == right.alpha_compare_0 &&
           left.alpha_compare_1 == right.alpha_compare_1 &&
           left.alpha_op == right.alpha_op &&
           left.alpha_ref_0 == right.alpha_ref_0 &&
           left.alpha_ref_1 == right.alpha_ref_1;
}

/* Projects the modelled pixel state onto the fields that decide how a triangle
 * reaches the framebuffer, then resolves it to an index in the captured table.
 */
mh_u32 current_draw_state_id_locked()
{
    MeleeHostGxPixelState pixel{};
    melee_host_gx_pixel_state(&pixel);
    const MeleeHostGxDrawState state{
        pixel.cull_mode,      pixel.z_compare_enable,
        pixel.z_update_enable, pixel.z_func,
        pixel.blend_mode,     pixel.blend_src_factor,
        pixel.blend_dst_factor, pixel.blend_logic_op,
        pixel.color_update_enable, pixel.alpha_update_enable,
        pixel.alpha_compare_0, pixel.alpha_compare_1,
        pixel.alpha_op,       pixel.alpha_ref_0,
        pixel.alpha_ref_1,
    };
    for (std::size_t index = 0; index < captured_draw_states.size(); ++index) {
        if (same_draw_state(captured_draw_states[index], state)) {
            return static_cast<mh_u32>(index);
        }
    }
    captured_draw_states.push_back(state);
    return static_cast<mh_u32>(captured_draw_states.size() - 1);
}

bool same_tev_stage(const MeleeHostGxTevStage& left,
                    const MeleeHostGxTevStage& right)
{
    for (std::size_t index = 0; index < 4; ++index) {
        if (left.color_input[index] != right.color_input[index] ||
            left.alpha_input[index] != right.alpha_input[index])
        {
            return false;
        }
    }
    return left.mode == right.mode && left.texcoord == right.texcoord &&
           left.texmap == right.texmap &&
           left.color_channel == right.color_channel &&
           left.color_op == right.color_op &&
           left.color_bias == right.color_bias &&
           left.color_scale == right.color_scale &&
           left.color_out_reg == right.color_out_reg &&
           left.color_clamp == right.color_clamp &&
           left.alpha_op == right.alpha_op &&
           left.alpha_bias == right.alpha_bias &&
           left.alpha_scale == right.alpha_scale &&
           left.alpha_out_reg == right.alpha_out_reg &&
           left.alpha_clamp == right.alpha_clamp &&
           left.konst_color_select == right.konst_color_select &&
           left.konst_alpha_select == right.konst_alpha_select &&
           left.raster_swap == right.raster_swap &&
           left.texture_swap == right.texture_swap;
}

/* Marks the register and konst components a program actually reads.
 *
 * This matters because HSD_TExpSetReg builds its register values in an
 * uninitialized local and writes only the components its expression names, so
 * whatever the stack held reaches GX in the rest.  Those components never
 * affect the image, since no stage reads them, but comparing them would make
 * two draws of the same material look different from one run to the next. */
struct ReferencedComponents {
    mh_u8 registers[MELEE_HOST_GX_MAX_TEVREG] = {};
    mh_u8 konst[MELEE_HOST_GX_MAX_KCOLOR] = {};
};

constexpr mh_u8 kRgbMask = 0x7;
constexpr mh_u8 kAlphaMask = 0x8;

void mark_color_arg(mh_u32 arg, const MeleeHostGxTevStage& stage,
                    ReferencedComponents* out)
{
    /* CPREV/APREV through C2/A2 name a register and a side of it. */
    if (arg < 8) {
        const std::size_t reg = arg / 2;
        out->registers[reg] |= (arg % 2 == 0) ? kRgbMask : kAlphaMask;
        return;
    }
    if (arg != 14) { // GX_CC_KONST
        return;
    }
    const mh_u32 select = stage.konst_color_select;
    if (select >= 0x0C && select <= 0x0F) {
        out->konst[select - 0x0C] |= kRgbMask;
    } else if (select >= 0x10 && select <= 0x1F) {
        const std::size_t index = (select - 0x10) % 4;
        const mh_u8 component =
            select < 0x14 ? 0x1 : select < 0x18 ? 0x2
                                : select < 0x1C ? 0x4
                                                : kAlphaMask;
        out->konst[index] |= component;
    }
}

void mark_alpha_arg(mh_u32 arg, const MeleeHostGxTevStage& stage,
                    ReferencedComponents* out)
{
    if (arg < 4) { // APREV, A0, A1, A2
        out->registers[arg] |= kAlphaMask;
        return;
    }
    if (arg != 6) { // GX_CA_KONST
        return;
    }
    const mh_u32 select = stage.konst_alpha_select;
    if (select >= 0x10 && select <= 0x1F) {
        const std::size_t index = (select - 0x10) % 4;
        const mh_u8 component =
            select < 0x14 ? 0x1 : select < 0x18 ? 0x2
                                : select < 0x1C ? 0x4
                                                : kAlphaMask;
        out->konst[index] |= component;
    }
}

ReferencedComponents referenced_components(const MeleeHostGxTevState& tev)
{
    ReferencedComponents referenced;
    const std::size_t stages =
        tev.stage_count < MELEE_HOST_GX_MAX_TEVSTAGE
            ? tev.stage_count
            : static_cast<std::size_t>(MELEE_HOST_GX_MAX_TEVSTAGE);
    for (std::size_t stage = 0; stage < stages; ++stage) {
        for (std::size_t input = 0; input < 4; ++input) {
            mark_color_arg(tev.stages[stage].color_input[input],
                           tev.stages[stage], &referenced);
            mark_alpha_arg(tev.stages[stage].alpha_input[input],
                           tev.stages[stage], &referenced);
        }
    }
    return referenced;
}

/* Compared field by field rather than with memcmp: the struct has padding, and
 * a byte comparison would make two equal configurations look different. */
bool same_tev_state(const MeleeHostGxTevState& left,
                    const MeleeHostGxTevState& right)
{
    if (left.stage_count != right.stage_count ||
        left.texcoord_gen_count != right.texcoord_gen_count ||
        left.channel_count != right.channel_count)
    {
        return false;
    }
    for (std::size_t stage = 0; stage < left.stage_count; ++stage) {
        if (!same_tev_stage(left.stages[stage], right.stages[stage])) {
            return false;
        }
    }
    for (std::size_t gen = 0; gen < left.texcoord_gen_count; ++gen) {
        const MeleeHostGxTexCoordGen& a = left.texcoord_gens[gen];
        const MeleeHostGxTexCoordGen& b = right.texcoord_gens[gen];
        if (a.function != b.function || a.source != b.source ||
            a.matrix != b.matrix || a.normalize != b.normalize ||
            a.post_matrix != b.post_matrix)
        {
            return false;
        }
    }
    /* Only the components some stage reads; see ReferencedComponents. */
    const ReferencedComponents referenced = referenced_components(left);
    for (std::size_t reg = 0; reg < MELEE_HOST_GX_MAX_TEVREG; ++reg) {
        for (std::size_t channel = 0; channel < 4; ++channel) {
            const mh_u8 bit =
                channel == 3 ? kAlphaMask
                             : static_cast<mh_u8>(1U << channel);
            if ((referenced.registers[reg] & bit) != 0 &&
                left.registers[reg][channel] != right.registers[reg][channel])
            {
                return false;
            }
        }
    }
    for (std::size_t konst = 0; konst < MELEE_HOST_GX_MAX_KCOLOR; ++konst) {
        for (std::size_t channel = 0; channel < 4; ++channel) {
            const mh_u8 bit =
                channel == 3 ? kAlphaMask
                             : static_cast<mh_u8>(1U << channel);
            if ((referenced.konst[konst] & bit) != 0 &&
                left.konst_colors[konst][channel] !=
                    right.konst_colors[konst][channel])
            {
                return false;
            }
        }
    }
    return true;
}

mh_u32 current_tev_state_id_locked()
{
    MeleeHostGxTevState state{};
    melee_host_gx_tev_state(&state);
    for (std::size_t index = 0; index < captured_tev_states.size(); ++index) {
        if (same_tev_state(captured_tev_states[index], state)) {
            return static_cast<mh_u32>(index);
        }
    }
    captured_tev_states.push_back(state);
    return static_cast<mh_u32>(captured_tev_states.size() - 1);
}

void transform_vertex_locked(MeleeHostGxCapturedVertex& vertex,
                             const MeleeHostGxAffineTransform& matrix)
{
    const mh_f32 a = matrix.values[0][0];
    const mh_f32 b = matrix.values[0][1];
    const mh_f32 c = matrix.values[0][2];
    const mh_f32 d = matrix.values[1][0];
    const mh_f32 e = matrix.values[1][1];
    const mh_f32 f = matrix.values[1][2];
    const mh_f32 g = matrix.values[2][0];
    const mh_f32 h = matrix.values[2][1];
    const mh_f32 i = matrix.values[2][2];
    const mh_f32 determinant =
        a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    const auto transform_direction =
        [=](const MeleeHostGxPosition3f32& source) {
            return MeleeHostGxPosition3f32{
                a * source.x + b * source.y + c * source.z,
                d * source.x + e * source.y + f * source.z,
                g * source.x + h * source.y + i * source.z,
            };
        };
    const auto transform_normal = [=](const MeleeHostGxPosition3f32& source) {
        if (std::fabs(determinant) < 1.0e-8F) {
            return transform_direction(source);
        }
        return MeleeHostGxPosition3f32{
            ((e * i - f * h) * source.x + (f * g - d * i) * source.y +
             (d * h - e * g) * source.z) / determinant,
            ((c * h - b * i) * source.x + (a * i - c * g) * source.y +
             (b * g - a * h) * source.z) / determinant,
            ((b * f - c * e) * source.x + (c * d - a * f) * source.y +
             (a * e - b * d) * source.z) / determinant,
        };
    };
    const auto normalize = [](MeleeHostGxPosition3f32 vector) {
        const mh_f32 length = std::sqrt(vector.x * vector.x +
                                        vector.y * vector.y +
                                        vector.z * vector.z);
        if (length > 0.0F) {
            vector.x /= length;
            vector.y /= length;
            vector.z /= length;
        }
        return vector;
    };

    if ((vertex.attributes & MELEE_HOST_GX_VERTEX_POSITION) != 0) {
        const auto source = vertex.position;
        vertex.position = {
            a * source.x + b * source.y + c * source.z + matrix.values[0][3],
            d * source.x + e * source.y + f * source.z + matrix.values[1][3],
            g * source.x + h * source.y + i * source.z + matrix.values[2][3],
        };
    }
    if ((vertex.attributes & MELEE_HOST_GX_VERTEX_NORMAL) != 0) {
        vertex.normal = normalize(transform_normal(vertex.normal));
    }
    if ((vertex.attributes & MELEE_HOST_GX_VERTEX_TANGENT) != 0) {
        vertex.tangent = normalize(transform_direction(vertex.tangent));
    }
    if ((vertex.attributes & MELEE_HOST_GX_VERTEX_BINORMAL) != 0) {
        vertex.binormal = normalize(transform_direction(vertex.binormal));
    }
}

/* Resolves the texture bound to a texture map into an index in the captured
 * table, adding it the first time it is seen. */
mh_u32 current_texture_id_locked(mh_u32 texmap)
{
    MeleeHostGxTextureDesc desc{};
    if (!melee_host_gx_bound_texture(texmap, &desc) || !desc.bound ||
        desc.image == nullptr)
    {
        return MELEE_HOST_GX_NO_TEXTURE;
    }
    for (std::size_t index = 0; index < captured_textures.size(); ++index) {
        if (captured_textures[index].image == desc.image &&
            captured_textures[index].format == desc.format &&
            captured_textures[index].tlut_name == desc.tlut_name)
        {
            return static_cast<mh_u32>(index);
        }
    }
    captured_textures.push_back(desc);
    return static_cast<mh_u32>(captured_textures.size() - 1);
}

/* GX transforms every vertex by the position matrix in effect, so the capture
 * does the same: a skinned PObj loads one matrix per joint and names them per
 * vertex, which no single transform applied afterwards could reproduce. */
void transform_captured_draw_locked()
{
    for (std::size_t index = active_draw.captured_vertex_start;
         index < captured_vertices.size(); ++index) {
        const mh_u32 row = captured_vertex_matrix_rows[index];
        if (!melee_host_gx_matrix_loaded(row)) {
            continue;
        }
        MeleeHostGxAffineTransform matrix{};
        if (!melee_host_gx_matrix(row, &matrix)) {
            continue;
        }
        transform_vertex_locked(captured_vertices[index], matrix);
    }
    for (std::size_t index = 0; index < triangles.size(); ++index) {
        const auto& indices = captured_triangle_indices[index];
        triangles[index] = { { captured_vertices[indices[0]].position,
                               captured_vertices[indices[1]].position,
                               captured_vertices[indices[2]].position } };
    }
}

const MeleeHostGxTevState* active_tev_locked()
{
    return active_draw.tev_state < captured_tev_states.size()
               ? &captured_tev_states[active_draw.tev_state]
               : nullptr;
}

/* GX settles the channel count before TEV sees any colour: with no channel the
 * first rasterized colour is the vertex colour, white without one, and with
 * fewer than two the second repeats the first. */
void evaluate_captured_draw_locked()
{
    const MeleeHostGxTevState* const tev = active_tev_locked();
    const mh_u8 channels = tev != nullptr ? tev->channel_count : 0;
    for (std::size_t index = active_draw.captured_vertex_start;
         index < captured_vertices.size(); ++index) {
        auto& vertex = captured_vertices[index];
        melee_host_gx_evaluate_lighting(&vertex, vertex.raster_color[0],
                                        vertex.raster_color[1]);
        const bool has_color =
            (vertex.attributes & MELEE_HOST_GX_VERTEX_COLOR) != 0;
        for (std::size_t component = 0; component < 4; ++component) {
            if (channels == 0) {
                vertex.raster_color[0][component] =
                    has_color ? vertex.color[component]
                              : static_cast<mh_u8>(255);
            }
            if (channels < 2) {
                vertex.raster_color[1][component] =
                    vertex.raster_color[0][component];
            }
        }
    }
}

using TexMatrix = std::array<std::array<mh_f32, 4>, 3>;

/* A matrix row as texgen reads it.  GXInit loads identity at GX_IDENTITY and
 * GX_PTIDENTITY, which the host answers directly; a row the game never loaded
 * also reads as identity, the same choice the position transform makes. */
TexMatrix texgen_matrix(mh_u32 row)
{
    TexMatrix matrix{ { { 1.0F, 0.0F, 0.0F, 0.0F },
                        { 0.0F, 1.0F, 0.0F, 0.0F },
                        { 0.0F, 0.0F, 1.0F, 0.0F } } };
    if (row == kIdentityTexMatrix || row == kIdentityPostMatrix ||
        !melee_host_gx_matrix_loaded(row))
    {
        return matrix;
    }
    MeleeHostGxAffineTransform loaded{};
    if (!melee_host_gx_matrix(row, &loaded)) {
        return matrix;
    }
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 4; ++c) {
            matrix[r][c] = loaded.values[r][c];
        }
    }
    return matrix;
}

/* GXSetTexCoordGen2 for every vertex of the finished draw.  Matrix texgens
 * read the raw attributes, so they run before the position matrix moves them;
 * SRTG reads the lit colour, so it runs after lighting.  `color_sources`
 * selects which of the two passes this is. */
void evaluate_texgen_locked(bool color_sources)
{
    const MeleeHostGxTevState* const tev = active_tev_locked();
    if (tev == nullptr) {
        return;
    }
    constexpr mh_u32 kMtx2x4 = GX_TG_MTX2x4;
    constexpr mh_u32 kBump0 = GX_TG_BUMP0;
    constexpr mh_u32 kBump7 = GX_TG_BUMP7;
    constexpr mh_u32 kSrtg = GX_TG_SRTG;
    constexpr mh_u32 kSourceTex0 = GX_TG_TEX0;
    constexpr mh_u32 kSourceTex7 = GX_TG_TEX7;
    constexpr mh_u32 kSourceTexcoord0 = GX_TG_TEXCOORD0;
    constexpr mh_u32 kSourceColor1 = GX_TG_COLOR1;
    const std::size_t gens =
        tev->texcoord_gen_count < MELEE_HOST_GX_MAX_TEXCOORD
            ? tev->texcoord_gen_count
            : static_cast<std::size_t>(MELEE_HOST_GX_MAX_TEXCOORD);

    std::vector<std::pair<mh_u32, TexMatrix>> cache;
    const auto matrix_at = [&cache](mh_u32 row) {
        for (const auto& entry : cache) {
            if (entry.first == row) {
                return entry.second;
            }
        }
        cache.emplace_back(row, texgen_matrix(row));
        return cache.back().second;
    };
    const auto apply = [](const TexMatrix& m, const std::array<mh_f32, 4>& v,
                          std::size_t r) {
        return m[r][0] * v[0] + m[r][1] * v[1] + m[r][2] * v[2] +
               m[r][3] * v[3];
    };

    for (std::size_t index = active_draw.captured_vertex_start;
         index < captured_vertices.size(); ++index) {
        auto& vertex = captured_vertices[index];
        const RawVertex& raw = captured_raw_vertices[index];
        for (std::size_t gen = 0; gen < gens; ++gen) {
            const MeleeHostGxTexCoordGen& config = tev->texcoord_gens[gen];
            const bool srtg = config.function == kSrtg;
            if (srtg != color_sources) {
                continue;
            }
            mh_f32* const out = vertex.texgen[gen];
            if (srtg) {
                const std::size_t channel =
                    config.source == kSourceColor1 ? 1U : 0U;
                out[0] = static_cast<mh_f32>(vertex.raster_color[channel][0]) /
                         255.0F;
                out[1] = static_cast<mh_f32>(vertex.raster_color[channel][1]) /
                         255.0F;
                out[2] = 1.0F;
                continue;
            }
            if (config.function >= kBump0 && config.function <= kBump7) {
                /* Not modelled: the source coordinate, unperturbed. */
                const mh_u32 source = config.source - kSourceTexcoord0;
                const bool known =
                    config.source >= kSourceTexcoord0 && source < gen;
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    out[axis] = known ? vertex.texgen[source][axis]
                                      : (axis == 2 ? 1.0F : 0.0F);
                }
                continue;
            }

            std::array<mh_f32, 4> input{ 0.0F, 0.0F, 1.0F, 1.0F };
            const auto from_vector = [](const MeleeHostGxPosition3f32& v) {
                return std::array<mh_f32, 4>{ v.x, v.y, v.z, 1.0F };
            };
            switch (config.source) {
            case GX_TG_POS:
                input = from_vector(vertex.position);
                break;
            case GX_TG_NRM:
                input = from_vector(vertex.normal);
                break;
            case GX_TG_BINRM:
                input = from_vector(vertex.binormal);
                break;
            case GX_TG_TANGENT:
                input = from_vector(vertex.tangent);
                break;
            default:
                if (config.source >= kSourceTex0 &&
                    config.source <= kSourceTex7) {
                    const auto& coord = raw.texcoord[config.source - kSourceTex0];
                    input = { coord[0], coord[1], 1.0F, 1.0F };
                }
                break;
            }
            const mh_u32 row = raw.texture_matrix_row[gen] != kNoMatrixIndex
                                   ? raw.texture_matrix_row[gen]
                                   : config.matrix;
            const TexMatrix matrix = matrix_at(row);
            std::array<mh_f32, 4> result{
                apply(matrix, input, 0), apply(matrix, input, 1),
                config.function == kMtx2x4 ? 1.0F : apply(matrix, input, 2),
                1.0F
            };
            if (config.normalize) {
                const mh_f32 length =
                    std::sqrt(result[0] * result[0] + result[1] * result[1] +
                              result[2] * result[2]);
                if (length > 0.0F) {
                    for (std::size_t axis = 0; axis < 3; ++axis) {
                        result[axis] /= length;
                    }
                }
            }
            if (config.post_matrix != kIdentityPostMatrix) {
                const TexMatrix post = matrix_at(config.post_matrix);
                result = { apply(post, result, 0), apply(post, result, 1),
                           apply(post, result, 2), 1.0F };
            }
            for (std::size_t axis = 0; axis < 3; ++axis) {
                out[axis] = result[axis];
            }
        }
    }
}

mh_u32 texture_set_id_locked(const TextureSet& set)
{
    for (std::size_t index = 0; index < captured_texture_sets.size(); ++index) {
        if (captured_texture_sets[index] == set) {
            return static_cast<mh_u32>(index);
        }
    }
    captured_texture_sets.push_back(set);
    return static_cast<mh_u32>(captured_texture_sets.size() - 1);
}

/* The texture every sampled map held when the draw began. */
mh_u32 current_texture_set_id_locked()
{
    TextureSet set{};
    set.fill(MELEE_HOST_GX_NO_TEXTURE);
    if (const MeleeHostGxTevState* const tev = active_tev_locked()) {
        const std::size_t stages =
            tev->stage_count == 0 ? 1U
            : tev->stage_count < MELEE_HOST_GX_MAX_TEVSTAGE
                ? tev->stage_count
                : static_cast<std::size_t>(MELEE_HOST_GX_MAX_TEVSTAGE);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            const mh_u32 texmap = tev->stages[stage].texmap;
            if (texmap < MELEE_HOST_GX_MAX_TEXMAP &&
                set[texmap] == MELEE_HOST_GX_NO_TEXTURE)
            {
                set[texmap] = current_texture_id_locked(texmap);
            }
        }
    }
    return texture_set_id_locked(set);
}

/* Everything a draw needs once its last vertex is in. */
void finish_draw_locked()
{
    evaluate_texgen_locked(false);
    transform_captured_draw_locked();
    evaluate_captured_draw_locked();
    evaluate_texgen_locked(true);
    active_draw.active = false;
}

void begin_locked(mh_u8 primitive, mh_u8 vertex_format,
                  mh_u16 vertex_count)
{
    submit_locked(MELEE_HOST_GX_U8,
                  static_cast<mh_u32>(primitive | vertex_format));
    submit_locked(MELEE_HOST_GX_U16, vertex_count);
    active_draw.active = vertex_count != 0;
    active_draw.primitive = primitive;
    active_draw.vertex_format = vertex_format;
    active_draw.expected_vertices = vertex_count;
    active_draw.captured_vertex_start = captured_vertices.size();
    /* The state layer models the matrix memory and the bound textures, so the
     * draw reads what the game set rather than keeping a second copy. */
    MeleeHostGxTransformState transform{};
    melee_host_gx_transform_state(&transform);
    active_draw.draw_matrix_row = transform.current_matrix;
    active_draw.vertex_matrix_row = transform.current_matrix;
    active_draw.texture_image = current_texture_id_locked(0);
    active_draw.draw_state = current_draw_state_id_locked();
    active_draw.tev_state = current_tev_state_id_locked();
    active_draw.texture_set = current_texture_set_id_locked();
    active_draw.vertex_texture_matrix_rows.fill(kNoMatrixIndex);
    active_draw.positions.clear();
    active_draw.positions.reserve(vertex_count);
    active_draw.vertices.clear();
    active_draw.vertices.reserve(vertex_count);
}

bool valid_attribute(GXAttr attribute)
{
    return static_cast<unsigned>(attribute) <
           static_cast<unsigned>(GX_VA_MAX_ATTR);
}

bool valid_vertex_format(GXVtxFmt format)
{
    return static_cast<unsigned>(format) <
           static_cast<unsigned>(GX_MAX_VTXFMT);
}

mh_u16 read_be_u16(const std::byte* source)
{
    return static_cast<mh_u16>((std::to_integer<mh_u16>(source[0]) << 8U) |
                               std::to_integer<mh_u16>(source[1]));
}

mh_u32 read_be_u32(const std::byte* source)
{
    return (std::to_integer<mh_u32>(source[0]) << 24U) |
           (std::to_integer<mh_u32>(source[1]) << 16U) |
           (std::to_integer<mh_u32>(source[2]) << 8U) |
           std::to_integer<mh_u32>(source[3]);
}

std::size_t component_size(GXCompType type)
{
    switch (type) {
    case GX_U8:
    case GX_S8:
        return 1;
    case GX_U16:
    case GX_S16:
        return 2;
    case GX_F32:
        return 4;
    default:
        return 0;
    }
}

mh_f32 decode_component(const std::byte* source, GXCompType type,
                        mh_u8 fractional_bits)
{
    mh_f32 value = 0.0F;
    switch (type) {
    case GX_U8:
        value = static_cast<mh_f32>(std::to_integer<mh_u8>(source[0]));
        break;
    case GX_S8:
        value = static_cast<mh_f32>(
            static_cast<std::int8_t>(std::to_integer<mh_u8>(source[0])));
        break;
    case GX_U16:
        value = static_cast<mh_f32>(read_be_u16(source));
        break;
    case GX_S16:
        value = static_cast<mh_f32>(
            static_cast<std::int16_t>(read_be_u16(source)));
        break;
    case GX_F32:
        return std::bit_cast<mh_f32>(read_be_u32(source));
    default:
        return 0.0F;
    }
    return std::ldexp(value, -static_cast<int>(fractional_bits));
}

const std::byte* indexed_data(GXAttr attribute, mh_u16 index,
                              GXAttrType expected_type)
{
    if (!valid_attribute(attribute)) {
        return nullptr;
    }
    const std::size_t attribute_index = static_cast<std::size_t>(attribute);
    const AttributeArray& array = attribute_arrays[attribute_index];
    if (vertex_descriptors[attribute_index] != expected_type ||
        array.base == nullptr || array.stride == 0)
    {
        return nullptr;
    }
    const std::size_t offset = static_cast<std::size_t>(index) * array.stride;
    if (array.byte_length != kUnboundedArraySize &&
        (offset > array.byte_length ||
         array.stride > array.byte_length - offset))
    {
        /* Counted rather than passed over quietly: the caller drops the
         * attribute when this returns nothing, so without a count a bad index
         * would look like a vertex that simply had no normal. */
        ++rejected_index_count;
        return nullptr;
    }
    return array.base + offset;
}

const AttributeFormat* active_format(GXAttr attribute)
{
    if (!valid_attribute(attribute) ||
        active_draw.vertex_format >= kVertexFormatCount)
    {
        return nullptr;
    }
    return &attribute_formats[active_draw.vertex_format]
                             [static_cast<std::size_t>(attribute)];
}

void capture_position(mh_f32 x, mh_f32 y, mh_f32 z)
{
    if (!active_draw.active ||
        active_draw.positions.size() >= active_draw.expected_vertices)
    {
        return;
    }
    active_draw.positions.push_back({ x, y, z });
    MeleeHostGxCapturedVertex vertex{};
    vertex.attributes = MELEE_HOST_GX_VERTEX_POSITION;
    vertex.position = { x, y, z };
    vertex.texture_image = active_draw.texture_image;
    vertex.draw_state = active_draw.draw_state;
    vertex.tev_state = active_draw.tev_state;
    vertex.texture_set = active_draw.texture_set;
    if (active_draw.texture_image != MELEE_HOST_GX_NO_TEXTURE) {
        vertex.attributes |= MELEE_HOST_GX_VERTEX_TEXTURE_IMAGE;
    }
    active_draw.vertices.push_back(vertex);
    captured_vertices.push_back(vertex);
    captured_vertex_matrix_rows.push_back(active_draw.vertex_matrix_row);
    RawVertex raw{};
    raw.texture_matrix_row = active_draw.vertex_texture_matrix_rows;
    captured_raw_vertices.push_back(raw);
    /* A matrix index applies to one vertex; the next falls back to the one the
     * draw started with. */
    active_draw.vertex_matrix_row = active_draw.draw_matrix_row;
    active_draw.vertex_texture_matrix_rows.fill(kNoMatrixIndex);
    assemble_latest_position();
}

void capture_normal(mh_f32 x, mh_f32 y, mh_f32 z)
{
    if (active_draw.active && !active_draw.vertices.empty()) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_NORMAL;
        vertex.normal = { x, y, z };
        captured_vertices.back() = vertex;
    }
}

void capture_nbt(const MeleeHostGxPosition3f32& normal,
                 const MeleeHostGxPosition3f32& tangent,
                 const MeleeHostGxPosition3f32& binormal)
{
    if (active_draw.active && !active_draw.vertices.empty()) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_NORMAL |
                             MELEE_HOST_GX_VERTEX_TANGENT |
                             MELEE_HOST_GX_VERTEX_BINORMAL;
        vertex.normal = normal;
        vertex.tangent = tangent;
        vertex.binormal = binormal;
        captured_vertices.back() = vertex;
    }
}

void capture_color(mh_u8 red, mh_u8 green, mh_u8 blue, mh_u8 alpha)
{
    if (active_draw.active && !active_draw.vertices.empty()) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_COLOR;
        vertex.color[0] = red;
        vertex.color[1] = green;
        vertex.color[2] = blue;
        vertex.color[3] = alpha;
        captured_vertices.back() = vertex;
    }
}

/* Every coordinate the stream carries lands in the raw vertex texgen reads;
 * the first is also kept on the public vertex. */
void capture_texcoord(std::size_t slot, mh_f32 s, mh_f32 t)
{
    if (!active_draw.active || active_draw.vertices.empty() ||
        slot >= MELEE_HOST_GX_MAX_TEXCOORD)
    {
        return;
    }
    captured_raw_vertices.back().texcoord[slot] = { s, t };
    if (slot == 0) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_TEXCOORD;
        vertex.texcoord[0] = s;
        vertex.texcoord[1] = t;
        captured_vertices.back() = vertex;
    }
}

void decode_position_index(mh_u16 index, GXAttrType index_type)
{
    const std::byte* source = indexed_data(GX_VA_POS, index, index_type);
    const AttributeFormat* format = active_format(GX_VA_POS);
    if (source == nullptr || format == nullptr) {
        return;
    }
    const std::size_t size = component_size(format->component_type);
    if (size == 0) {
        return;
    }
    const mh_f32 x = decode_component(source, format->component_type,
                                      format->fractional_bits);
    const mh_f32 y = decode_component(source + size, format->component_type,
                                      format->fractional_bits);
    const mh_f32 z = format->component_count == GX_POS_XYZ
                         ? decode_component(source + 2 * size,
                                            format->component_type,
                                            format->fractional_bits)
                         : 0.0F;
    capture_position(x, y, z);
}

MeleeHostGxPosition3f32 decode_normal(const std::byte* source,
                                      const AttributeFormat& format,
                                      std::size_t offset)
{
    const std::size_t size = component_size(format.component_type);
    return {
        decode_component(source + offset, format.component_type,
                         format.fractional_bits),
        decode_component(source + offset + size, format.component_type,
                         format.fractional_bits),
        decode_component(source + offset + 2 * size, format.component_type,
                         format.fractional_bits),
    };
}

void decode_normal_index(GXAttr attribute, mh_u16 index, GXAttrType index_type,
                         std::size_t nbt3_component = 0)
{
    const std::byte* source = indexed_data(attribute, index, index_type);
    const AttributeFormat* format = active_format(attribute);
    if (source == nullptr || format == nullptr)
    {
        return;
    }
    const std::size_t size = component_size(format->component_type);
    if (size == 0) {
        return;
    }
    if (attribute == GX_VA_NRM) {
        const auto normal = decode_normal(source, *format, 0);
        capture_normal(normal.x, normal.y, normal.z);
    } else if (format->component_count == GX_NRM_NBT) {
        capture_nbt(decode_normal(source, *format, 0),
                    decode_normal(source, *format, 3 * size),
                    decode_normal(source, *format, 6 * size));
    } else if (format->component_count == GX_NRM_NBT3) {
        const auto vector = decode_normal(source, *format, 0);
        if (nbt3_component == 0) {
            capture_normal(vector.x, vector.y, vector.z);
        } else if (active_draw.active && !active_draw.vertices.empty()) {
            auto& vertex = active_draw.vertices.back();
            if (nbt3_component == 1) {
                vertex.attributes |= MELEE_HOST_GX_VERTEX_TANGENT;
                vertex.tangent = vector;
            } else {
                vertex.attributes |= MELEE_HOST_GX_VERTEX_BINORMAL;
                vertex.binormal = vector;
            }
            captured_vertices.back() = vertex;
        }
    }
}

mh_u8 expand_bits(mh_u32 value, unsigned bits)
{
    const mh_u32 maximum = (1U << bits) - 1U;
    return static_cast<mh_u8>((value * 255U + maximum / 2U) / maximum);
}

void decode_color_data(const std::byte* source, const AttributeFormat& format)
{
    mh_u8 red = 0;
    mh_u8 green = 0;
    mh_u8 blue = 0;
    mh_u8 alpha = 255;
    switch (format.component_type) {
    case GX_RGB565: {
        const mh_u16 packed = read_be_u16(source);
        red = expand_bits((packed >> 11U) & 0x1FU, 5);
        green = expand_bits((packed >> 5U) & 0x3FU, 6);
        blue = expand_bits(packed & 0x1FU, 5);
        break;
    }
    case GX_RGB8:
        red = std::to_integer<mh_u8>(source[0]);
        green = std::to_integer<mh_u8>(source[1]);
        blue = std::to_integer<mh_u8>(source[2]);
        break;
    case GX_RGBX8:
        red = std::to_integer<mh_u8>(source[0]);
        green = std::to_integer<mh_u8>(source[1]);
        blue = std::to_integer<mh_u8>(source[2]);
        break;
    case GX_RGBA4: {
        const mh_u16 packed = read_be_u16(source);
        red = expand_bits((packed >> 12U) & 0xFU, 4);
        green = expand_bits((packed >> 8U) & 0xFU, 4);
        blue = expand_bits((packed >> 4U) & 0xFU, 4);
        alpha = expand_bits(packed & 0xFU, 4);
        break;
    }
    case GX_RGBA6: {
        const mh_u32 packed =
            (std::to_integer<mh_u32>(source[0]) << 16U) |
            (std::to_integer<mh_u32>(source[1]) << 8U) |
            std::to_integer<mh_u32>(source[2]);
        red = expand_bits((packed >> 18U) & 0x3FU, 6);
        green = expand_bits((packed >> 12U) & 0x3FU, 6);
        blue = expand_bits((packed >> 6U) & 0x3FU, 6);
        alpha = expand_bits(packed & 0x3FU, 6);
        break;
    }
    case GX_RGBA8:
        red = std::to_integer<mh_u8>(source[0]);
        green = std::to_integer<mh_u8>(source[1]);
        blue = std::to_integer<mh_u8>(source[2]);
        alpha = std::to_integer<mh_u8>(source[3]);
        break;
    default:
        return;
    }
    capture_color(red, green, blue, alpha);
}

void decode_color_index(mh_u16 index, GXAttrType index_type)
{
    const std::byte* source = indexed_data(GX_VA_CLR0, index, index_type);
    const AttributeFormat* format = active_format(GX_VA_CLR0);
    if (source != nullptr && format != nullptr) {
        decode_color_data(source, *format);
    }
}

void decode_texcoord_data(GXAttr attribute, const std::byte* source,
                          const AttributeFormat& format)
{
    const std::size_t size = component_size(format.component_type);
    if (size == 0) {
        return;
    }
    const mh_f32 s = decode_component(source, format.component_type,
                                      format.fractional_bits);
    const mh_f32 t = format.component_count == GX_TEX_ST
                         ? decode_component(source + size,
                                            format.component_type,
                                            format.fractional_bits)
                         : 0.0F;
    capture_texcoord(static_cast<std::size_t>(attribute - GX_VA_TEX0), s, t);
}

void decode_texcoord_index(GXAttr attribute, mh_u16 index,
                           GXAttrType index_type)
{
    const std::byte* source = indexed_data(attribute, index, index_type);
    const AttributeFormat* format = active_format(attribute);
    if (source != nullptr && format != nullptr) {
        decode_texcoord_data(attribute, source, *format);
    }
}

bool is_matrix_index(GXAttr attribute)
{
    return attribute >= GX_VA_PNMTXIDX && attribute <= GX_VA_TEX7MTXIDX;
}

bool is_color(GXAttr attribute)
{
    return attribute == GX_VA_CLR0 || attribute == GX_VA_CLR1;
}

bool is_texcoord(GXAttr attribute)
{
    return attribute >= GX_VA_TEX0 && attribute <= GX_VA_TEX7;
}

std::size_t packed_color_size(GXCompType type)
{
    switch (type) {
    case GX_RGB565:
    case GX_RGBA4:
        return 2;
    case GX_RGB8:
    case GX_RGBA6:
        return 3;
    case GX_RGBX8:
    case GX_RGBA8:
        return 4;
    default:
        return 0;
    }
}

std::size_t direct_attribute_size(GXAttr attribute,
                                  const AttributeFormat& format)
{
    if (is_matrix_index(attribute)) {
        return 1;
    }
    if (attribute == GX_VA_POS) {
        const std::size_t size = component_size(format.component_type);
        return size * (format.component_count == GX_POS_XYZ ? 3U : 2U);
    }
    if (attribute == GX_VA_NRM || attribute == GX_VA_NBT) {
        const std::size_t size = component_size(format.component_type);
        return size * (format.component_count == GX_NRM_XYZ ? 3U : 9U);
    }
    if (is_color(attribute)) {
        return packed_color_size(format.component_type);
    }
    if (is_texcoord(attribute)) {
        const std::size_t size = component_size(format.component_type);
        return size * (format.component_count == GX_TEX_ST ? 2U : 1U);
    }
    return 0;
}

void decode_direct_attribute(GXAttr attribute, const std::byte* source,
                             const AttributeFormat& format)
{
    if (attribute == GX_VA_PNMTXIDX) {
        /* The position-matrix index selects a row of matrix memory for this
         * vertex alone, which is how an envelope-skinned PObj addresses one
         * joint per vertex. */
        active_draw.vertex_matrix_row =
            std::to_integer<mh_u8>(source[0]);
        return;
    }
    if (attribute >= GX_VA_TEX0MTXIDX && attribute <= GX_VA_TEX7MTXIDX) {
        /* Likewise for the texture matrix of one coordinate. */
        active_draw.vertex_texture_matrix_rows[static_cast<std::size_t>(
            attribute - GX_VA_TEX0MTXIDX)] = std::to_integer<mh_u8>(source[0]);
        return;
    }
    if (attribute == GX_VA_POS) {
        const std::size_t size = component_size(format.component_type);
        const mh_f32 x = decode_component(source, format.component_type,
                                          format.fractional_bits);
        const mh_f32 y = decode_component(source + size, format.component_type,
                                          format.fractional_bits);
        const mh_f32 z = format.component_count == GX_POS_XYZ
                             ? decode_component(source + 2 * size,
                                                format.component_type,
                                                format.fractional_bits)
                             : 0.0F;
        capture_position(x, y, z);
    } else if (attribute == GX_VA_NRM || attribute == GX_VA_NBT)
    {
        const std::size_t size = component_size(format.component_type);
        if (attribute == GX_VA_NRM) {
            const auto normal = decode_normal(source, format, 0);
            capture_normal(normal.x, normal.y, normal.z);
        } else {
            capture_nbt(decode_normal(source, format, 0),
                        decode_normal(source, format, 3 * size),
                        decode_normal(source, format, 6 * size));
        }
    } else if (attribute == GX_VA_CLR0) {
        decode_color_data(source, format);
    } else if (attribute >= GX_VA_TEX0 && attribute <= GX_VA_TEX7) {
        decode_texcoord_data(attribute, source, format);
    }
}

bool valid_draw_primitive(mh_u8 primitive)
{
    switch (primitive) {
    case GX_DRAW_QUADS:
    case GX_DRAW_TRIANGLES:
    case GX_DRAW_TRIANGLE_STRIP:
    case GX_DRAW_TRIANGLE_FAN:
    case GX_DRAW_LINES:
    case GX_DRAW_LINE_STRIP:
    case GX_DRAW_POINTS:
        return true;
    default:
        return false;
    }
}

bool consume_display_attribute(const std::byte*& cursor,
                               const std::byte* end, GXAttr attribute,
                               GXAttrType descriptor,
                               const AttributeFormat& format)
{
    if (descriptor == GX_DIRECT) {
        const std::size_t byte_count = direct_attribute_size(attribute, format);
        if (byte_count == 0 || static_cast<std::size_t>(end - cursor) < byte_count) {
            return false;
        }
        decode_direct_attribute(attribute, cursor, format);
        cursor += byte_count;
        return true;
    }

    if (descriptor != GX_INDEX8 && descriptor != GX_INDEX16) {
        return false;
    }
    const std::size_t index_size = descriptor == GX_INDEX8 ? 1U : 2U;
    const std::size_t index_count =
        (attribute == GX_VA_NRM || attribute == GX_VA_NBT) &&
                format.component_count == GX_NRM_NBT3
            ? 3U
            : 1U;
    if (static_cast<std::size_t>(end - cursor) < index_size * index_count) {
        return false;
    }

    for (std::size_t item = 0; item < index_count; ++item) {
        const mh_u16 index = descriptor == GX_INDEX8
                                 ? std::to_integer<mh_u8>(cursor[0])
                                 : read_be_u16(cursor);
        submit_locked(descriptor == GX_INDEX8 ? MELEE_HOST_GX_U8
                                              : MELEE_HOST_GX_U16,
                      index);
        if (item == 0 || (attribute == GX_VA_NBT &&
                          format.component_count == GX_NRM_NBT3)) {
            if (attribute == GX_VA_POS) {
                decode_position_index(index, descriptor);
            } else if (attribute == GX_VA_NRM || attribute == GX_VA_NBT) {
                decode_normal_index(attribute, index, descriptor, item);
            } else if (attribute == GX_VA_CLR0) {
                decode_color_index(index, descriptor);
            } else if (is_texcoord(attribute)) {
                decode_texcoord_index(attribute, index, descriptor);
            }
        }
        cursor += index_size;
    }
    return true;
}

bool parse_display_list_locked(const std::byte* cursor, std::size_t byte_count)
{
    if (byte_count == 0) {
        return true;
    }
    if (cursor == nullptr && byte_count != 0) {
        return false;
    }
    const std::byte* const end = cursor + byte_count;
    while (cursor < end) {
        const mh_u8 opcode = std::to_integer<mh_u8>(*cursor++);
        if (opcode == GX_NOP) {
            return true;
        }
        const mh_u8 primitive = opcode & GX_OPCODE_MASK;
        const mh_u8 vertex_format = opcode & GX_VAT_MASK;
        if (!valid_draw_primitive(primitive) || end - cursor < 2) {
            return false;
        }
        const mh_u16 vertex_count = read_be_u16(cursor);
        cursor += 2;
        begin_locked(primitive, vertex_format, vertex_count);

        for (mh_u16 vertex = 0; vertex < vertex_count; ++vertex) {
            for (const GXAttr attribute : kVertexLayoutOrder) {
                const std::size_t index = static_cast<std::size_t>(attribute);
                const GXAttrType descriptor = vertex_descriptors[index];
                if (descriptor == GX_NONE) {
                    continue;
                }
                const AttributeFormat& format =
                    attribute_formats[vertex_format][index];
                if (!consume_display_attribute(cursor, end, attribute,
                                               descriptor, format))
                {
                    finish_draw_locked();
                    return false;
                }
            }
        }
        finish_draw_locked();
    }
    return true;
}

template <typename Index, typename Decoder>
void submit_index(Index index, MeleeHostGxValueType command_type,
                  GXAttrType index_type, Decoder decoder)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(command_type, static_cast<mh_u32>(index));
    decoder(static_cast<mh_u16>(index), index_type);
}

} // namespace

extern "C" void melee_host_gx_submit_u8(mh_u8 value)
{
    submit(MELEE_HOST_GX_U8, value);
}

extern "C" void melee_host_gx_submit_u16(mh_u16 value)
{
    submit(MELEE_HOST_GX_U16, value);
}

extern "C" void melee_host_gx_submit_u32(mh_u32 value)
{
    submit(MELEE_HOST_GX_U32, value);
}

extern "C" void melee_host_gx_submit_f32(mh_f32 value)
{
    submit(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(value));
}

extern "C" void melee_host_gx_begin(mh_u8 primitive, mh_u8 vertex_format,
                                     mh_u16 vertex_count)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    begin_locked(primitive, vertex_format, vertex_count);
}

extern "C" void melee_host_gx_end(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    finish_draw_locked();
}

extern "C" void melee_host_gx_submit_position3f32(mh_f32 x, mh_f32 y,
                                                    mh_f32 z)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(x));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(y));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(z));
    if (!active_draw.active) {
        return;
    }
    if (active_draw.positions.size() >= active_draw.expected_vertices) {
        return;
    }
    capture_position(x, y, z);
}

extern "C" void melee_host_gx_submit_normal3f32(mh_f32 x, mh_f32 y,
                                                  mh_f32 z)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(x));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(y));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(z));
    capture_normal(x, y, z);
}

extern "C" void melee_host_gx_submit_color4u8(mh_u8 red, mh_u8 green,
                                                mh_u8 blue, mh_u8 alpha)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_U8, red);
    submit_locked(MELEE_HOST_GX_U8, green);
    submit_locked(MELEE_HOST_GX_U8, blue);
    submit_locked(MELEE_HOST_GX_U8, alpha);
    capture_color(red, green, blue, alpha);
}

extern "C" void melee_host_gx_submit_texcoord2f32(mh_f32 s, mh_f32 t)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(s));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(t));
    capture_texcoord(0, s, t);
}

extern "C" void melee_host_gx_submit_position_index8(mh_u8 index)
{
    submit_index(index, MELEE_HOST_GX_U8, GX_INDEX8, decode_position_index);
}

extern "C" void melee_host_gx_submit_position_index16(mh_u16 index)
{
    submit_index(index, MELEE_HOST_GX_U16, GX_INDEX16, decode_position_index);
}

extern "C" void melee_host_gx_submit_normal_index8(mh_u8 index)
{
    submit_index(index, MELEE_HOST_GX_U8, GX_INDEX8,
                 [](mh_u16 value, GXAttrType type) {
                     decode_normal_index(GX_VA_NRM, value, type);
                 });
}

extern "C" void melee_host_gx_submit_normal_index16(mh_u16 index)
{
    submit_index(index, MELEE_HOST_GX_U16, GX_INDEX16,
                 [](mh_u16 value, GXAttrType type) {
                     decode_normal_index(GX_VA_NRM, value, type);
                 });
}

extern "C" void melee_host_gx_submit_color_index8(mh_u8 index)
{
    submit_index(index, MELEE_HOST_GX_U8, GX_INDEX8, decode_color_index);
}

extern "C" void melee_host_gx_submit_color_index16(mh_u16 index)
{
    submit_index(index, MELEE_HOST_GX_U16, GX_INDEX16, decode_color_index);
}

extern "C" void melee_host_gx_submit_texcoord_index8(mh_u8 index)
{
    submit_index(index, MELEE_HOST_GX_U8, GX_INDEX8,
                 [](mh_u16 value, GXAttrType type) {
                     decode_texcoord_index(GX_VA_TEX0, value, type);
                 });
}

extern "C" void melee_host_gx_submit_texcoord_index16(mh_u16 index)
{
    submit_index(index, MELEE_HOST_GX_U16, GX_INDEX16,
                 [](mh_u16 value, GXAttrType type) {
                     decode_texcoord_index(GX_VA_TEX0, value, type);
                 });
}

extern "C" void GXSetVtxDesc(GXAttr attribute, GXAttrType type)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (valid_attribute(attribute)) {
        vertex_descriptors[static_cast<std::size_t>(attribute)] = type;
    }
}

extern "C" void GXSetVtxDescv(const GXVtxDescList* list)
{
    if (list == nullptr) {
        return;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    for (; list->attr != GX_VA_NULL; ++list) {
        if (valid_attribute(list->attr)) {
            vertex_descriptors[static_cast<std::size_t>(list->attr)] =
                list->type;
        }
    }
}

extern "C" void GXClearVtxDesc(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    vertex_descriptors.fill(GX_NONE);
}

extern "C" void GXSetVtxAttrFmt(GXVtxFmt vertex_format, GXAttr attribute,
                                  GXCompCnt component_count,
                                  GXCompType component_type, u8 fractional_bits)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (valid_vertex_format(vertex_format) && valid_attribute(attribute)) {
        attribute_formats[static_cast<std::size_t>(vertex_format)]
                         [static_cast<std::size_t>(attribute)] = {
                             component_count, component_type, fractional_bits
                         };
    }
}

extern "C" void GXSetVtxAttrFmtv(GXVtxFmt vertex_format,
                                   const GXVtxAttrFmtList* list)
{
    if (list == nullptr || !valid_vertex_format(vertex_format)) {
        return;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    for (; list->attr != GX_VA_NULL; ++list) {
        if (valid_attribute(list->attr)) {
            attribute_formats[static_cast<std::size_t>(vertex_format)]
                             [static_cast<std::size_t>(list->attr)] = {
                                 list->cnt, list->type, list->frac
                             };
        }
    }
}

extern "C" void GXSetArray(GXAttr attribute, const void* base, u8 stride)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (valid_attribute(attribute)) {
        const auto* const bytes = static_cast<const std::byte*>(base);
        attribute_arrays[static_cast<std::size_t>(attribute)] = {
            bytes, stride, bounded_length_locked(bytes)
        };
    }
}

extern "C" void melee_host_gx_register_array_region(const void* base,
                                                    size_t byte_length)
{
    if (base == nullptr || byte_length == 0) {
        return;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    array_regions.push_back(
        { static_cast<const std::byte*>(base), byte_length });
}

extern "C" void melee_host_gx_unregister_array_region(const void* base)
{
    if (base == nullptr) {
        return;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    for (std::size_t index = 0; index < array_regions.size(); ++index) {
        if (array_regions[index].base == base) {
            array_regions.erase(array_regions.begin() +
                                static_cast<std::ptrdiff_t>(index));
            return;
        }
    }
}

extern "C" void melee_host_gx_set_array_bounded(mh_u32 attribute,
                                                   const void* base,
                                                   size_t byte_length,
                                                   mh_u8 stride)
{
    const auto gx_attribute = static_cast<GXAttr>(attribute);
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (valid_attribute(gx_attribute)) {
        attribute_arrays[static_cast<std::size_t>(gx_attribute)] = {
            static_cast<const std::byte*>(base), stride, byte_length
        };
    }
}

extern "C" void GXInvalidateVtxCache(void)
{
    // Desktop arrays are read at submission time, so there is no vertex cache.
}

extern "C" void GXCallDisplayList(void* list, u32 byte_count)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (!parse_display_list_locked(static_cast<const std::byte*>(list),
                                   byte_count))
    {
        ++display_list_errors;
    }
}

extern "C" size_t melee_host_gx_rejected_index_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return rejected_index_count;
}

extern "C" void melee_host_gx_reset_command_log(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    commands.clear();
    triangles.clear();
    captured_vertices.clear();
    captured_triangle_indices.clear();
    display_list_errors = 0;
    rejected_index_count = 0;
    captured_textures.clear();
    captured_draw_states.clear();
    captured_tev_states.clear();
    captured_vertex_matrix_rows.clear();
    captured_raw_vertices.clear();
    captured_texture_sets.clear();
    active_draw = {};
}

extern "C" size_t melee_host_gx_captured_texture_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return captured_textures.size();
}

extern "C" bool melee_host_gx_captured_texture_at(
    size_t index, MeleeHostGxTextureDesc* output)
{
    if (output == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= captured_textures.size()) {
        return false;
    }
    *output = captured_textures[index];
    return true;
}

extern "C" size_t melee_host_gx_captured_texture_set_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return captured_texture_sets.size();
}

extern "C" bool melee_host_gx_captured_texture_set_at(
    size_t index, mh_u32 textures[MELEE_HOST_GX_MAX_TEXMAP])
{
    if (textures == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= captured_texture_sets.size()) {
        return false;
    }
    for (std::size_t map = 0; map < MELEE_HOST_GX_MAX_TEXMAP; ++map) {
        textures[map] = captured_texture_sets[index][map];
    }
    return true;
}

extern "C" bool melee_host_gx_resolve_alpha_test(
    const MeleeHostGxDrawState* state, mh_u32* out_compare,
    mh_u8* out_reference)
{
    if (state == nullptr || out_compare == nullptr ||
        out_reference == nullptr) {
        return false;
    }
    enum : mh_u32 {
        kCompareLessEqual = 3,
        kCompareGreaterEqual = 6,
        kCompareAlways = 7,
        kOpAnd = 0,
    };
    /* True for every alpha, so the comparison says nothing. */
    const auto vacuous = [](mh_u32 compare, mh_u8 reference) {
        return compare == kCompareAlways ||
               (compare == kCompareLessEqual && reference == 255) ||
               (compare == kCompareGreaterEqual && reference == 0);
    };
    const bool first_vacuous =
        vacuous(state->alpha_compare_0, state->alpha_ref_0);
    const bool second_vacuous =
        vacuous(state->alpha_compare_1, state->alpha_ref_1);

    if (state->alpha_op == kOpAnd) {
        if (first_vacuous && second_vacuous) {
            return false;
        }
        if (first_vacuous) {
            *out_compare = state->alpha_compare_1;
            *out_reference = state->alpha_ref_1;
            return true;
        }
    } else if (first_vacuous && second_vacuous) {
        return false;
    }
    *out_compare = state->alpha_compare_0;
    *out_reference = state->alpha_ref_0;
    return true;
}

extern "C" size_t melee_host_gx_captured_tev_state_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return captured_tev_states.size();
}

extern "C" bool melee_host_gx_captured_tev_state_at(
    size_t index, MeleeHostGxTevState* output)
{
    if (output == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= captured_tev_states.size()) {
        return false;
    }
    *output = captured_tev_states[index];
    return true;
}

extern "C" size_t melee_host_gx_captured_draw_state_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return captured_draw_states.size();
}

extern "C" bool melee_host_gx_captured_draw_state_at(
    size_t index, MeleeHostGxDrawState* output)
{
    if (output == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= captured_draw_states.size()) {
        return false;
    }
    *output = captured_draw_states[index];
    return true;
}

extern "C" size_t melee_host_gx_triangle_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return triangles.size();
}

extern "C" bool melee_host_gx_triangle_at(size_t index,
                                           MeleeHostGxTriangle* output)
{
    if (output == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= triangles.size()) {
        return false;
    }
    *output = triangles[index];
    return true;
}

extern "C" bool melee_host_gx_captured_triangle_at(
    size_t index, MeleeHostGxCapturedTriangle* output)
{
    if (output == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= captured_triangle_indices.size()) {
        return false;
    }
    const auto& indices = captured_triangle_indices[index];
    for (std::size_t vertex = 0; vertex < indices.size(); ++vertex) {
        if (indices[vertex] >= captured_vertices.size()) {
            return false;
        }
        output->vertices[vertex] = captured_vertices[indices[vertex]];
    }
    return true;
}

extern "C" size_t melee_host_gx_captured_vertex_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return captured_vertices.size();
}

extern "C" bool melee_host_gx_captured_vertex_at(
    size_t index, MeleeHostGxCapturedVertex* output)
{
    if (output == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= captured_vertices.size()) {
        return false;
    }
    *output = captured_vertices[index];
    return true;
}

extern "C" size_t melee_host_gx_display_list_error_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return display_list_errors;
}

extern "C" void melee_host_gx_transform_vertices(
    size_t first, size_t count, const MeleeHostGxAffineTransform* matrix)
{
    if (matrix == nullptr) {
        return;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (first > captured_vertices.size() ||
        count > captured_vertices.size() - first)
    {
        return;
    }
    for (size_t index = first; index < first + count; ++index) {
        transform_vertex_locked(captured_vertices[index], *matrix);
    }
    for (size_t index = 0; index < triangles.size(); ++index) {
        const auto& indices = captured_triangle_indices[index];
        triangles[index] = { { captured_vertices[indices[0]].position,
                               captured_vertices[indices[1]].position,
                               captured_vertices[indices[2]].position } };
    }
}

extern "C" void melee_host_gx_apply_material(size_t first, size_t count,
                                               const mh_u8 diffuse[4],
                                               mh_u32 texture_image,
                                               mh_u32 render_mode)
{
    if (diffuse == nullptr) {
        return;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (first > captured_vertices.size() ||
        count > captured_vertices.size() - first)
    {
        return;
    }
    TextureSet set{};
    set.fill(MELEE_HOST_GX_NO_TEXTURE);
    set[0] = texture_image;
    const mh_u32 set_id = texture_set_id_locked(set);
    for (size_t index = first; index < first + count; ++index) {
        auto& vertex = captured_vertices[index];
        for (size_t channel = 0; channel < 4; ++channel) {
            const mh_u8 source =
                (vertex.attributes & MELEE_HOST_GX_VERTEX_COLOR) != 0
                    ? vertex.color[channel]
                    : static_cast<mh_u8>(255);
            vertex.color[channel] = static_cast<mh_u8>(
                (static_cast<mh_u16>(source) * diffuse[channel] + 127U) / 255U);
        }
        vertex.attributes |= MELEE_HOST_GX_VERTEX_COLOR;
        /* The schema's material stands in for lighting and for the texture
         * binding: both rasterized colours take the modulated colour, and
         * map 0 takes the texture. */
        for (size_t raster = 0; raster < 2; ++raster) {
            for (size_t component = 0; component < 4; ++component) {
                vertex.raster_color[raster][component] =
                    vertex.color[component];
            }
        }
        if (texture_image != MELEE_HOST_GX_NO_TEXTURE) {
            vertex.attributes |= MELEE_HOST_GX_VERTEX_TEXTURE_IMAGE;
            vertex.texture_image = texture_image;
        }
        vertex.texture_set = set_id;
        vertex.render_mode = render_mode;
    }
}

extern "C" size_t melee_host_gx_command_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return commands.size();
}

extern "C" bool melee_host_gx_command_at(size_t index,
                                           MeleeHostGxCommand* output)
{
    if (output == nullptr) {
        return false;
    }

    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= commands.size()) {
        return false;
    }
    *output = commands[index];
    return true;
}
