#ifndef MELEE_HOST_DOLPHIN_OS_H
#define MELEE_HOST_DOLPHIN_OS_H

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL OSEnableInterrupts(void);
BOOL OSDisableInterrupts(void);
BOOL OSRestoreInterrupts(BOOL level);

#ifdef __cplusplus
}
#endif

#endif

