#include <melee_host/menu_native.h>

#include <dolphin/pad.h>
#include <melee/gm/gm_1A36.h>
#include <sysdolphin/baselib/controller.h>

static HSD_PadData pad_queue[5];
static HSD_PadRumbleListData rumble_list[12];
static bool native_menu_initialized;

MeleeHostStatus melee_host_native_menu_init(void)
{
    HSD_PadInit(5, pad_queue, 12, rumble_list);
    gm_801A3E88();
    native_menu_initialized = true;
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_native_menu_update(mh_u32 slot,
                                              mh_u32* out_menu_events)
{
    if (out_menu_events == NULL || slot > PAD_MAX_CONTROLLERS) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (!native_menu_initialized) {
        return MELEE_HOST_NOT_READY;
    }
    HSD_PadRenewStatus();
    gm_EvaluateAllControllerInputs();
    {
        const u64 repeated = gm_801A36C0((u8) slot);
        const u64 triggered = gm_GetButtonsTriggered((u8) slot);
        mh_u32 events = 0;

        if ((triggered & PAD_BUTTON_A) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_A;
        }
        if ((triggered & PAD_BUTTON_START) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_START;
        }
        if ((triggered & PAD_CONFIRM) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_CONFIRM;
        }
        if ((triggered & PAD_CANCEL) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_BACK;
        }
        if ((triggered & PAD_TRIGGER_L) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_L_TRIGGER;
        }
        if ((triggered & PAD_TRIGGER_R) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_R_TRIGGER;
        }
        if ((triggered & PAD_BUTTON_X) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_X;
        }
        if ((triggered & PAD_BUTTON_Y) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_Y;
        }
        if ((repeated & PAD_ANY_UP) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_UP;
        }
        if ((repeated & PAD_ANY_DOWN) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_DOWN;
        }
        if ((repeated & PAD_ANY_LEFT) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_LEFT;
        }
        if ((repeated & PAD_ANY_RIGHT) != 0) {
            events |= MELEE_HOST_NATIVE_MENU_RIGHT;
        }
        *out_menu_events = events;
    }
    return MELEE_HOST_OK;
}
