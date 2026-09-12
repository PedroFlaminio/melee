#ifndef MELEE_HOST_DOLPHIN_TIME_H
#define MELEE_HOST_DOLPHIN_TIME_H

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
