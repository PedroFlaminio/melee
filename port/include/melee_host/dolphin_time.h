#ifndef MELEE_HOST_DOLPHIN_TIME_H
#define MELEE_HOST_DOLPHIN_TIME_H

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _DOLPHIN_OS_H_
typedef s64 OSTime;
typedef u32 OSTick;

typedef struct OSCalendarTime {
    int sec;
    int min;
    int hour;
    int mday;
    int mon;
    int year;
    int wday;
    int yday;
    int msec;
    int usec;
} OSCalendarTime;

#define OS_BUS_CLOCK 162000000U
#define OS_CORE_CLOCK 486000000U
#define OS_TIMER_CLOCK (OS_BUS_CLOCK / 4U)

#define OSTicksToSeconds(ticks) ((ticks) / OS_TIMER_CLOCK)
#define OSSecondsToTicks(seconds) ((seconds) * OS_TIMER_CLOCK)

OSTick OSGetTick(void);
OSTime OSGetTime(void);
void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime* output);
OSTime OSCalendarTimeToTicks(OSCalendarTime* input);
#endif

/* The OS clock reads the host's clocks until it is frozen.  Frozen, it keeps
 * the time it had and moves only through melee_host_os_time_advance, so a run
 * paced by alarms repeats exactly and never waits on the wall clock.
 * OSGetTime and OSGetTick both follow it. */
void melee_host_os_time_freeze(void);
void melee_host_os_time_thaw(void);
BOOL melee_host_os_time_frozen(void);
/* Moves a frozen clock forward.  A running clock, or a step that is not
 * positive, is left as it is. */
void melee_host_os_time_advance(s64 ticks);

#ifdef __cplusplus
}
#endif

#endif
