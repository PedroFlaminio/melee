#ifndef MELEE_HOST_BOOT_H
#define MELEE_HOST_BOOT_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The part of gmMain that builds the game's memory before a scene runs, and
 * the heap setup the first scene then performs: an arena from host memory,
 * HSD_InitComponent, lbMemory_8001564C, lbHeap_80015F3C and lbHeap_80015900,
 * in that order.
 *
 * It moves the OS arena and replaces HSD's heaps, so it belongs in a process of
 * its own rather than beside tests that assume the headless bootstrap. */

typedef struct MeleeHostBootMemoryStats {
    mh_u64 arena_bytes;
    /* lbHeap slots in the created state.  After boot that is the main heap
     * and the ARAM heap; the scene heaps join once a scene keeps them. */
    mh_u32 lb_heaps_created;
} MeleeHostBootMemoryStats;

/* `arena_bytes` of zero gives the console's main memory size. */
MeleeHostStatus melee_host_boot_memory_init(size_t arena_bytes);
MeleeHostStatus
melee_host_boot_memory_stats(MeleeHostBootMemoryStats* out_stats);

typedef struct MeleeHostTitleArchiveReport {
    mh_u32 file_bytes;
    /* Of the twelve symbols gmTitle_801A1AC0 names. */
    mh_u32 symbols_resolved;
    mh_u32 title_jobjs;
    mh_u32 title_animated_jobjs;
    mh_u32 background_jobjs;
    mh_u32 background_animated_jobjs;
    mh_u32 lights;
    mh_u8 camera_loaded;
    mh_u8 fog_loaded;
    mh_u8 mark_has_image;
} MeleeHostTitleArchiveReport;

/* The title screen's lbArchive_LoadSymbols call from gmTitle_801A1AC0, with
 * its file and symbol list, through the game's own lbArchive and lbFile over
 * devcom and the host DVD.  Then the loads gm_Scene_Title_OnEnter performs on
 * the result, with each object freed again.  Needs the boot memory and an
 * active DVD backend; a missing symbol is fatal, as it is in the game. */
MeleeHostStatus
melee_host_boot_load_title_archive(MeleeHostTitleArchiveReport* out_report);

/* Reads the game data that lives in main.dol rather than in a disc file, such
 * as the SIS font atlas, from the user's extracted DOL. */
MeleeHostStatus melee_host_boot_load_dol_data(const char* dol_path);

/* gm_Scene_Title_OnEnter, the title screen's own scene entry. */
void melee_host_title_scene_enter(void);

#ifdef __cplusplus
}
#endif

#endif
