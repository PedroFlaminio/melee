#ifndef MELEE_HOST_SCENE_RUNTIME_H
#define MELEE_HOST_SCENE_RUNTIME_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    /* gmScene_Init raises the process priority ceiling before calling
     * HSD_GObjInit; the link ceilings keep HSD_GObjSetInitDefaults' values. */
    MELEE_HOST_SCENE_PROC_PRIORITY_MAX = 0x18,
    MELEE_HOST_SCENE_P_LINK_MAX = 0x3F,
    MELEE_HOST_SCENE_GX_LINK_MAX = 0x3F,
    /* User-data kind reserved for host-owned objects.  The game only uses
     * small kinds, so this cannot collide with ported gameplay code. */
    MELEE_HOST_SCENE_USER_DATA_KIND = 0x7F,
};

/* Opaque handle into the host's object registry.  Zero is never valid.  The
 * host never hands out an HSD_GObj pointer, so a 32-bit handle stays stable
 * regardless of the runtime's pointer width. */
typedef mh_u32 MeleeHostSceneObject;

typedef void (*MeleeHostSceneProc)(void* user_data);

typedef struct MeleeHostSceneRuntimeStats {
    mh_u64 frame_count;
    mh_u32 objects_live;
    mh_u32 objects_pooled;
    mh_u32 procs_live;
    mh_u32 procs_pooled;
} MeleeHostSceneRuntimeStats;

/* Brings up the original HSD object library with Melee's own priority
 * ceilings.  Idempotent: HSD_GObjInit allocates its link tables on every call,
 * so the facade runs it once. */
MeleeHostStatus melee_host_scene_runtime_init(void);

/* Runs HSD_GObj_RunProcs once, which is the original per-frame pass over every
 * queued process in priority order.  At the same boundary it drains a pending
 * GX draw-done fence and advances one VI retrace when video is initialized. */
MeleeHostStatus melee_host_scene_runtime_run_frame(void);

MeleeHostStatus
melee_host_scene_runtime_stats(MeleeHostSceneRuntimeStats* out_stats);

/* Each set bit suspends the processes of the matching p_link.  This is the
 * mask the original scene controller points HSD_GObjLibInitData.unk_2 at. */
MeleeHostStatus melee_host_scene_runtime_set_paused_links(mh_u64 mask);
MeleeHostStatus melee_host_scene_runtime_get_paused_links(mh_u64* out_mask);

/* Creates a GObj carrying one host process.  `proc` runs once per frame from
 * inside HSD_GObj_RunProcs, so it observes the original ordering rules. */
MeleeHostStatus melee_host_scene_runtime_add_object(
    mh_u16 classifier, mh_u8 p_link, mh_u8 priority, mh_u8 proc_priority,
    MeleeHostSceneProc proc, void* user_data,
    MeleeHostSceneObject* out_object);

/* Releases the object.  Calling this from inside its own process defers the
 * release to the end of that process, which is the original GObj behaviour. */
MeleeHostStatus melee_host_scene_runtime_remove_object(
    MeleeHostSceneObject object);

MeleeHostStatus melee_host_scene_runtime_object_is_live(
    MeleeHostSceneObject object, bool* out_live);

#ifdef __cplusplus
}
#endif

#endif
