#ifndef MELEE_HOST_GX_H
#define MELEE_HOST_GX_H

#include <melee_host/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MeleeHostGxValueType {
    MELEE_HOST_GX_U8 = 0,
    MELEE_HOST_GX_U16 = 1,
    MELEE_HOST_GX_U32 = 2,
    MELEE_HOST_GX_F32 = 3,
} MeleeHostGxValueType;

typedef struct MeleeHostGxCommand {
    MeleeHostGxValueType type;
    mh_u32 bits;
} MeleeHostGxCommand;

typedef struct MeleeHostGxPosition3f32 {
    mh_f32 x;
    mh_f32 y;
    mh_f32 z;
} MeleeHostGxPosition3f32;

enum {
    MELEE_HOST_GX_VERTEX_POSITION = 1 << 0,
    MELEE_HOST_GX_VERTEX_NORMAL = 1 << 1,
    MELEE_HOST_GX_VERTEX_COLOR = 1 << 2,
    MELEE_HOST_GX_VERTEX_TEXCOORD = 1 << 3,
    MELEE_HOST_GX_VERTEX_TANGENT = 1 << 4,
    MELEE_HOST_GX_VERTEX_BINORMAL = 1 << 5,
    MELEE_HOST_GX_VERTEX_TEXTURE_IMAGE = 1 << 6,
    MELEE_HOST_GX_NO_TEXTURE = 0xFFFFFFFFU,
};

/* The pixel state a draw ran under: what GX was told would decide how the
 * triangle reaches the framebuffer.  Kept separate from the full pixel state
 * because these are the fields that change the image, and a consumer has to
 * compare them to group draws. */
typedef struct MeleeHostGxDrawState {
    mh_u32 cull_mode;
    bool z_compare_enable;
    bool z_update_enable;
    mh_u32 z_func;
    mh_u32 blend_mode;
    mh_u32 blend_src_factor;
    mh_u32 blend_dst_factor;
    mh_u32 blend_logic_op;
    bool color_update_enable;
    bool alpha_update_enable;
    mh_u32 alpha_compare_0;
    mh_u32 alpha_compare_1;
    mh_u32 alpha_op;
    mh_u8 alpha_ref_0;
    mh_u8 alpha_ref_1;
} MeleeHostGxDrawState;

typedef struct MeleeHostGxCapturedVertex {
    mh_u32 attributes;
    MeleeHostGxPosition3f32 position;
    MeleeHostGxPosition3f32 normal;
    MeleeHostGxPosition3f32 tangent;
    MeleeHostGxPosition3f32 binormal;
    mh_u8 color[4];
    /* GX evaluates COLOR0A0 and COLOR1A1 independently before TEV.  `color`
     * remains the source vertex colour; these are the two raster colours the
     * TEV stages select through GXSetTevOrder. */
    mh_u8 raster_color[2][4];
    mh_f32 texcoord[2];
    mh_u32 texture_image;
    mh_u32 render_mode;
    /* Index into the captured draw-state table. */
    mh_u32 draw_state;
    /* Index into the captured TEV-state table. */
    mh_u32 tev_state;
} MeleeHostGxCapturedVertex;

typedef struct MeleeHostGxTriangle {
    MeleeHostGxPosition3f32 vertices[3];
} MeleeHostGxTriangle;

typedef struct MeleeHostGxCapturedTriangle {
    MeleeHostGxCapturedVertex vertices[3];
} MeleeHostGxCapturedTriangle;

typedef struct MeleeHostGxAffineTransform {
    mh_f32 values[3][4];
} MeleeHostGxAffineTransform;

void melee_host_gx_submit_u8(mh_u8 value);
void melee_host_gx_submit_u16(mh_u16 value);
void melee_host_gx_submit_u32(mh_u32 value);
void melee_host_gx_submit_f32(mh_f32 value);
void melee_host_gx_begin(mh_u8 primitive, mh_u8 vertex_format,
                         mh_u16 vertex_count);
void melee_host_gx_end(void);
void melee_host_gx_submit_position3f32(mh_f32 x, mh_f32 y, mh_f32 z);
void melee_host_gx_submit_normal3f32(mh_f32 x, mh_f32 y, mh_f32 z);
void melee_host_gx_submit_color4u8(mh_u8 red, mh_u8 green, mh_u8 blue,
                                   mh_u8 alpha);
