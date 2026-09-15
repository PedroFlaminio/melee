#ifndef MELEE_HOST_DOLPHIN_OS_H
#define MELEE_HOST_DOLPHIN_OS_H

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL OSEnableInterrupts(void);
BOOL OSDisableInterrupts(void);
BOOL OSRestoreInterrupts(BOOL level);

/* Runs every OSAlarm whose time has come, in order of fire time.  The console
 * delivers alarms by interrupt; the host calls this where it hands control
 * back to game code: the disc wait and the frame boundary. */
void melee_host_os_fire_alarms(void);

/* The fire time of the earliest armed alarm, in OSGetTime ticks.  Returns
 * FALSE when no alarm is armed. */
BOOL melee_host_os_next_alarm(s64* out_fire);

/* The fast cast behind OSf32tou16: the console stores the float with psq_st
 * through QR3, which OSInitFastCast sets to u16 with no scale.  The value
 * saturates at the u16 range and the fraction is dropped; NaN stores 0. */
static inline u16 melee_host_os_f32_to_u16(f32 value)
{
    if (!(value > 0.0F)) {
        return 0;
    }
    if (value >= 65535.0F) {
        return 0xFFFF;
    }
    return (u16) value;
}

#ifdef __cplusplus
}
#endif

#endif

