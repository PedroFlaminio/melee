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

/* ARAM is a stack in the SDK: ARFree releases the most recent block and
 * reports its length, so the lengths are kept in allocation order. */
#define MELEE_HOST_ARAM_BLOCKS 64

static u32 melee_host_aram_next;
static u32 melee_host_aram_blocks[MELEE_HOST_ARAM_BLOCKS];
static u32 melee_host_aram_block_count;
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
    if (melee_host_aram_block_count == MELEE_HOST_ARAM_BLOCKS) {
        OSPanic(__FILE__, __LINE__, "host ARAM has no free blocks");
    }
    melee_host_aram_blocks[melee_host_aram_block_count++] = aligned_length;
    melee_host_aram_next += aligned_length;
    return result;
}

u32 ARFree(u32* length)
{
    u32 block;

    if (melee_host_aram_block_count == 0) {
        OSPanic(__FILE__, __LINE__, "ARFree with no ARAM block allocated");
    }
    block = melee_host_aram_blocks[--melee_host_aram_block_count];
    if (length != NULL) {
        *length = block;
    }
    melee_host_aram_next -= block;
    return melee_host_aram_next;
}

u32 ARGetSize(void)
{
    return MELEE_HOST_ARAM_SIZE;
}

/* The SDK's ARInit sets up the block stack and reports where user ARAM
 * begins.  The host keeps its own stack in ARAlloc and ARFree, so this only
 * reports the current top. */
u32 ARInit(u32* stack_index_addr, u32 num_entries)
{
    (void) stack_index_addr;
    (void) num_entries;
    return melee_host_aram_next;
}

/* ARAM transfers complete inside ARQPostRequest, so the queue needs no
 * setup. */
void ARQInit(void) {}

/* No audio interface to start: nothing drains the DSP's output yet. */
void AIInit(u8* stack)
{
    (void) stack;
}

void AXInit(void)
{
    melee_host_ax_callback = NULL;
}

/* There is no mixer on the host yet, so no voice can be handed out.  The
 * synth treats a NULL voice as every voice being taken and gives up on the
 * sound, which is what happens on the console when voices run out.  The
 * voice setters below are only ever called with a voice this returned. */
AXVPB* AXAcquireVoice(u32 priority, void (*callback)(void*), u32 userContext)
{
    (void) priority;
    (void) callback;
    (void) userContext;
    return NULL;
}

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

void AXSetVoiceMix(AXVPB* voice, AXPBMIX* mix)
{
    (void) voice;
    (void) mix;
}

void AXSetVoiceItdOn(AXVPB* voice)
{
    (void) voice;
}

void AXSetVoiceItdTarget(AXVPB* voice, u16 lShift, u16 rShift)
{
    (void) voice;
    (void) lShift;
    (void) rShift;
}

void AXSetVoiceSrc(AXVPB* voice, AXPBSRC* src)
{
    (void) voice;
    (void) src;
}

void AXSetVoiceAddr(AXVPB* voice, AXPBADDR* addr)
{
    (void) voice;
    (void) addr;
}

void AXSetVoiceAdpcm(AXVPB* voice, AXPBADPCM* adpcm)
{
    (void) voice;
    (void) adpcm;
}

void AXSetVoiceState(AXVPB* voice, u16 state)
{
    (void) voice;
    (void) state;
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

void AXSetVoicePriority(AXVPB* voice, u32 priority)
{
    (void) voice;
    (void) priority;
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

/* The console keeps why it last reset in low memory; zero is a cold power-on,
 * which is the only way the host starts.  gmMainLib_8015FCC0 reads it to
 * decide whether the intro is skipped. */
unsigned long OSGetResetCode(void)
{
    return 0;
}

/* gm_801A4014 resets the console only while the game is resetting, which
 * starts at the reset button; the host has neither the button nor a reset. */
void OSResetSystem(int reset, u32 resetCode, BOOL forceMenu)
{
    (void) reset;
    (void) resetCode;
    (void) forceMenu;
    OSPanic(__FILE__, __LINE__, "OSResetSystem is not ported to the host");
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