void melee_host_gx_submit_texcoord2f32(mh_f32 s, mh_f32 t);
void melee_host_gx_submit_position_index8(mh_u8 index);
void melee_host_gx_submit_position_index16(mh_u16 index);
void melee_host_gx_submit_normal_index8(mh_u8 index);
void melee_host_gx_submit_normal_index16(mh_u16 index);
void melee_host_gx_submit_color_index8(mh_u8 index);
void melee_host_gx_submit_color_index16(mh_u16 index);
void melee_host_gx_submit_texcoord_index8(mh_u8 index);
void melee_host_gx_submit_texcoord_index16(mh_u16 index);
void melee_host_gx_set_array_bounded(mh_u32 attribute, const void* base,
                                     size_t byte_length, mh_u8 stride);

/* Declares a host-owned region and its extent, so a vertex array the original
 * GXSetArray points into can be bounded even though that call carries no
 * length.  The archive payload is such a region: the console let an array run
 * into whatever followed it, and the host stops at the end of the file. */
void melee_host_gx_register_array_region(const void* base,
                                         size_t byte_length);
void melee_host_gx_unregister_array_region(const void* base);

void melee_host_gx_reset_command_log(void);
size_t melee_host_gx_command_count(void);
bool melee_host_gx_command_at(size_t index, MeleeHostGxCommand* output);
size_t melee_host_gx_triangle_count(void);
bool melee_host_gx_triangle_at(size_t index, MeleeHostGxTriangle* output);
bool melee_host_gx_captured_triangle_at(
    size_t index, MeleeHostGxCapturedTriangle* output);
size_t melee_host_gx_captured_vertex_count(void);
bool melee_host_gx_captured_vertex_at(size_t index,
                                      MeleeHostGxCapturedVertex* output);
/* Distinct pixel states the captured draws ran under, in first-use order.  A
 * captured vertex carries an index into this table, which is how a consumer
 * groups triangles that share state instead of guessing at fixed passes. */
/* Reduces GX's pair of alpha comparisons to the single one a fixed-function
 * alpha test can express, and reports whether a test is needed at all.
 *
 * GX compares alpha twice and combines the results.  A comparison that is true
 * for every possible alpha carries no information: GX_ALWAYS, but also
 * `<= 255` and `>= 0`, which is how the game writes an open-ended side.  Under
 * GX_AOP_AND such a side can be dropped, and two identical comparisons collapse
 * to one.  That covers every combination the game's materials use.  Anything
 * else keeps the first comparison, which is an approximation, and the return
 * value cannot tell you so - compare the state yourself if that matters. */
bool melee_host_gx_resolve_alpha_test(const MeleeHostGxDrawState* state,
                                      mh_u32* out_compare,
                                      mh_u8* out_reference);

size_t melee_host_gx_captured_draw_state_count(void);
bool melee_host_gx_captured_draw_state_at(size_t index,
                                          MeleeHostGxDrawState* output);

size_t melee_host_gx_display_list_error_count(void);
/* Indexed vertex reads the host refused because they fell outside a bounded
 * array.  The attribute is dropped when that happens, so this has to be read
 * alongside the geometry counts to know the capture is complete. */
size_t melee_host_gx_rejected_index_count(void);
void melee_host_gx_transform_vertices(size_t first, size_t count,
                                      const MeleeHostGxAffineTransform* matrix);
void melee_host_gx_apply_material(size_t first, size_t count,
                                  const mh_u8 diffuse[4],
                                  mh_u32 texture_image, mh_u32 render_mode);

/* ---------------------------------------------------------------------------
 * GX pipeline state.
 *
 * The host models the state the GX API exposes, not the Flipper hardware.
 * Every GX state-setting entry point records into the block below, and
 * the renderer and tests read it back through these accessors.  Enumerated
 * values are carried as plain integers so this ABI stays free of the Dolphin
 * GX headers.
 * ------------------------------------------------------------------------- */

enum {
    MELEE_HOST_GX_MAX_TEXMAP = 8,
    MELEE_HOST_GX_MAX_TEXCOORD = 8,
    MELEE_HOST_GX_MAX_TEVSTAGE = 16,
    MELEE_HOST_GX_MAX_CHANNEL = 4,
    MELEE_HOST_GX_MAX_LIGHT = 8,
    MELEE_HOST_GX_MATRIX_ROWS = 64,
    MELEE_HOST_GX_MAX_TLUT = 20,
    MELEE_HOST_GX_MAX_TEVREG = 4,
    MELEE_HOST_GX_MAX_KCOLOR = 4,
    MELEE_HOST_GX_FOG_ADJ_ENTRIES = 10,
};

