/* Host implementation of the GameCube video interface.
 *
 * VI drives a display the host does not have, so none of the SDK's register
 * code is compiled here.  What the game actually observes is a retrace
 * counter, a field parity, a pair of retrace callbacks and a shadow-register
 * latch, and that is what this models.
 *
 * Time is explicit: a retrace happens when the game blocks for one or when the
 * host frame loop asks for one.  No call here sleeps or reads a wall clock, so
 * the same sequence of calls always produces the same counters.
 */

#include <melee_host/video.h>

#include <melee_host/ax_mixer.h>
#include <melee_host/gx.h>

#include <dolphin/gx/GXStruct.h>
#include <dolphin/types.h>
#include <dolphin/vi.h>
#include <dolphin/vi/vitypes.h>

#include <mutex>

namespace {

std::mutex video_mutex;

struct VideoState {
    bool initialized = false;
    u32 tv_mode = VI_TVMODE_NTSC_INT;
    u16 framebuffer_width = 640;
    u16 embedded_framebuffer_height = 480;
    u16 external_framebuffer_height = 480;
    u16 view_left = 0;
    u16 view_top = 0;
    u16 view_width = 640;
    u16 view_height = 480;
    u32 external_framebuffer_mode = VI_XFBMODE_DF;
    bool black = false;
    const void* staged_framebuffer = nullptr;
    const void* active_framebuffer = nullptr;
    bool has_staged_changes = false;
    u32 retrace_count = 0;
    u32 next_field = 0;
    u32 flush_count = 0;
    u32 pre_retrace_callback_count = 0;
    u32 post_retrace_callback_count = 0;
};

VideoState state;
VIRetraceCallback pre_retrace_callback = nullptr;
VIRetraceCallback post_retrace_callback = nullptr;

bool interlaced_locked()
{
    /* The low two bits of a TV mode carry the scan type; interlace is zero. */
    return (state.tv_mode & 3U) == VI_INTERLACE;
}

/* Runs one retrace and reports the callbacks to invoke.  The callbacks
 * themselves run outside the lock: they are game code and may call back into
 * VI, which would otherwise deadlock. */
struct RetraceDelivery {
    VIRetraceCallback pre = nullptr;
    VIRetraceCallback post = nullptr;
    u32 count = 0;
};

RetraceDelivery retrace_locked()
{
    RetraceDelivery delivery;

    state.retrace_count += 1;
    delivery.count = state.retrace_count;

    if (pre_retrace_callback != nullptr) {
        state.pre_retrace_callback_count += 1;
        delivery.pre = pre_retrace_callback;
    }

    /* The hardware latches its shadow registers at the retrace that follows a
     * VIFlush, so a framebuffer set without a flush is not displayed yet. */
    if (state.has_staged_changes) {
        state.active_framebuffer = state.staged_framebuffer;
        state.has_staged_changes = false;
    }

    if (interlaced_locked()) {
        state.next_field ^= 1U;
    } else {
        state.next_field = 0;
    }

    if (post_retrace_callback != nullptr) {
        state.post_retrace_callback_count += 1;
        delivery.post = post_retrace_callback;
    }
    return delivery;
}

mh_u64 field_nanoseconds_locked()
{
    switch (state.tv_mode >> 2U) {
    case VI_PAL:
    case VI_DEBUG_PAL:
        return MELEE_HOST_VIDEO_PAL_FIELD_NANOSECONDS;
    default:
        return MELEE_HOST_VIDEO_NTSC_FIELD_NANOSECONDS;
    }
}

void deliver(const RetraceDelivery& delivery)
{
    if (delivery.pre != nullptr) {
        delivery.pre(delivery.count);
    }
    if (delivery.post != nullptr) {
        delivery.post(delivery.count);
    }
}

} // namespace

extern "C" {

void VIInit(void)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    state = VideoState{};
    state.initialized = true;
}

void VIConfigure(GXRenderModeObj* rm)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    if (rm == nullptr) {
        return;
    }
    state.tv_mode = static_cast<u32>(rm->viTVmode);
    state.framebuffer_width = rm->fbWidth;
    state.embedded_framebuffer_height = rm->efbHeight;
    state.external_framebuffer_height = rm->xfbHeight;
    state.view_left = rm->viXOrigin;
    state.view_top = rm->viYOrigin;
    state.view_width = rm->viWidth;
    state.view_height = rm->viHeight;
    state.external_framebuffer_mode = static_cast<u32>(rm->xFBmode);
    state.has_staged_changes = true;
    if (!interlaced_locked()) {
        state.next_field = 0;
    }
}

