#ifndef MELEE_HOST_MATCH_TRACE_H
#define MELEE_HOST_MATCH_TRACE_H

#include <melee_host/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A narrow C ABI for observing the live VS scene without exposing the
 * PowerPC-layout fighter headers to host C++ tools. */
bool melee_host_match_fighter_position(mh_u32 slot, mh_f32* x, mh_f32* y,
                                        mh_f32* z);
bool melee_host_match_fighter_motion(mh_u32 slot, mh_s32* motion);

#ifdef __cplusplus
}
#endif

#endif
