#include <melee_host/dolphin_os.h>

// The GameCube API uses this state to guard short critical sections. Native
// code must never emulate CPU interrupts; it only preserves the observable,
// per-thread enable/restore contract until the scheduler owns callbacks.
static _Thread_local BOOL interrupts_enabled = TRUE;

BOOL OSEnableInterrupts(void)
{
    const BOOL previous = interrupts_enabled;
    interrupts_enabled = TRUE;
    return previous;
}

BOOL OSDisableInterrupts(void)
{
    const BOOL previous = interrupts_enabled;
    interrupts_enabled = FALSE;
    return previous;
}

BOOL OSRestoreInterrupts(BOOL level)
{
    const BOOL previous = interrupts_enabled;
    interrupts_enabled = level != FALSE ? TRUE : FALSE;
    return previous;
}

