/* Checks the host's AX mixer through the SDK's own AX types, which C++ cannot
 * include (dolphin/os.h is not C++ clean).  Each check returns 1, or 0 with
 * the failed condition in `message`. */

#include <melee_host/ax_mixer.h>

#include <dolphin/ax.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int melee_host_test_ax_decode(char* message, size_t size);
int melee_host_test_ax_prediction(char* message, size_t size);
int melee_host_test_ax_loop(char* message, size_t size);
int melee_host_test_ax_loop_ahead(char* message, size_t size);
int melee_host_test_ax_resample(char* message, size_t size);
int melee_host_test_ax_voices_off(char* message, size_t size);
int melee_host_test_ax_steal(char* message, size_t size);
int melee_host_test_ax_setters(char* message, size_t size);
int melee_host_test_ax_frame(char* message, size_t size);

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            snprintf(message, size, "line %d: %s", __LINE__, #condition);     \
            return 0;                                                         \
        }                                                                     \
    } while (0)

/* A DSP ADPCM voice at the output rate and full volume, so each output
 * sample is one decoded sample. */
static AXPB adpcm_voice(u32 current, u32 end)
{
    AXPB pb;

    memset(&pb, 0, sizeof(pb));
    pb.state = 1;
    pb.addr.format = 0;
    pb.addr.currentAddressHi = current >> 16;
    pb.addr.currentAddressLo = current;
    pb.addr.endAddressHi = end >> 16;
    pb.addr.endAddressLo = end;
    pb.src.ratioHi = 1;
    pb.ve.currentVolume = 0x8000;
    pb.mix.vL = 0x8000;
    pb.mix.vR = 0x8000;
    return pb;
}

int melee_host_test_ax_decode(char* message, size_t size)
{
    /* Two frames with zero coefficients, the first at scale 1 and the second
     * at scale 4: each sample is its nibble times the scale, and the frame
     * header at nibble 16 is read, not played. */
    static const u8 aram[16] = { 0x00, 0x01, 0x23, 0x45, 0x67, 0x89,
                                 0xAB, 0xCD, 0x02, 0x7F };
    static const s32 expected[20] = { 0,  1,  2,  3,  4,  5,  6,
                                      7,  -8, -7, -6, -5, -4, -3,
                                      28, -4, 0,  0,  0,  0 };
    s32 left[20] = { 0 };
    s32 right[20] = { 0 };
    AXPB pb = adpcm_voice(2, 19);

    melee_host_ax_render_voice(&pb, aram, sizeof(aram), left, right, 20);
    CHECK(memcmp(left, expected, sizeof(expected)) == 0);
    CHECK(memcmp(right, expected, sizeof(expected)) == 0);
    CHECK(pb.state == 0);
    CHECK(pb.adpcm.pred_scale == 0x02);
    return 1;
}

int melee_host_test_ax_prediction(char* message, size_t size)
{
    /* Pair 1 weighs the previous sample by 1.0 (0x800 in 5.11), so each
     * nibble adds to the running value. */
    static const u8 aram[8] = { 0x10, 0x11, 0x11, 0x11,
                                0xFF, 0xFF, 0x11, 0x11 };
    static const s32 expected[14] = { 1, 2, 3, 4, 5, 6, 5,
                                      4, 3, 2, 3, 4, 5, 6 };
    s32 left[14] = { 0 };
    AXPB pb = adpcm_voice(2, 15);

    /* A voice starts after its first frame header, whose byte the synth's
     * descriptor carries as the starting predictor and scale. */
    pb.adpcm.pred_scale = 0x10;
    pb.adpcm.a[1][0] = 0x0800;
    melee_host_ax_render_voice(&pb, aram, sizeof(aram), left, NULL, 14);
    CHECK(memcmp(left, expected, sizeof(expected)) == 0);
    CHECK((s16) pb.adpcm.yn1 == 6);
    CHECK((s16) pb.adpcm.yn2 == 5);
    return 1;
}

int melee_host_test_ax_loop(char* message, size_t size)
{
    static const u8 aram[8] = { 0x00, 0x12, 0x30 };
    static const s32 expected[10] = { 1, 2, 3, 0, 1, 2, 3, 0, 1, 2 };
    s32 left[10] = { 0 };
    AXPB pb = adpcm_voice(2, 5);

    pb.addr.loopFlag = 1;
    pb.addr.loopAddressLo = 2;
    melee_host_ax_render_voice(&pb, aram, sizeof(aram), left, NULL, 10);
    CHECK(memcmp(left, expected, sizeof(expected)) == 0);
    CHECK(pb.state == 1);
    return 1;
}

int melee_host_test_ax_loop_ahead(char* message, size_t size)
{
    /* A music stream loops into its next chunk, past its end address, and
     * the end address moves only at the next frame callback.  Until then the
     * voice plays on from the loop address instead of returning to it on
     * every sample. */
    static const u8 aram[16] = { 0x00, 0x12, 0x34, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x56, 0x71 };
    static const s32 expected[8] = { 1, 2, 3, 4, 5, 6, 7, 1 };
    s32 left[8] = { 0 };
    AXPB pb = adpcm_voice(2, 5);

    pb.addr.loopFlag = 1;
    pb.addr.loopAddressLo = 18;
    melee_host_ax_render_voice(&pb, aram, sizeof(aram), left, NULL, 8);
    CHECK(memcmp(left, expected, sizeof(expected)) == 0);
    CHECK(pb.state == 1);
    return 1;
}

