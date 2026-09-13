/* The SDK's alarms, driven by the host clock.
 *
 * On the console an alarm fires from the decrementer interrupt when its time
 * comes, interrupting whatever the game was doing.  The host has no such
 * interrupt, so alarms fire where the host already hands control back to game
 * code: while the game waits for the disc and at a frame boundary.  They fire
 * in order of their fire time, and a periodic alarm is armed again before its
 * handler runs, as the SDK's InsertAlarm does.
 */

#include <melee_host/dolphin_os.h>

#include <dolphin/os/OSAlarm.h>

#include <stddef.h>

static OSAlarm* alarm_head;
static OSAlarm* alarm_tail;

static void unlink_alarm(OSAlarm* alarm)
{
    if (alarm->prev != NULL) {
        alarm->prev->next = alarm->next;
    } else {
        alarm_head = alarm->next;
    }
    if (alarm->next != NULL) {
        alarm->next->prev = alarm->prev;
    } else {
        alarm_tail = alarm->prev;
    }
    alarm->prev = NULL;
    alarm->next = NULL;
}

static void insert_alarm(OSAlarm* alarm, OSTime fire, OSAlarmHandler handler)
{
    OSAlarm* next;

    if (alarm->period > 0) {
        /* The next multiple of the period after now, counted from start. */
        const OSTime now = OSGetTime();
        fire = alarm->start;
        if (alarm->start < now) {
            fire += alarm->period *
                    ((now - alarm->start) / alarm->period + 1);
        }
    }
    alarm->handler = handler;
    alarm->fire = fire;

    for (next = alarm_head; next != NULL; next = next->next) {
        if (fire < next->fire) {
            break;
        }
    }
    alarm->next = next;
    if (next != NULL) {
        alarm->prev = next->prev;
        next->prev = alarm;
    } else {
        alarm->prev = alarm_tail;
        alarm_tail = alarm;
    }
    if (alarm->prev != NULL) {
        alarm->prev->next = alarm;
    } else {
        alarm_head = alarm;
    }
}

void OSInitAlarm(void)
{
    alarm_head = NULL;
    alarm_tail = NULL;
}

void OSCreateAlarm(OSAlarm* alarm)
{
    alarm->handler = NULL;
    alarm->tag = 0;
}

void OSSetAlarm(OSAlarm* alarm, OSTime tick, OSAlarmHandler handler)
{
    alarm->period = 0;
    insert_alarm(alarm, OSGetTime() + tick, handler);
}

void OSSetAbsAlarm(OSAlarm* alarm, long long time, OSAlarmHandler handler)
{
    /* OSGetTime already reports system time on the host. */
    alarm->period = 0;
    insert_alarm(alarm, time, handler);
}

void OSSetPeriodicAlarm(OSAlarm* alarm, OSTime start, OSTime period,
                        OSAlarmHandler handler)
{
    alarm->period = period;
    alarm->start = start;
    insert_alarm(alarm, 0, handler);
}

void OSCancelAlarm(OSAlarm* alarm)
{
    if (alarm->handler == NULL) {
        return;
    }
    unlink_alarm(alarm);
    alarm->handler = NULL;
}

BOOL OSCheckAlarmQueue(void)
{
    OSAlarm* alarm;

    for (alarm = alarm_head; alarm != NULL; alarm = alarm->next) {
        if (alarm->next != NULL && alarm->next->prev != alarm) {
            return FALSE;
        }
        if (alarm->handler == NULL) {
            return FALSE;
        }
    }
    return TRUE;
}

void melee_host_os_fire_alarms(void)
{
    const OSTime now = OSGetTime();

    while (alarm_head != NULL && alarm_head->fire <= now) {
        OSAlarm* const alarm = alarm_head;
        const OSAlarmHandler handler = alarm->handler;

        unlink_alarm(alarm);
        alarm->handler = NULL;
        if (alarm->period > 0) {
            insert_alarm(alarm, 0, handler);
        }
        /* No interrupted context exists on the host. */
        handler(alarm, NULL);
    }
}