void VISetNextFrameBuffer(void* fb)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    state.staged_framebuffer = fb;
    state.has_staged_changes = true;
}

void VISetBlack(BOOL black)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    state.black = black != 0;
    state.has_staged_changes = true;
}

/* VIFlush only marks the staged registers ready; the latch itself happens at
 * the next retrace. */
void VIFlush(void)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    state.flush_count += 1;
}

void VIWaitForRetrace(void)
{
    /* The console takes interrupts while it waits for a retrace, and HSD's XFB
     * cycle needs the draw-done one: it is what turns the frame GX finished
     * into one a retrace can display, and HSD waits on retraces until one is.
     * The host has no graphics processor to raise it, so a fence still pending
     * is delivered here. */
    static_cast<void>(melee_host_gx_drain_draw_done());
    RetraceDelivery delivery;
    mh_u64 field = 0;
    {
        const std::lock_guard<std::mutex> guard(video_mutex);
        delivery = retrace_locked();
        field = field_nanoseconds_locked();
    }
    deliver(delivery);
    /* The DSP's frames keep coming while the game waits: one field of them
     * per retrace. */
    melee_host_ax_advance_time(field);
}

u32 VIGetRetraceCount(void)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    return state.retrace_count;
}

u32 VIGetNextField(void)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    return state.next_field;
}

/* The TV format lives in the bits above the scan type. */
u32 VIGetTvFormat(void)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    return state.tv_mode >> 2U;
}

/* No component cable exists on the host, so the digital output reports as
 * absent rather than guessing a progressive mode the host cannot honour. */
u32 VIGetDTVStatus(void)
{
    return 0;
}

VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback cb)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    VIRetraceCallback previous = pre_retrace_callback;
    pre_retrace_callback = cb;
    return previous;
}

VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback cb)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    VIRetraceCallback previous = post_retrace_callback;
    post_retrace_callback = cb;
    return previous;
}

MeleeHostStatus melee_host_video_state(MeleeHostVideoState* out_state)
{
    if (out_state == nullptr) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    const std::lock_guard<std::mutex> guard(video_mutex);
    out_state->initialized = state.initialized;
    out_state->tv_mode = state.tv_mode;
    out_state->tv_format = state.tv_mode >> 2U;
    out_state->interlaced = interlaced_locked();
    out_state->framebuffer_width = state.framebuffer_width;
    out_state->embedded_framebuffer_height = state.embedded_framebuffer_height;
    out_state->external_framebuffer_height =
        state.external_framebuffer_height;
    out_state->view_left = state.view_left;
    out_state->view_top = state.view_top;
    out_state->view_width = state.view_width;
    out_state->view_height = state.view_height;
    out_state->external_framebuffer_mode = state.external_framebuffer_mode;
    out_state->black = state.black;
    out_state->staged_framebuffer = state.staged_framebuffer;
    out_state->active_framebuffer = state.active_framebuffer;
    out_state->has_staged_changes = state.has_staged_changes;
    out_state->retrace_count = state.retrace_count;
    out_state->next_field = state.next_field;
    out_state->flush_count = state.flush_count;
    out_state->pre_retrace_callback_count = state.pre_retrace_callback_count;
    out_state->post_retrace_callback_count = state.post_retrace_callback_count;
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_video_advance_retrace(void)
{
    RetraceDelivery delivery;
    mh_u64 field = 0;
    {
        const std::lock_guard<std::mutex> guard(video_mutex);
        if (!state.initialized) {
            return MELEE_HOST_NOT_READY;
        }
        delivery = retrace_locked();
        field = field_nanoseconds_locked();
    }
    deliver(delivery);
    melee_host_ax_advance_time(field);
    return MELEE_HOST_OK;
}

mh_u64 melee_host_video_field_nanoseconds(void)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    return field_nanoseconds_locked();
}

void melee_host_video_reset(void)
{
    const std::lock_guard<std::mutex> guard(video_mutex);
    state = VideoState{};
    pre_retrace_callback = nullptr;
    post_retrace_callback = nullptr;
}

} // extern "C"