int melee_host_test_ax_resample(char* message, size_t size)
{
    /* Nibbles 0, 2, 4 and 6 at half speed; the last sample holds for its
     * second half before the voice stops. */
    static const u8 aram[8] = { 0x00, 0x02, 0x46 };
    static const s32 expected[10] = { 0, 1, 2, 3, 4, 5, 6, 6, 0, 0 };
    s32 left[10] = { 0 };
    AXPB pb = adpcm_voice(2, 5);

    pb.src.ratioHi = 0;
    pb.src.ratioLo = 0x8000;
    melee_host_ax_render_voice(&pb, aram, sizeof(aram), left, NULL, 10);
    CHECK(memcmp(left, expected, sizeof(expected)) == 0);
    CHECK(pb.state == 0);
    return 1;
}

int melee_host_test_ax_voices_off(char* message, size_t size)
{
    melee_host_ax_set_voices_enabled(false);
    AXInit();
    CHECK(AXAcquireVoice(10, NULL, 0) == NULL);
    CHECK(melee_host_ax_acquired_voices() == 0);
    return 1;
}

static int dropped_calls;
static AXVPB* dropped_voice;

static void record_drop(void* voice)
{
    dropped_calls++;
    dropped_voice = voice;
}

int melee_host_test_ax_steal(char* message, size_t size)
{
    AXVPB* voices[AX_MAX_VOICES];
    AXVPB* taken;
    u32 i;

    melee_host_ax_set_voices_enabled(true);
    AXInit();
    for (i = 0; i < AX_MAX_VOICES; i++) {
        voices[i] = AXAcquireVoice(5, record_drop, i);
        CHECK(voices[i] != NULL);
        CHECK(voices[i]->index == i);
    }
    CHECK(melee_host_ax_acquired_voices() == AX_MAX_VOICES);
    /* A request no higher than every voice finds nothing to take. */
    CHECK(AXAcquireVoice(5, record_drop, 99) == NULL);

    dropped_calls = 0;
    voices[0]->pb.state = 1;
    taken = AXAcquireVoice(10, NULL, 100);
    CHECK(taken == voices[0]);
    CHECK(dropped_calls == 1);
    CHECK(dropped_voice == voices[0]);
    CHECK(taken->depop == 1);
    CHECK(taken->pb.state == 0);
    CHECK(taken->userContext == 100);

    AXFreeVoice(voices[1]);
    CHECK(melee_host_ax_acquired_voices() == AX_MAX_VOICES - 1);
    CHECK(AXAcquireVoice(3, NULL, 0) == voices[1]);
    for (i = 0; i < AX_MAX_VOICES; i++) {
        AXFreeVoice(voices[i]);
    }
    CHECK(melee_host_ax_acquired_voices() == 0);
    melee_host_ax_set_voices_enabled(false);
    return 1;
}

int melee_host_test_ax_setters(char* message, size_t size)
{
    AXVPB voice;
    AXPBMIX mix;
    AXPBADDR pcm;

    memset(&voice, 0, sizeof(voice));
    AXSetVoiceSrcRatio(&voice, 0.5F);
    CHECK(voice.pb.src.ratioHi == 0);
    CHECK(voice.pb.src.ratioLo == 0x8000);
    AXSetVoiceSrcRatio(&voice, 9.0F);
    CHECK(voice.pb.src.ratioHi == 4);
    CHECK(voice.pb.src.ratioLo == 0);

    memset(&mix, 0, sizeof(mix));
    mix.vL = 0x8000;
    AXSetVoiceMix(&voice, &mix);
    CHECK(voice.pb.mixerCtrl == 0);
    mix.vAuxBL = 1;
    mix.vDeltaR = 2;
    AXSetVoiceMix(&voice, &mix);
    CHECK(voice.pb.mixerCtrl == (2 | 8));

    memset(&pcm, 0, sizeof(pcm));
    pcm.format = 10;
    voice.pb.adpcm.pred_scale = 0x55;
    AXSetVoiceAddr(&voice, &pcm);
    CHECK(voice.pb.adpcm.gain == 0x0800);
    CHECK(voice.pb.adpcm.pred_scale == 0);
    return 1;
}

static int frame_callbacks;

static void count_frame(void)
{
    frame_callbacks++;
}

int melee_host_test_ax_frame(char* message, size_t size)
{
    s16 stereo[MELEE_HOST_AX_FRAME_SAMPLES * 2];
    u32 i;

    melee_host_ax_set_voices_enabled(false);
    AXInit();
    AXRegisterCallback(count_frame);
    frame_callbacks = 0;
    for (i = 0; i < MELEE_HOST_AX_FRAME_SAMPLES * 2; i++) {
        stereo[i] = 7;
    }
    melee_host_ax_run_frame(stereo);
    CHECK(frame_callbacks == 1);
    for (i = 0; i < MELEE_HOST_AX_FRAME_SAMPLES * 2; i++) {
        CHECK(stereo[i] == 0);
    }
    AXRegisterCallback(NULL);
    return 1;
}
