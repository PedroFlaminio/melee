#include <melee_host/memory.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#endif
#include <dolphin/ai.h>
#include <dolphin/ar.h>
#include <dolphin/ax.h>
#include <dolphin/os/OSCache.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * The host does not emulate ARAM or the DSP.  These offsets are deliberately
 * not process pointers: code which stores an ARAM address must not be able to
 * accidentally dereference it on a 64-bit host.  Allocations are nevertheless
 * deterministic and keep the console's 32-byte DMA alignment contract.
 */
#define MELEE_HOST_ARAM_SIZE (16U * 1024U * 1024U)

static u32 melee_host_aram_next;
static u32 melee_host_dsp_sample_rate;
static u8 melee_host_stream_volume_left;
static u8 melee_host_stream_volume_right;
static void (*melee_host_ax_callback)(void);

u32 ARAlloc(u32 length)
{
    const u32 aligned_length = (length + (ARQ_DMA_ALIGNMENT - 1U)) &
                               ~(ARQ_DMA_ALIGNMENT - 1U);
    const u32 result = melee_host_aram_next;

    if (aligned_length > MELEE_HOST_ARAM_SIZE - result) {
        OSPanic(__FILE__, __LINE__, "host ARAM exhausted");
    }
    melee_host_aram_next += aligned_length;
    return result;
}

void AXInit(void)
{
    melee_host_ax_callback = NULL;
}

void AXRegisterCallback(void (*callback)(void))
{
    melee_host_ax_callback = callback;
}

void AXFreeVoice(AXVPB* voice)
{
    (void) voice;
}

void AXSetVoiceVe(AXVPB* voice, AXPBVE* envelope)
{
    (void) voice;
    (void) envelope;
}

void AXSetVoiceVeDelta(AXVPB* voice, s16 delta)
{
    (void) voice;
    (void) delta;
}

void AXSetVoiceLoop(AXVPB* voice, u16 loop)
{
    (void) voice;
    (void) loop;
}

void AXSetVoiceLoopAddr(AXVPB* voice, u32 address)
{
    (void) voice;
    (void) address;
}

void AXSetVoiceEndAddr(AXVPB* voice, u32 address)
{
    (void) voice;
    (void) address;
}

void AXSetVoiceSrcRatio(AXVPB* voice, float ratio)
{
    (void) voice;
    (void) ratio;
}

void AXSetVoiceAdpcmLoop(AXVPB* voice, AXPBADPCMLOOP* loop)
{
    (void) voice;
    (void) loop;
}

void AISetDSPSampleRate(u32 rate)
{
    melee_host_dsp_sample_rate = rate;
}

u32 AIGetDSPSampleRate(void)
{
    return melee_host_dsp_sample_rate;
}

void AISetStreamVolLeft(u8 volume)
{
    melee_host_stream_volume_left = volume;
}

u8 AIGetStreamVolLeft(void)
{
    return melee_host_stream_volume_left;
}

void AISetStreamVolRight(u8 volume)
{
    melee_host_stream_volume_right = volume;
}

u8 AIGetStreamVolRight(void)
{
    return melee_host_stream_volume_right;
}

u32 OSGetSoundMode(void)
{
    return 0;
}

void DCInvalidateRange(void* address, u32 length)
{
    (void) address;
    (void) length;
}

void DCStoreRange(void* address, u32 length)
{
    (void) address;
    (void) length;
}

void DCStoreRangeNoSync(void* address, u32 length)
{
    (void) address;
    (void) length;
}

BOOL OSGetResetSwitchState(void)
{
    return FALSE;
}

void ARQPostRequest(ARQRequest* request, u32 owner, u32 type, u32 priority,
                    ARQAddress source, ARQAddress dest, u32 length,
                    ARQCallback callback)
{
    request->next = NULL;
    request->owner = owner;
    request->type = type;
    request->priority = priority;
    request->source = source;
    request->dest = dest;
    request->length = length;
    request->callback = callback;
    if (callback != NULL) {
        callback(request);
    }
}

void OSReport(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
}

void OSPanic(char* file, int line, char* message, ...)
{
    va_list arguments;

    fprintf(stderr, "OS panic at %s:%d: ", file, line);
    va_start(arguments, message);
    vfprintf(stderr, message, arguments);
    va_end(arguments);
    fputc('\n', stderr);
    abort();
}

void __assert(const char* file, u32 line, const char* condition)
{
    fprintf(stderr, "HSD assertion failed at %s:%u: %s\n", file, line,
            condition);
    abort();
}

void HSD_Panic(char* file, u32 line, char* message)
{
    fprintf(stderr, "HSD panic at %s:%u: %s\n", file, line, message);
    abort();
}
