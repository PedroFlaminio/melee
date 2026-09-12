#ifndef MELEE_HOST_OS_HEAP_H
#define MELEE_HOST_OS_HEAP_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The original OSAlloc heap, running on host memory.
 *
 * The console carves its heap out of a fixed 24 MB main memory arena.  The
 * host has no such map, so this facade allocates a backing block and hands it
 * to the SDK's own arena and heap code.  Everything above that point is the
 * original allocator: the same cell layout, the same 32-byte header and the
 * same free-list behaviour the game's allocation patterns were written for. */

enum {
    /* GameCube main memory, which is what OSGetPhysicalMemSize reports. */
    MELEE_HOST_OS_PHYSICAL_MEMORY_BYTES = 0x01800000,
};

typedef struct MeleeHostOsHeapStats {
    bool initialized;
    size_t arena_bytes;
    mh_s32 current_heap;
    mh_s32 heap_count;
    /* Bytes OSCheckHeap reports free on the current heap, or -1 when the heap
     * is not consistent. */
    mh_s64 free_bytes;
    /* True once the heap has taken the whole arena, which is the state the
     * boot path leaves behind. */
    bool arena_exhausted;
} MeleeHostOsHeapStats;

/* Builds the arena and one heap covering it, then makes that heap current.
 * `arena_bytes` defaults to the console's main memory size when zero. */
MeleeHostStatus melee_host_os_heap_init(size_t arena_bytes, mh_s32 max_heaps);
MeleeHostStatus melee_host_os_heap_shutdown(void);
MeleeHostStatus melee_host_os_heap_stats(MeleeHostOsHeapStats* out_stats);

#ifdef __cplusplus
}
#endif

#endif