typedef struct MeleeHostGxPixelState {
    bool z_compare_enable;
    bool z_update_enable;
    mh_u32 z_func;
    bool z_before_texture;
    mh_u32 z_texture_op;
    mh_u32 z_texture_format;
    mh_u32 z_texture_bias;
    mh_u32 blend_mode;
    mh_u32 blend_src_factor;
    mh_u32 blend_dst_factor;
    mh_u32 blend_logic_op;
    bool color_update_enable;
    bool alpha_update_enable;
    mh_u32 alpha_compare_0;
    mh_u32 alpha_compare_1;
    mh_u32 alpha_op;
    mh_u8 alpha_ref_0;
    mh_u8 alpha_ref_1;
    mh_u32 cull_mode;
    mh_u32 scissor_left;
    mh_u32 scissor_top;
    mh_u32 scissor_width;
    mh_u32 scissor_height;
    /* Counts GXPixModeSync calls, which the game uses to fence EFB reads. */
    mh_u32 pixel_sync_count;
    bool dither_enabled;
    bool destination_alpha_enabled;
    mh_u8 destination_alpha;
    mh_u32 pixel_format;
    mh_u32 depth_format;
    mh_u8 line_width;
    mh_u32 line_texture_offsets;
    mh_u8 point_size;
    mh_u32 point_texture_offsets;
    bool field_mode;
    bool half_aspect_ratio;
} MeleeHostGxPixelState;

typedef struct MeleeHostGxTransformState {
    mh_f32 viewport_left;
    mh_f32 viewport_top;
    mh_f32 viewport_width;
    mh_f32 viewport_height;
    mh_f32 viewport_near;
    mh_f32 viewport_far;
    mh_u32 viewport_field;
    /* Projection is kept in the six-float GX form: the type followed by the
     * five values GXGetProjectionv reports back. */
    mh_u32 projection_type;
    mh_f32 projection[6];
    mh_u32 current_matrix;
} MeleeHostGxTransformState;

typedef struct MeleeHostGxTevStage {
    /* The mode GXSetTevOp last applied, or the sentinel below when the stage
     * was configured through the individual input and output calls. */
    mh_u32 mode;
    mh_u32 texcoord;
    mh_u32 texmap;
    mh_u32 color_channel;
    mh_u32 color_input[4];
    mh_u32 alpha_input[4];
    mh_u32 color_op;
    mh_u32 color_bias;
    mh_u32 color_scale;
    mh_u32 color_out_reg;
    bool color_clamp;
    mh_u32 alpha_op;
    mh_u32 alpha_bias;
    mh_u32 alpha_scale;
    mh_u32 alpha_out_reg;
    bool alpha_clamp;
    mh_u32 konst_color_select;
    mh_u32 konst_alpha_select;
    mh_u32 raster_swap;
    mh_u32 texture_swap;
} MeleeHostGxTevStage;

enum { MELEE_HOST_GX_TEV_MODE_CUSTOM = 0xFFFFFFFFU };

typedef struct MeleeHostGxTexCoordGen {
    mh_u32 function;
    mh_u32 source;
    mh_u32 matrix;
    bool normalize;
    mh_u32 post_matrix;
} MeleeHostGxTexCoordGen;

typedef struct MeleeHostGxTevState {
    mh_u8 stage_count;
    mh_u8 texcoord_gen_count;
    mh_u8 channel_count;
    MeleeHostGxTevStage stages[MELEE_HOST_GX_MAX_TEVSTAGE];
    MeleeHostGxTexCoordGen texcoord_gens[MELEE_HOST_GX_MAX_TEXCOORD];
    /* TEV output registers are signed 10-bit per component, so the wider
     * GXSetTevColorS10 values survive here without clamping. */
    mh_s16 registers[MELEE_HOST_GX_MAX_TEVREG][4];
    mh_u8 konst_colors[MELEE_HOST_GX_MAX_KCOLOR][4];
} MeleeHostGxTevState;

