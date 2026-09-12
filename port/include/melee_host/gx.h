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
};

typedef struct MeleeHostGxCapturedVertex {
    mh_u32 attributes;
    MeleeHostGxPosition3f32 position;
    MeleeHostGxPosition3f32 normal;
    mh_u8 color[4];
    mh_f32 texcoord[2];
} MeleeHostGxCapturedVertex;

typedef struct MeleeHostGxTriangle {
    MeleeHostGxPosition3f32 vertices[3];
} MeleeHostGxTriangle;

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

void melee_host_gx_reset_command_log(void);
size_t melee_host_gx_command_count(void);
bool melee_host_gx_command_at(size_t index, MeleeHostGxCommand* output);
size_t melee_host_gx_triangle_count(void);
bool melee_host_gx_triangle_at(size_t index, MeleeHostGxTriangle* output);
size_t melee_host_gx_captured_vertex_count(void);
bool melee_host_gx_captured_vertex_at(size_t index,
                                      MeleeHostGxCapturedVertex* output);

#ifdef __cplusplus
}
#endif

#endif
