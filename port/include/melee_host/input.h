#ifndef MELEE_HOST_INPUT_H
#define MELEE_HOST_INPUT_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { MELEE_HOST_MAX_CONTROLLERS = 4 };

/*
 * Backend-neutral GameCube-style controller sample.  A platform backend maps
 * physical controls into this representation; game code observes a snapshot
 * that changes only at the beginning of a host simulation tick.
 */
typedef struct MeleeHostPadState {
    mh_u16 buttons;
    mh_s8 stick_x;
    mh_s8 stick_y;
    mh_s8 c_stick_x;
    mh_s8 c_stick_y;
    mh_u8 trigger_left;
    mh_u8 trigger_right;
    bool connected;
} MeleeHostPadState;

typedef struct MeleeHostInputSnapshot {
    mh_u64 tick;
    MeleeHostPadState pads[MELEE_HOST_MAX_CONTROLLERS];
} MeleeHostInputSnapshot;

/* PAD is process-global on GameCube. Select the context served by PADRead. */
MeleeHostStatus melee_host_activate_pad_backend(MeleeHostContext* context);
MeleeHostStatus melee_host_submit_pad_state(MeleeHostContext* context,
                                            mh_u32 port,
                                            const MeleeHostPadState* state);
MeleeHostStatus melee_host_input_snapshot(const MeleeHostContext* context,
                                          MeleeHostInputSnapshot* out_snapshot);

#ifdef __cplusplus
}
#endif

#endif
