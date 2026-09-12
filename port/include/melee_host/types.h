#ifndef MELEE_HOST_TYPES_H
#define MELEE_HOST_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t mh_s8;
typedef uint8_t mh_u8;
typedef int16_t mh_s16;
typedef uint16_t mh_u16;
typedef int32_t mh_s32;
typedef uint32_t mh_u32;
typedef int64_t mh_s64;
typedef uint64_t mh_u64;
typedef float mh_f32;
typedef double mh_f64;

typedef mh_u32 MeleeAssetOffset32;
typedef mh_u64 MeleeRuntimeHandle;

#ifdef __cplusplus
}

static_assert(sizeof(mh_s8) == 1);
static_assert(sizeof(mh_u8) == 1);
static_assert(sizeof(mh_s16) == 2);
static_assert(sizeof(mh_u16) == 2);
static_assert(sizeof(mh_s32) == 4);
static_assert(sizeof(mh_u32) == 4);
static_assert(sizeof(mh_s64) == 8);
static_assert(sizeof(mh_u64) == 8);
static_assert(sizeof(mh_f32) == 4);
static_assert(sizeof(mh_f64) == 8);
#else
_Static_assert(sizeof(mh_s8) == 1, "mh_s8 must be 8-bit");
_Static_assert(sizeof(mh_u8) == 1, "mh_u8 must be 8-bit");
_Static_assert(sizeof(mh_s16) == 2, "mh_s16 must be 16-bit");
_Static_assert(sizeof(mh_u16) == 2, "mh_u16 must be 16-bit");
_Static_assert(sizeof(mh_s32) == 4, "mh_s32 must be 32-bit");
_Static_assert(sizeof(mh_u32) == 4, "mh_u32 must be 32-bit");
_Static_assert(sizeof(mh_s64) == 8, "mh_s64 must be 64-bit");
_Static_assert(sizeof(mh_u64) == 8, "mh_u64 must be 64-bit");
_Static_assert(sizeof(mh_f32) == 4, "mh_f32 must be 32-bit");
_Static_assert(sizeof(mh_f64) == 8, "mh_f64 must be 64-bit");
#endif

#endif

