#ifndef MELEE_HOST_INPUT_H
#define MELEE_HOST_INPUT_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { MELEE_HOST_MAX_CONTROLLERS = 4 };

enum {
    MELEE_HOST_MENU_UP = 1 << 0,
    MELEE_HOST_MENU_DOWN = 1 << 1,
    MELEE_HOST_MENU_LEFT = 1 << 2,
    MELEE_HOST_MENU_RIGHT = 1 << 3,
    MELEE_HOST_MENU_CONFIRM = 1 << 4,
    MELEE_HOST_MENU_BACK = 1 << 5,
    MELEE_HOST_MENU_L_TRIGGER = 1 << 6,
    MELEE_HOST_MENU_R_TRIGGER = 1 << 7,
    MELEE_HOST_MENU_START = 1 << 8,
    MELEE_HOST_MENU_A = 1 << 9,
    MELEE_HOST_MENU_X = 1 << 10,
    MELEE_HOST_MENU_Y = 1 << 11,
};

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

typedef struct MeleeHostMenuInputFilter {
    mh_u16 previous;
    mh_u16 directional_held;
} MeleeHostMenuInputFilter;

/* PAD is process-global on GameCube. Select the context served by PADRead. */
MeleeHostStatus melee_host_activate_pad_backend(MeleeHostContext* context);
MeleeHostStatus melee_host_submit_pad_state(MeleeHostContext* context,
                                            mh_u32 port,
                                            const MeleeHostPadState* state);
MeleeHostStatus melee_host_input_snapshot(const MeleeHostContext* context,
                                          MeleeHostInputSnapshot* out_snapshot);
mh_u16 melee_host_menu_input_from_pad(const MeleeHostPadState* state);
void melee_host_menu_input_filter_reset(MeleeHostMenuInputFilter* filter);
mh_u16 melee_host_menu_input_filter_update(MeleeHostMenuInputFilter* filter,
                                           mh_u16 held_input);

#ifdef __cplusplus
}
#endif

#endif
