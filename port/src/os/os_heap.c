/* Host arena for the original OSAlloc heap.
 *
 * OSAlloc.c and OSArena.c are the SDK's own code, compiled unchanged apart
 * from carrying addresses at pointer width under MELEE_HOST.  What the console
 * provides and the host does not is the memory map: a fixed arena between
 * OSGetArenaLo and OSGetArenaHi.  This file supplies that arena from host
 * memory and runs the same initialisation sequence the boot path does.
 *
 * This is C rather than C++ because the SDK's os.h is not C++ clean, and
 * because the heap itself has no internal locking: on hardware the game guards
 * it with the interrupt facade, and wrapping only this file in a mutex would
 * suggest a thread safety the allocator does not have.
 */

#include <melee_host/memory.h>
#include <melee_host/os_heap.h>

#include <dolphin/os.h>
#include <dolphin/os/OSAlloc.h>

#include <stddef.h>

#define MELEE_HOST_ARENA_ALIGNMENT 32

static void* arena_backing;
static size_t arena_size;
static int heap_handle = -1;
static int configured_heaps;

/* Both memory-size queries describe console hardware the host is standing in
 * for, so they report the real GameCube figure rather than the host's RAM.
 * Game code uses them to decide how much it may consume. */
u32 OSGetPhysicalMemSize(void)
{
    return MELEE_HOST_OS_PHYSICAL_MEMORY_BYTES;
}

u32 OSGetConsoleSimulatedMemSize(void)
{
    return MELEE_HOST_OS_PHYSICAL_MEMORY_BYTES;
}

static void release_backing(void)
{
    melee_host_aligned_free(arena_backing, MELEE_HOST_ARENA_ALIGNMENT);
    arena_backing = NULL;
    arena_size = 0;
}

MeleeHostStatus melee_host_os_heap_init(size_t arena_bytes, mh_s32 max_heaps)
{
    char* low;
    char* high;
    void* heap_start;
    size_t descriptors;

    if (max_heaps <= 0) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (arena_backing != NULL) {
        return MELEE_HOST_OK;
    }
    if (arena_bytes == 0) {
        arena_bytes = MELEE_HOST_OS_PHYSICAL_MEMORY_BYTES;
    }
    /* OSInitAlloc carves the heap descriptor array off the front and
     * OSCreateHeap refuses a span under 64 bytes, so a tiny arena cannot
     * produce a usable heap. */
    descriptors = (size_t) max_heaps * 24U;
    if (arena_bytes < descriptors + 128U) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }

    arena_backing =
        melee_host_aligned_alloc(arena_bytes, MELEE_HOST_ARENA_ALIGNMENT);
    if (arena_backing == NULL) {
        return MELEE_HOST_INTERNAL_ERROR;
    }
    arena_size = arena_bytes;

    low = arena_backing;
    high = low + arena_bytes;
    OSSetArenaLo(low);
    OSSetArenaHi(high);

    heap_start = OSInitAlloc(OSGetArenaLo(), OSGetArenaHi(), max_heaps);
    if (heap_start == NULL) {
        release_backing();
        return MELEE_HOST_INTERNAL_ERROR;
    }
    OSSetArenaLo(heap_start);

    heap_handle = OSCreateHeap(OSGetArenaLo(), OSGetArenaHi());
    if (heap_handle < 0) {
        release_backing();
        return MELEE_HOST_INTERNAL_ERROR;
    }
    /* The heap now owns everything up to the top, so the arena is spent. */
    OSSetArenaLo(OSGetArenaHi());
    OSSetCurrentHeap(heap_handle);
    configured_heaps = max_heaps;
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_os_arena_init(size_t arena_bytes)
{
    if (arena_backing != NULL) {
        return MELEE_HOST_NOT_READY;
    }
    if (arena_bytes == 0) {
        arena_bytes = MELEE_HOST_OS_PHYSICAL_MEMORY_BYTES;
    }
    arena_backing =
        melee_host_aligned_alloc(arena_bytes, MELEE_HOST_ARENA_ALIGNMENT);
    if (arena_backing == NULL) {
        return MELEE_HOST_INTERNAL_ERROR;
    }
    arena_size = arena_bytes;
    OSSetArenaLo(arena_backing);
    OSSetArenaHi((char*) arena_backing + arena_bytes);
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_os_heap_shutdown(void)
{
    if (arena_backing == NULL) {
        return MELEE_HOST_NOT_READY;
    }
    if (heap_handle >= 0) {
        OSDestroyHeap(heap_handle);
        heap_handle = -1;
    }
    OSSetCurrentHeap(-1);
    release_backing();
    configured_heaps = 0;
    /* Leave the arena pointers where they sit before any heap exists. */
    OSSetArenaLo(NULL);
    OSSetArenaHi(NULL);
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_os_heap_stats(MeleeHostOsHeapStats* out_stats)
{
    if (out_stats == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    out_stats->initialized = arena_backing != NULL;
    out_stats->arena_bytes = arena_size;
    out_stats->current_heap = __OSCurrHeap;
    out_stats->heap_count = configured_heaps;
    out_stats->free_bytes =
        heap_handle >= 0 ? (mh_s64) OSCheckHeap(heap_handle) : -1;
    out_stats->arena_exhausted =
        arena_backing != NULL && OSGetArenaLo() == OSGetArenaHi();
    return MELEE_HOST_OK;
}
