#include <melee_host/ax_mixer.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#endif
#include <dolphin/ax.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <stdbool.h>
#include <string.h>

/* The SDK keeps voices on stacks by priority and takes the bottom of the
 * lowest stack when it has to steal one.  The host keeps a flag and an
 * acquisition order per voice, which picks the same voice: the oldest of the
 * lowest priority. */
static AXVPB ax_voices[AX_MAX_VOICES];
static bool ax_voice_used[AX_MAX_VOICES];
static mh_u32 ax_voice_order[AX_MAX_VOICES];
static mh_u32 ax_acquisitions;
static bool ax_ready;
static bool ax_voices_on;
static void (*ax_frame_callback)(void);

enum {
    /* AX address formats. */
    AX_FORMAT_ADPCM = 0,
    AX_FORMAT_PCM16 = 10,
    AX_FORMAT_PCM8 = 25,
};

static mh_s32 ax_clamp(mh_s32 value, mh_s32 low, mh_s32 high)
{
    return value < low ? low : value > high ? high : value;
}

/* __AXSetPBDefault. */
static void ax_set_pb_default(AXVPB* p)
{
    p->pb.state = 0;
    p->pb.itd.flag = 0;
    p->sync = 0xA4;
    p->updateMS = 0;
    p->updateCounter = 0;
    p->updateWrite = p->updateData;
    memset(p->pb.update.updNum, 0, sizeof(p->pb.update.updNum));
}

static void ax_reset_voices(void)
{
    mh_u32 i;

    memset(ax_voices, 0, sizeof(ax_voices));
    memset(ax_voice_used, 0, sizeof(ax_voice_used));
    memset(ax_voice_order, 0, sizeof(ax_voice_order));
    ax_acquisitions = 0;
    for (i = 0; i < AX_MAX_VOICES; i++) {
        /* The synth indexes its sound nodes by a voice's index. */
        ax_voices[i].index = i;
        ax_set_pb_default(&ax_voices[i]);
    }
    ax_ready = true;
}

void melee_host_ax_set_voices_enabled(bool enabled)
{
    ax_voices_on = enabled;
}

bool melee_host_ax_voices_enabled(void)
{
    return ax_voices_on;
}

mh_u32 melee_host_ax_acquired_voices(void)
{
    mh_u32 count = 0;
    mh_u32 i;

    for (i = 0; i < AX_MAX_VOICES; i++) {
        count += ax_voice_used[i] ? 1U : 0U;
    }
    return count;
}

void AXInit(void)
{
    ax_reset_voices();
    ax_frame_callback = NULL;
}

AXVPB* AXAcquireVoice(u32 priority, void (*callback)(void*), u32 userContext)
{
    AXVPB* p;
    mh_u32 slot = AX_MAX_VOICES;
    mh_u32 i;

    if (!ax_voices_on) {
        return NULL;
    }
    if (!ax_ready) {
        ax_reset_voices();
    }
    for (i = 0; i < AX_MAX_VOICES; i++) {
        if (!ax_voice_used[i]) {
            slot = i;
            break;
        }
    }
    if (slot == AX_MAX_VOICES) {
        for (i = 0; i < AX_MAX_VOICES; i++) {
            const mh_u32 held = (mh_u32) ax_voices[i].priority;
            if (held >= priority) {
                continue;
            }
            if (slot == AX_MAX_VOICES ||
                held < (mh_u32) ax_voices[slot].priority ||
                (held == (mh_u32) ax_voices[slot].priority &&
                 ax_voice_order[i] < ax_voice_order[slot]))
            {
                slot = i;
            }
        }
        if (slot == AX_MAX_VOICES) {
            return NULL;
        }
        p = &ax_voices[slot];
        if (p->pb.state == 1) {
            p->depop = 1;
        }
        if (p->callback != NULL) {
            p->callback(p);
        }
    }
    p = &ax_voices[slot];
    ax_voice_used[slot] = true;
    ax_voice_order[slot] = ++ax_acquisitions;
    p->priority = (int) priority;
    p->callback = callback;
    p->userContext = userContext;
    ax_set_pb_default(p);
    return p;
}

