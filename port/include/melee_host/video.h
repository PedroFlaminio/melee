#ifndef MELEE_HOST_VIDEO_H
#define MELEE_HOST_VIDEO_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The video interface is console hardware, so the host owns it outright
 * rather than compiling the SDK's register code.  Time advances only when
 * something asks for a retrace: the game blocking in VIWaitForRetrace, or the
 * host frame loop calling melee_host_video_advance_retrace.  Nothing here
 * sleeps, so a replay advances identically regardless of wall clock. */

enum {
    /* One NTSC field, in nanoseconds: 1/59.94 s rounded to the nearest
     * nanosecond.  Two fields make an interlaced frame. */
    MELEE_HOST_VIDEO_NTSC_FIELD_NANOSECONDS = 16683333,
    MELEE_HOST_VIDEO_PAL_FIELD_NANOSECONDS = 20000000,
};

typedef struct MeleeHostVideoState {
    bool initialized;
    mh_u32 tv_mode;
    mh_u32 tv_format;
    bool interlaced;
    mh_u16 framebuffer_width;
    mh_u16 embedded_framebuffer_height;
    mh_u16 external_framebuffer_height;
    mh_u16 view_left;
    mh_u16 view_top;
    mh_u16 view_width;
    mh_u16 view_height;
    mh_u32 external_framebuffer_mode;
    bool black;
    /* The framebuffer staged by VISetNextFrameBuffer, and the one the display
     * is actually scanning.  A staged change only latches at the retrace that
     * follows a VIFlush, which is the hardware's shadow-register behaviour. */
    const void* staged_framebuffer;
    const void* active_framebuffer;
    bool has_staged_changes;
    mh_u32 retrace_count;
    /* Field of the retrace that will happen next: 0 or 1 while interlaced,
     * always 0 otherwise. */
    mh_u32 next_field;
    mh_u32 flush_count;
    mh_u32 pre_retrace_callback_count;
    mh_u32 post_retrace_callback_count;
} MeleeHostVideoState;

MeleeHostStatus melee_host_video_state(MeleeHostVideoState* out_state);

/* Advances the display by one field: latches whatever VIFlush staged and runs
 * the pre- and post-retrace callbacks in the original order.  This is the only
 * thing that moves VI time forward besides VIWaitForRetrace. */
MeleeHostStatus melee_host_video_advance_retrace(void);

/* Nanoseconds per field for the configured TV mode, for a host loop that wants
 * to pace itself against a real clock. */
mh_u64 melee_host_video_field_nanoseconds(void);

/* Discards the configuration and counters.  Intended for tests. */
void melee_host_video_reset(void);

#ifdef __cplusplus
}
#endif

#endif
