#include "hsd_include.hpp"
#include "test.hpp"

#include <melee_host/video.h>

MELEE_HOST_TEST_HSD_BEGIN
#include <dolphin/gx/GXFrameBuffer.h>
#include <dolphin/gx/GXStruct.h>
#include <dolphin/vi.h>
#include <dolphin/vi/vitypes.h>
MELEE_HOST_TEST_HSD_END

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace {

std::vector<std::string>& retrace_log()
{
    static std::vector<std::string> log;
    return log;
}

u32 last_pre_count = 0;
u32 last_post_count = 0;

void on_pre_retrace(u32 count)
{
    last_pre_count = count;
    retrace_log().push_back("pre");
}

void on_post_retrace(u32 count)
{
    last_post_count = count;
    retrace_log().push_back("post");
}

} // namespace

TEST_CASE("video init reports NTSC interlaced defaults")
{
    melee_host_video_reset();
    VIInit();

    MeleeHostVideoState video{};
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(video.initialized);
    REQUIRE(video.tv_mode == VI_TVMODE_NTSC_INT);
    REQUIRE(video.tv_format == VI_NTSC);
    REQUIRE(video.interlaced);
    REQUIRE(video.retrace_count == 0);
    REQUIRE(VIGetRetraceCount() == 0);
    REQUIRE(VIGetTvFormat() == VI_NTSC);
    REQUIRE(melee_host_video_field_nanoseconds() ==
            MELEE_HOST_VIDEO_NTSC_FIELD_NANOSECONDS);
}

TEST_CASE("configuring from a render mode adopts its geometry")
{
    melee_host_video_reset();
    VIInit();
    VIConfigure(&GXNtsc480IntDf);

    MeleeHostVideoState video{};
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(video.framebuffer_width == 640);
    REQUIRE(video.embedded_framebuffer_height == 480);
    REQUIRE(video.external_framebuffer_height == 480);
    REQUIRE(video.view_left == 40);
    REQUIRE(video.view_width == 640);
    REQUIRE(video.external_framebuffer_mode == VI_XFBMODE_DF);
    REQUIRE(video.interlaced);
}

TEST_CASE("a staged framebuffer only reaches the display at a retrace")
{
    melee_host_video_reset();
    VIInit();

    std::array<std::uint8_t, 16> first{};
    std::array<std::uint8_t, 16> second{};

    VISetNextFrameBuffer(first.data());
    VIFlush();

    MeleeHostVideoState video{};
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(video.staged_framebuffer == first.data());
    // Nothing is scanned out until the retrace latches the shadow registers.
    REQUIRE(video.active_framebuffer == nullptr);
    REQUIRE(video.has_staged_changes);
    REQUIRE(video.flush_count == 1);

    VIWaitForRetrace();
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(video.active_framebuffer == first.data());
    REQUIRE(!video.has_staged_changes);
    REQUIRE(video.retrace_count == 1);

    // A second buffer replaces the first only after its own retrace.
    VISetNextFrameBuffer(second.data());
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(video.active_framebuffer == first.data());
    VIWaitForRetrace();
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(video.active_framebuffer == second.data());
    REQUIRE(video.retrace_count == 2);
}

TEST_CASE("retrace callbacks run in the original pre-then-post order")
{
    melee_host_video_reset();
    VIInit();
    retrace_log().clear();
    last_pre_count = 0;
    last_post_count = 0;

    REQUIRE(VISetPreRetraceCallback(on_pre_retrace) == nullptr);
    REQUIRE(VISetPostRetraceCallback(on_post_retrace) == nullptr);

    VIWaitForRetrace();
    REQUIRE(retrace_log().size() == 2);
    REQUIRE(retrace_log()[0] == "pre");
    REQUIRE(retrace_log()[1] == "post");
    // Both callbacks see the count of the retrace that just happened.
    REQUIRE(last_pre_count == 1);
    REQUIRE(last_post_count == 1);

    // The host frame loop can drive retraces instead of the game blocking.
    REQUIRE(melee_host_video_advance_retrace() == MELEE_HOST_OK);
    REQUIRE(last_post_count == 2);
    REQUIRE(VIGetRetraceCount() == 2);

    MeleeHostVideoState video{};
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(video.pre_retrace_callback_count == 2);
    REQUIRE(video.post_retrace_callback_count == 2);

    // Removing a callback returns the one it replaces and stops the calls.
    REQUIRE(VISetPreRetraceCallback(nullptr) == on_pre_retrace);
    REQUIRE(VISetPostRetraceCallback(nullptr) == on_post_retrace);
    retrace_log().clear();
    VIWaitForRetrace();
    REQUIRE(retrace_log().empty());
    REQUIRE(VIGetRetraceCount() == 3);
}

TEST_CASE("field parity alternates while interlaced and holds otherwise")
{
    melee_host_video_reset();
    VIInit();

    const u32 first = VIGetNextField();
    VIWaitForRetrace();
    const u32 second = VIGetNextField();
    VIWaitForRetrace();
    const u32 third = VIGetNextField();
    REQUIRE(first != second);
    REQUIRE(first == third);

    // A non-interlaced mode has a single field, so parity stops alternating.
    GXRenderModeObj progressive = GXNtsc480IntDf;
    progressive.viTVmode = VI_TVMODE_NTSC_DS;
    VIConfigure(&progressive);
    MeleeHostVideoState video{};
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(!video.interlaced);
    VIWaitForRetrace();
    REQUIRE(VIGetNextField() == 0);
    VIWaitForRetrace();
    REQUIRE(VIGetNextField() == 0);
}

TEST_CASE("blanking the output is staged like any other register change")
{
    melee_host_video_reset();
    VIInit();

    VISetBlack(TRUE);
    MeleeHostVideoState video{};
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(video.black);
    REQUIRE(video.has_staged_changes);
    VIWaitForRetrace();
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(!video.has_staged_changes);

    VISetBlack(FALSE);
    REQUIRE(melee_host_video_state(&video) == MELEE_HOST_OK);
    REQUIRE(!video.black);
}

TEST_CASE("the host refuses to advance a display that was never initialized")
{
    melee_host_video_reset();
    REQUIRE(melee_host_video_advance_retrace() == MELEE_HOST_NOT_READY);
    REQUIRE(melee_host_video_state(nullptr) == MELEE_HOST_INVALID_ARGUMENT);
}