void AXFreeVoice(AXVPB* voice)
{
    mh_u32 i;

    for (i = 0; i < AX_MAX_VOICES; i++) {
        if (&ax_voices[i] == voice) {
            if (voice->pb.state == 1) {
                voice->depop = 1;
            }
            ax_set_pb_default(voice);
            ax_voice_used[i] = false;
            return;
        }
    }
}

void AXSetVoicePriority(AXVPB* voice, u32 priority)
{
    voice->priority = (int) priority;
}

/* The aux buses run the reverb and chorus, whose DSP cores are PowerPC
 * assembly the host does not have; nothing is sent to them. */
void AXRegisterAuxACallback(void (*callback)(void*, void*), void* context)
{
    (void) callback;
    (void) context;
}

void AXRegisterAuxBCallback(void (*callback)(void*, void*), void* context)
{
    (void) callback;
    (void) context;
}

void AXRegisterCallback(void (*callback)(void))
{
    ax_frame_callback = callback;
}

void AXSetVoiceMix(AXVPB* voice, AXPBMIX* mix)
{
    mh_u32 control = 0;

    voice->pb.mix = *mix;
    if (mix->vAuxAL != 0 || mix->vAuxAR != 0) {
        control |= 1U;
    }
    if (mix->vAuxBL != 0 || mix->vAuxBR != 0) {
        control |= 2U;
    }
    if (mix->vS != 0 || mix->vAuxAS != 0 || mix->vAuxBS != 0) {
        control |= 4U;
    }
    if (mix->vDeltaL != 0 || mix->vDeltaR != 0 || mix->vDeltaS != 0 ||
        mix->vDeltaAuxAL != 0 || mix->vDeltaAuxAR != 0 ||
        mix->vDeltaAuxAS != 0 || mix->vDeltaAuxBL != 0 ||
        mix->vDeltaAuxBR != 0 || mix->vDeltaAuxBS != 0)
    {
        control |= 8U;
    }
    voice->pb.mixerCtrl = (u16) control;
}

void AXSetVoiceItdOn(AXVPB* voice)
{
    voice->pb.itd.flag = 1;
    voice->pb.itd.shiftL = 0;
    voice->pb.itd.shiftR = 0;
    voice->pb.itd.targetShiftL = 0;
    voice->pb.itd.targetShiftR = 0;
}

void AXSetVoiceItdTarget(AXVPB* voice, u16 lShift, u16 rShift)
{
    voice->pb.itd.targetShiftL = lShift;
    voice->pb.itd.targetShiftR = rShift;
}

void AXSetVoiceSrc(AXVPB* voice, AXPBSRC* src)
{
    voice->pb.src = *src;
}

/* PCM voices carry no ADPCM state: the SDK clears it and sets the gain the
 * DSP scales the samples by. */
void AXSetVoiceAddr(AXVPB* voice, AXPBADDR* addr)
{
    voice->pb.addr = *addr;
    if (addr->format == AX_FORMAT_PCM16 || addr->format == AX_FORMAT_PCM8) {
        memset(&voice->pb.adpcm, 0, sizeof(voice->pb.adpcm));
        voice->pb.adpcm.gain =
            addr->format == AX_FORMAT_PCM16 ? 0x0800 : 0x0100;
    }
}

void AXSetVoiceCurrentAddr(AXVPB* voice, u32 addr)
{
    voice->pb.addr.currentAddressHi = (u16) (addr >> 16);
    voice->pb.addr.currentAddressLo = (u16) addr;
}

void AXSetVoiceAdpcm(AXVPB* voice, AXPBADPCM* adpcm)
{
    voice->pb.adpcm = *adpcm;
}

void AXSetVoiceState(AXVPB* voice, u16 state)
{
    voice->pb.state = state;
    if (state == 0) {
        voice->depop = 1;
    }
}

void AXSetVoiceVe(AXVPB* voice, AXPBVE* envelope)
{
    voice->pb.ve = *envelope;
}

void AXSetVoiceVeDelta(AXVPB* voice, s16 delta)
{
    voice->pb.ve.currentDelta = delta;
}

void AXSetVoiceLoop(AXVPB* voice, u16 loop)
{
    voice->pb.addr.loopFlag = loop;
}

void AXSetVoiceLoopAddr(AXVPB* voice, u32 address)
{
    voice->pb.addr.loopAddressHi = (u16) (address >> 16);
    voice->pb.addr.loopAddressLo = (u16) address;
}

