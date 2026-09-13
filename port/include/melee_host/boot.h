#ifndef MELEE_HOST_BOOT_H
#define MELEE_HOST_BOOT_H

#include <melee_host/gx.h>
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

/* The middle of gm_801A4014 for the title screen: gm_801A4BD4, the scene
 * manager's per-scene setup; gm_801A4B88 with the scene info of the title's
 * state in gmtitlemode.c; then gm_Scene_Title_OnEnter.  The state's preload and
 * on_enter, which come before it, are not run. */
void melee_host_title_scene_enter(void);

typedef struct MeleeHostTitleSceneReport {
    /* GObjs on the process lists, and those with a render callback. */
    mh_u32 gobjs;
    mh_u32 rendered;
    /* GObjs by the object they hold. */
    mh_u32 cameras;
    mh_u32 lights;
    mh_u32 fogs;
    mh_u32 models;
    mh_u32 procs;
    mh_u32 jobjs;
    mh_u32 lobjs;
} MeleeHostTitleSceneReport;

/* What the scene built, read from the GObj library's own lists. */
MeleeHostStatus
melee_host_title_scene_report(MeleeHostTitleSceneReport* out_report);

typedef struct MeleeHostTitleRunReport {
    /* Calls of the scene's on_frame: one per game frame gm_801A4D34 runs. */
    mh_u32 scene_frames;
    /* Frames the loop drew and copied to an XFB, and the triangles captured
     * in the last of them. */
    mh_u32 drawn_frames;
    mh_u32 last_frame_triangles;
    /* Retraces VI ran during the loop. */
    mh_u32 retraces;
    /* What gm_Scene_Title_OnFrame left in the state's exit data: the buttons
     * that ended the scene, or zero when it timed out. */
    mh_u32 exit_buttons;
    /* OS time the loop took, in ticks. */
    mh_u64 elapsed_ticks;
} MeleeHostTitleRunReport;

/* gm_801A4D34, the scene manager's frame loop, with the title's on_frame, until
 * the scene asks to leave.  Each drawn frame's GX capture goes to `frame_sink`,
 * when there is one, and is then cleared.  The scene has to have been entered.
 * With the OS clock frozen the loop runs as fast as it computes, unless the
 * sink paces it; otherwise it follows the wall clock, spinning while it
 * waits. */
MeleeHostStatus melee_host_title_scene_run(MeleeHostGxFrameSink frame_sink,
                                           void* user_data,
                                           MeleeHostTitleRunReport* out_report);

/* gm_801A4B60, what the scene itself calls to leave: the frame loop returns
 * after the frame in progress.  The exit data keeps what the scene last left
 * there. */
void melee_host_title_scene_request_exit(void);

#ifdef __cplusplus
}
#endif

#endif
