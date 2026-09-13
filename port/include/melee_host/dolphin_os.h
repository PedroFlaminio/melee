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

#ifdef __cplusplus
}
#endif

#endif