void AXSetVoiceEndAddr(AXVPB* voice, u32 address)
{
    voice->pb.addr.endAddressHi = (u16) (address >> 16);
    voice->pb.addr.endAddressLo = (u16) address;
}

/* The SDK caps the ratio at 4. */
void AXSetVoiceSrcRatio(AXVPB* voice, float ratio)
{
    mh_u32 fixed = (mh_u32) (65536.0f * ratio);

    if (fixed > 0x40000U) {
        fixed = 0x40000U;
    }
    voice->pb.src.ratioHi = (u16) (fixed >> 16);
    voice->pb.src.ratioLo = (u16) fixed;
}

void AXSetVoiceAdpcmLoop(AXVPB* voice, AXPBADPCMLOOP* loop)
{
    voice->pb.adpcmLoop = *loop;
}

static mh_u32 ax_address(u16 high, u16 low)
{
    return ((mh_u32) high << 16) | low;
}

/* The next source sample of a voice, or false once it has passed its end
 * address without looping.  A loop goes back to the loop address with the
 * ADPCM context the loop block gives. */
static bool ax_decode(AXPB* pb, const mh_u8* aram, mh_u32 aram_size,
                      mh_s32* out)
{
    mh_u32 current =
        ax_address(pb->addr.currentAddressHi, pb->addr.currentAddressLo);
    const mh_u32 end =
        ax_address(pb->addr.endAddressHi, pb->addr.endAddressLo);
    mh_s32 value;

    if (current > end) {
        if (pb->addr.loopFlag == 0) {
            return false;
        }
        current =
            ax_address(pb->addr.loopAddressHi, pb->addr.loopAddressLo);
        if (pb->addr.format == AX_FORMAT_ADPCM) {
            pb->adpcm.pred_scale = pb->adpcmLoop.loop_pred_scale;
            pb->adpcm.yn1 = pb->adpcmLoop.loop_yn1;
            pb->adpcm.yn2 = pb->adpcmLoop.loop_yn2;
        }
    }
    switch (pb->addr.format) {
    case AX_FORMAT_ADPCM: {
        mh_s32 nibble;
        mh_u32 pair;
        mh_s64 prediction;
        mh_u8 byte;

        /* Every sixteenth nibble starts a frame with its predictor and scale
         * byte. */
        if ((current & 0xFU) == 0) {
            if ((current >> 1) >= aram_size) {
                return false;
            }
            pb->adpcm.pred_scale = aram[current >> 1];
            current += 2;
        }
        if ((current >> 1) >= aram_size) {
            return false;
        }
        byte = aram[current >> 1];
        nibble = (current & 1U) != 0 ? (byte & 0xF) : (byte >> 4);
        if (nibble >= 8) {
            nibble -= 16;
        }
        pair = (pb->adpcm.pred_scale >> 4) & 7U;
        prediction =
            (mh_s64) (s16) pb->adpcm.a[pair][0] * (s16) pb->adpcm.yn1 +
            (mh_s64) (s16) pb->adpcm.a[pair][1] * (s16) pb->adpcm.yn2;
        value = nibble * (1 << (pb->adpcm.pred_scale & 0xF)) +
                (mh_s32) ((0x400 + prediction) >> 11);
        value = ax_clamp(value, -32768, 32767);
        pb->adpcm.yn2 = pb->adpcm.yn1;
        pb->adpcm.yn1 = (u16) (s16) value;
        current += 1;
        break;
    }
    case AX_FORMAT_PCM16:
        if ((mh_u64) current * 2U + 1U >= aram_size) {
            return false;
        }
        value = (s16) (u16) ((aram[current * 2U] << 8) |
                             aram[current * 2U + 1U]);
        current += 1;
        break;
    case AX_FORMAT_PCM8:
        if (current >= aram_size) {
            return false;
        }
        value = (s8) aram[current] * 256;
        current += 1;
        break;
    default:
        return false;
    }
    pb->addr.currentAddressHi = (u16) (current >> 16);
    pb->addr.currentAddressLo = (u16) current;
    *out = value;
    return true;
}