typedef struct MeleeHostGxChannelControl {
    bool lighting_enabled;
    mh_u32 ambient_source;
    mh_u32 material_source;
    mh_u32 light_mask;
    mh_u32 diffuse_function;
    mh_u32 attenuation_function;
    mh_u8 ambient_color[4];
    mh_u8 material_color[4];
} MeleeHostGxChannelControl;

typedef struct MeleeHostGxTextureDesc {
    bool bound;
    bool color_indexed;
    mh_u16 width;
    mh_u16 height;
    mh_u32 format;
    mh_u32 wrap_s;
    mh_u32 wrap_t;
    bool mipmap;
    mh_u32 tlut_name;
    mh_u32 min_filter;
    mh_u32 mag_filter;
    mh_f32 min_lod;
    mh_f32 max_lod;
    mh_f32 lod_bias;
    mh_u32 max_anisotropy;
    bool bias_clamp;
    bool edge_lod;
    const void* image;
} MeleeHostGxTextureDesc;

typedef struct MeleeHostGxTlutDesc {
    bool loaded;
    mh_u32 format;
    mh_u16 entry_count;
    const void* entries;
} MeleeHostGxTlutDesc;

typedef struct MeleeHostGxLightDesc {
    bool loaded;
    mh_u8 color[4];
    mh_f32 position[3];
    mh_f32 direction[3];
    mh_f32 angle_attenuation[3];
    mh_f32 distance_attenuation[3];
} MeleeHostGxLightDesc;

typedef struct MeleeHostGxFogState {
    mh_u32 type;
    mh_f32 start_z;
    mh_f32 end_z;
    mh_f32 near_z;
    mh_f32 far_z;
    mh_u8 color[4];
    bool range_adjust_enabled;
    mh_u16 range_adjust_center;
    mh_u16 range_adjust_table[MELEE_HOST_GX_FOG_ADJ_ENTRIES];
    /* The range-adjustment table the host builds is neutral: the real SDK
     * derives it from the projection, and that derivation is not modelled
     * yet.  A reader that needs true range adjustment must check this. */
    bool range_adjust_modelled;
} MeleeHostGxFogState;

typedef struct MeleeHostGxDisplayCopyState {
    mh_u16 source_left;
    mh_u16 source_top;
    mh_u16 source_width;
    mh_u16 source_height;
    mh_u16 destination_width;
    mh_u16 destination_height;
    mh_f32 vertical_scale;
    mh_u32 gamma;
    mh_u32 clamp;
    bool antialiasing;
    bool vertical_filter;
    mh_u8 sample_pattern[12][2];
    mh_u8 filter_weights[7];
    mh_u8 clear_color[4];
    mh_u32 clear_depth;
    mh_u32 copy_count;
    const void* last_destination;
    bool last_clear;
} MeleeHostGxDisplayCopyState;

typedef struct MeleeHostGxDrawSyncState {
    /* The host has no asynchronous graphics processor, so a draw-done fence
     * stays pending until something drains it: GXWaitDrawDone, GXDrawDone, or
     * the host frame loop calling melee_host_gx_drain_draw_done.  On hardware
     * the callback can also arrive from interrupt context without a wait,
     * which the host deliberately does not reproduce. */
    bool pending;
    mh_u32 fence_count;
    mh_u32 wait_count;
    mh_u32 callback_count;
} MeleeHostGxDrawSyncState;

typedef struct MeleeHostGxCopyState {
    mh_u16 source_left;
    mh_u16 source_top;
    mh_u16 source_width;
    mh_u16 source_height;
    mh_u16 destination_width;
    mh_u16 destination_height;
    mh_u32 destination_format;
    bool destination_mipmap;
    /* GXCopyTex cannot produce pixels without a framebuffer, so the host
     * records the request instead of writing to the destination. */
    mh_u32 copy_count;
    const void* last_destination;
    bool last_clear;
    mh_u32 texture_invalidate_count;
} MeleeHostGxCopyState;

void melee_host_gx_state_reset(void);
void melee_host_gx_pixel_state(MeleeHostGxPixelState* output);
void melee_host_gx_transform_state(MeleeHostGxTransformState* output);
void melee_host_gx_tev_state(MeleeHostGxTevState* output);
/* Distinct TEV configurations the captured draws ran under, in first-use
 * order.  A captured vertex carries an index into this table, which is what
 * lets a consumer colour a triangle the way its material describes instead of
 * assuming one fixed combination. */
