#ifndef _DOLPHIN_GX_GXVERT_H_
#define _DOLPHIN_GX_GXVERT_H_

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GXFIFO_ADDR 0xCC008000

#if !defined(MELEE_HOST)
typedef union
{
    u8  u8;
    u16 u16;
    u32 u32;
    u64 u64;
    s8  s8;
    s16 s16;
    s32 s32;
    s64 s64;
    f32 f32;
    f64 f64;
} PPCWGPipe;
#endif

#if defined(MELEE_HOST)
#include <melee_host/gx.h>
#elif defined(__MWERKS__) && !defined(M2CTX)
volatile PPCWGPipe GXWGFifo : GXFIFO_ADDR;
#else
#define GXWGFifo (*(volatile PPCWGPipe *)GXFIFO_ADDR)
#endif

#if defined(MELEE_HOST)

#define MELEE_HOST_GX_WRITE_u8(x) melee_host_gx_submit_u8((u8) (x))
#define MELEE_HOST_GX_WRITE_u16(x) melee_host_gx_submit_u16((u16) (x))
#define MELEE_HOST_GX_WRITE_u32(x) melee_host_gx_submit_u32((u32) (x))
#define MELEE_HOST_GX_WRITE_s8(x) melee_host_gx_submit_u8((u8) (x))
#define MELEE_HOST_GX_WRITE_s16(x) melee_host_gx_submit_u16((u16) (x))
#define MELEE_HOST_GX_WRITE_s32(x) melee_host_gx_submit_u32((u32) (x))
#define MELEE_HOST_GX_WRITE_f32(x) melee_host_gx_submit_f32((f32) (x))

#define FUNC_1PARAM(name, T)                                                  \
    static inline void name##1##T(T x) { MELEE_HOST_GX_WRITE_##T(x); }
#define FUNC_2PARAM(name, T)                                                  \
    static inline void name##2##T(T x, T y)                                   \
    {                                                                          \
        MELEE_HOST_GX_WRITE_##T(x);                                           \
        MELEE_HOST_GX_WRITE_##T(y);                                           \
    }
#define FUNC_3PARAM(name, T)                                                  \
    static inline void name##3##T(T x, T y, T z)                              \
    {                                                                          \
        MELEE_HOST_GX_WRITE_##T(x);                                           \
        MELEE_HOST_GX_WRITE_##T(y);                                           \
        MELEE_HOST_GX_WRITE_##T(z);                                           \
    }
#define FUNC_4PARAM(name, T)                                                  \
    static inline void name##4##T(T x, T y, T z, T w)                         \
    {                                                                          \
        MELEE_HOST_GX_WRITE_##T(x);                                           \
        MELEE_HOST_GX_WRITE_##T(y);                                           \
        MELEE_HOST_GX_WRITE_##T(z);                                           \
        MELEE_HOST_GX_WRITE_##T(w);                                           \
    }
#define FUNC_INDEX8(name)                                                     \
    static inline void name##1x8(u8 x) { melee_host_gx_submit_u8(x); }
#define FUNC_INDEX16(name)                                                    \
    static inline void name##1x16(u16 x) { melee_host_gx_submit_u16(x); }

#elif DEBUG

// external functions

#define FUNC_1PARAM(name, T) void name##1##T(T x);
#define FUNC_2PARAM(name, T) void name##2##T(T x, T y);
#define FUNC_3PARAM(name, T) void name##3##T(T x, T y, T z);
#define FUNC_4PARAM(name, T) void name##4##T(T x, T y, T z, T w);
#define FUNC_INDEX8(name)    void name##1x8(u8 x);
#define FUNC_INDEX16(name)   void name##1x16(u16 x);

#else

// inline functions

#define FUNC_1PARAM(name, T) \
static inline void name##1##T(T x) { GXWGFifo.T = x; }

#define FUNC_2PARAM(name, T) \
static inline void name##2##T(T x, T y) { GXWGFifo.T = x; GXWGFifo.T = y; }

#define FUNC_3PARAM(name, T) \
static inline void name##3##T(T x, T y, T z) { GXWGFifo.T = x; GXWGFifo.T = y; GXWGFifo.T = z; }

#define FUNC_4PARAM(name, T) \
static inline void name##4##T(T x, T y, T z, T w) { GXWGFifo.T = x; GXWGFifo.T = y; GXWGFifo.T = z; GXWGFifo.T = w; }