void melee_host_ax_render_voice(struct _AXPB* pb, const mh_u8* aram,
                                mh_u32 aram_size, mh_s32* left,
                                mh_s32* right, mh_u32 count)
{
    /* The DSP keeps its resampler's history in last_samples.  The host's
     * linear resampler keeps there whether the voice has started ([0]),
     * whether the sample after the play position is past the end ([1]), and
     * the samples on either side of the position ([2], [3]).  AXSetVoiceSrc
     * clears all four, which starts a new sound. */
    const mh_u32 ratio =
        ((mh_u32) pb->src.ratioHi << 16) | pb->src.ratioLo;
    mh_u32 fraction = pb->src.currentAddressFrac;
    bool ending = pb->src.last_samples[1] != 0;
    mh_s32 here = (s16) pb->src.last_samples[2];
    mh_s32 next = (s16) pb->src.last_samples[3];
    mh_s32 volume = pb->ve.currentVolume;
    const mh_s32 volume_delta = pb->ve.currentDelta;
    mh_s32 mix_left = pb->mix.vL;
    mh_s32 mix_right = pb->mix.vR;
    const bool ramp = (pb->mixerCtrl & 8U) != 0;
    mh_u32 n;

    if (pb->state != 1) {
        return;
    }
    if (pb->src.last_samples[0] == 0) {
        if (!ax_decode(pb, aram, aram_size, &here)) {
            pb->state = 0;
            return;
        }
        if (!ax_decode(pb, aram, aram_size, &next)) {
            next = here;
            ending = true;
        }
    }
    for (n = 0; n < count && pb->state == 1; n++) {
        const mh_s32 sample =
            here + (mh_s32) (((mh_s64) (next - here) * fraction) >> 16);
        const mh_s32 played = (mh_s32) (((mh_s64) sample * volume) >> 15);

        if (left != NULL) {
            left[n] += (mh_s32) (((mh_s64) played * mix_left) >> 15);
        }
        if (right != NULL) {
            right[n] += (mh_s32) (((mh_s64) played * mix_right) >> 15);
        }
        volume = ax_clamp(volume + volume_delta, 0, 0xFFFF);
        if (ramp) {
            mix_left = ax_clamp(mix_left + (s16) pb->mix.vDeltaL, 0, 0xFFFF);
            mix_right =
                ax_clamp(mix_right + (s16) pb->mix.vDeltaR, 0, 0xFFFF);
        }
        fraction += ratio;
        while (fraction >= 0x10000U) {
            fraction -= 0x10000U;
            if (ending) {
                pb->state = 0;
                break;
            }
            here = next;
            if (!ax_decode(pb, aram, aram_size, &next)) {
                next = here;
                ending = true;
            }
        }
    }
    pb->src.currentAddressFrac = (u16) fraction;
    pb->src.last_samples[0] = 1;
    pb->src.last_samples[1] = ending ? 1 : 0;
    pb->src.last_samples[2] = (u16) (s16) here;
    pb->src.last_samples[3] = (u16) (s16) next;
    pb->ve.currentVolume = (u16) volume;
    pb->mix.vL = (u16) mix_left;
    pb->mix.vR = (u16) mix_right;
}

void melee_host_ax_run_frame(mh_s16* stereo)
{
    mh_s32 left[MELEE_HOST_AX_FRAME_SAMPLES];
    mh_s32 right[MELEE_HOST_AX_FRAME_SAMPLES];
    const mh_u8* const aram = melee_host_aram_bytes();
    const mh_u32 aram_size = melee_host_aram_size();
    mh_u32 i;

    memset(left, 0, sizeof(left));
    memset(right, 0, sizeof(right));
    for (i = 0; i < AX_MAX_VOICES; i++) {
        if (ax_voice_used[i] && ax_voices[i].pb.state == 1) {
            melee_host_ax_render_voice(&ax_voices[i].pb, aram, aram_size,
                                       left, right,
                                       MELEE_HOST_AX_FRAME_SAMPLES);
        }
    }
    if (stereo != NULL) {
        for (i = 0; i < MELEE_HOST_AX_FRAME_SAMPLES; i++) {
            stereo[i * 2] = (mh_s16) ax_clamp(left[i], -32768, 32767);
            stereo[i * 2 + 1] = (mh_s16) ax_clamp(right[i], -32768, 32767);
        }
    }
    if (ax_frame_callback != NULL) {
        ax_frame_callback();
    }
}