typedef enum MeleeHostGxShadingKind {
    /* texture x rasterized colour, which is what most materials compile to */
    MELEE_HOST_GX_SHADING_TEXTURE_TIMES_COLOR = 0,
    /* a constant colour x rasterized colour, for an untextured material */
    MELEE_HOST_GX_SHADING_KONST_TIMES_COLOR = 1,
    /* the texture alone, ignoring the rasterized colour */
    MELEE_HOST_GX_SHADING_TEXTURE = 2,
    /* the rasterized colour alone */
    MELEE_HOST_GX_SHADING_COLOR = 3,
    /* the program is not one of the forms above; a consumer has to
     * approximate it and should say so */
    MELEE_HOST_GX_SHADING_APPROXIMATED = 4,
} MeleeHostGxShadingKind;

typedef struct MeleeHostGxResolvedShading {
    mh_u32 kind;
    /* The constant colour the KONST form multiplies by. */
    mh_u8 konst_color[4];
    /* What the program multiplies alpha by, and whether the rasterized alpha
     * takes part.  HSD puts a material's alpha in a TEV register and
     * multiplies the vertex alpha by it, which is this pair. */
    mh_u8 constant_alpha;
    bool uses_raster_alpha;
} MeleeHostGxResolvedShading;

/* Reduces a captured TEV program to the colour expression it computes, when
 * that expression is one a consumer can render directly.
 *
 * HSD compiles its material expressions into TEV stages, so nothing here comes
 * from GXSetTevOp presets: the program has to be read to know what it does.
 * Only single-stage programs writing the final register are reduced, and only
 * the forms the game's materials actually use.  Everything else resolves to
 * MELEE_HOST_GX_SHADING_APPROXIMATED, which is not a failure - it means a
 * consumer must pick a stand-in and report that it did. */
bool melee_host_gx_resolve_shading(const MeleeHostGxTevState* tev,
                                   MeleeHostGxResolvedShading* out_shading);

size_t melee_host_gx_captured_tev_state_count(void);
bool melee_host_gx_captured_tev_state_at(size_t index,
                                         MeleeHostGxTevState* output);
bool melee_host_gx_channel_control(mh_u32 channel,
                                   MeleeHostGxChannelControl* output);
/* Textures bound while capturing, in the order they were first used.  A
 * captured vertex carries an index into this table, so a consumer uploads each
 * image once instead of resolving it per triangle. */
size_t melee_host_gx_captured_texture_count(void);
bool melee_host_gx_captured_texture_at(size_t index,
                                       MeleeHostGxTextureDesc* output);

bool melee_host_gx_bound_texture(mh_u32 texmap,
                                 MeleeHostGxTextureDesc* output);
bool melee_host_gx_loaded_tlut(mh_u32 tlut_name, MeleeHostGxTlutDesc* output);
bool melee_host_gx_light(mh_u32 light_index, MeleeHostGxLightDesc* output);
/* Evaluates GX's per-vertex colour channels from the recorded light and
 * channel state.  Position and normal must already be in the same view space
 * as GXLightObj. */
void melee_host_gx_evaluate_lighting(const MeleeHostGxCapturedVertex* vertex,
                                     mh_u8 color0a0[4],
                                     mh_u8 color1a1[4]);
void melee_host_gx_fog_state(MeleeHostGxFogState* output);
void melee_host_gx_copy_state(MeleeHostGxCopyState* output);
void melee_host_gx_display_copy_state(MeleeHostGxDisplayCopyState* output);
void melee_host_gx_draw_sync_state(MeleeHostGxDrawSyncState* output);
/* Delivers a pending draw-done callback from the host frame loop.  Returns
 * true when a fence was outstanding and its callback ran. */
bool melee_host_gx_drain_draw_done(void);
/* Reads a 3x4 matrix out of GX matrix memory at the given row id. */
bool melee_host_gx_matrix(mh_u32 row_id, MeleeHostGxAffineTransform* output);
/* Whether the game actually loaded all three rows of that matrix.  Matrix
 * memory resets to zeros rather than identity, so reading it without asking
 * this would transform geometry by a zero matrix. */
bool melee_host_gx_matrix_loaded(mh_u32 row_id);

#ifdef __cplusplus
}
#endif

#endif