#define FUNC_INDEX8(name) \
static inline void name##1x8(u8 x) { GXWGFifo.u8 = x; }

#define FUNC_INDEX16(name) \
static inline void name##1x16(u16 x) { GXWGFifo.u16 = x; }

#endif

// GXCmd
FUNC_1PARAM(GXCmd, u8)
FUNC_1PARAM(GXCmd, u16)
FUNC_1PARAM(GXCmd, u32)

// GXParam
FUNC_1PARAM(GXParam, u8)
FUNC_1PARAM(GXParam, u16)
FUNC_1PARAM(GXParam, u32)
FUNC_1PARAM(GXParam, s8)
FUNC_1PARAM(GXParam, s16)
FUNC_1PARAM(GXParam, s32)
FUNC_1PARAM(GXParam, f32)
FUNC_3PARAM(GXParam, f32)
FUNC_4PARAM(GXParam, f32)

// GXPosition
#if defined(MELEE_HOST)
static inline void GXPosition3f32(f32 x, f32 y, f32 z)
{
    melee_host_gx_submit_position3f32(x, y, z);
}
#else
FUNC_3PARAM(GXPosition, f32)
#endif
FUNC_3PARAM(GXPosition, u8)
FUNC_3PARAM(GXPosition, s8)
FUNC_3PARAM(GXPosition, u16)
FUNC_3PARAM(GXPosition, s16)
FUNC_2PARAM(GXPosition, f32)
FUNC_2PARAM(GXPosition, u8)
FUNC_2PARAM(GXPosition, s8)
FUNC_2PARAM(GXPosition, u16)
FUNC_2PARAM(GXPosition, s16)
FUNC_INDEX16(GXPosition)
FUNC_INDEX8(GXPosition)

// GXNormal
#if defined(MELEE_HOST)
static inline void GXNormal3f32(f32 x, f32 y, f32 z)
{
    melee_host_gx_submit_normal3f32(x, y, z);
}
#else
FUNC_3PARAM(GXNormal, f32)
#endif
FUNC_3PARAM(GXNormal, s16)
FUNC_3PARAM(GXNormal, s8)
FUNC_INDEX16(GXNormal)
FUNC_INDEX8(GXNormal)

// GXColor
#if defined(MELEE_HOST)
static inline void GXColor4u8(u8 red, u8 green, u8 blue, u8 alpha)
{
    melee_host_gx_submit_color4u8(red, green, blue, alpha);
}
#else
FUNC_4PARAM(GXColor, u8)
#endif
FUNC_1PARAM(GXColor, u32)
FUNC_3PARAM(GXColor, u8)
FUNC_1PARAM(GXColor, u16)
FUNC_INDEX16(GXColor)
FUNC_INDEX8(GXColor)

// GXTexCoord
#if defined(MELEE_HOST)
static inline void GXTexCoord2f32(f32 s, f32 t)
{
    melee_host_gx_submit_texcoord2f32(s, t);
}
#else
FUNC_2PARAM(GXTexCoord, f32)
#endif
FUNC_2PARAM(GXTexCoord, s16)
FUNC_2PARAM(GXTexCoord, u16)
FUNC_2PARAM(GXTexCoord, s8)
FUNC_2PARAM(GXTexCoord, u8)
FUNC_1PARAM(GXTexCoord, f32)
FUNC_1PARAM(GXTexCoord, s16)
FUNC_1PARAM(GXTexCoord, u16)
FUNC_1PARAM(GXTexCoord, s8)
FUNC_1PARAM(GXTexCoord, u8)
FUNC_INDEX16(GXTexCoord)
FUNC_INDEX8(GXTexCoord)

// GXMatrixIndex
FUNC_1PARAM(GXMatrixIndex, u8)

#if !defined(M2CTX) // undef not supported
#undef FUNC_1PARAM
#undef FUNC_2PARAM
#undef FUNC_3PARAM
#undef FUNC_4PARAM
#undef FUNC_INDEX8
#undef FUNC_INDEX16
#if defined(MELEE_HOST)
#undef MELEE_HOST_GX_WRITE_u8
#undef MELEE_HOST_GX_WRITE_u16
#undef MELEE_HOST_GX_WRITE_u32
#undef MELEE_HOST_GX_WRITE_s8
#undef MELEE_HOST_GX_WRITE_s16
#undef MELEE_HOST_GX_WRITE_s32
#undef MELEE_HOST_GX_WRITE_f32
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
