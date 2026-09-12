#ifndef MELEE_HOST_MENU_NATIVE_H
#define MELEE_HOST_MENU_NATIVE_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MeleeHostNativeMenuEvent {
    MELEE_HOST_NATIVE_MENU_UP = 1U << 0U,
    MELEE_HOST_NATIVE_MENU_DOWN = 1U << 1U,
    MELEE_HOST_NATIVE_MENU_LEFT = 1U << 2U,
    MELEE_HOST_NATIVE_MENU_RIGHT = 1U << 3U,
    MELEE_HOST_NATIVE_MENU_CONFIRM = 1U << 4U,
    MELEE_HOST_NATIVE_MENU_BACK = 1U << 5U,
    MELEE_HOST_NATIVE_MENU_L_TRIGGER = 1U << 6U,
    MELEE_HOST_NATIVE_MENU_R_TRIGGER = 1U << 7U,
    MELEE_HOST_NATIVE_MENU_START = 1U << 8U,
    MELEE_HOST_NATIVE_MENU_A = 1U << 9U,
    MELEE_HOST_NATIVE_MENU_X = 1U << 10U,
    MELEE_HOST_NATIVE_MENU_Y = 1U << 11U,
} MeleeHostNativeMenuEvent;

/* Initializes the original HSD pad state and menu controller evaluator. */
MeleeHostStatus melee_host_native_menu_init(void);

/* Converts the current host PAD snapshot through HSD Pad and the original
 * gm_1A36 evaluator. The final event bit mapping matches mn_80229624 without
 * loading the unported visual-menu graph. `slot` accepts 0..3 or 4 for the
 * all-controllers aggregate. */
MeleeHostStatus melee_host_native_menu_update(mh_u32 slot,
                                              mh_u32* out_menu_events);

#ifdef __cplusplus
}
#endif

#endif
