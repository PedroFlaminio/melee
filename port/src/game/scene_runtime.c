#include <melee_host/scene_runtime.h>

#include <melee_host/baselib.h>

#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/objalloc.h>

#include <string.h>

enum { MELEE_HOST_SCENE_MAX_OBJECTS = 256 };

/* One registry entry per host-owned GObj.  The GObj stores the entry as its
 * user data, so the original library owns the lifetime and the host only sees
 * handles. */
typedef struct SceneObjectSlot {
    HSD_GObj* gobj;
    MeleeHostSceneProc proc;
    void* user_data;
    u16 generation;
    bool in_use;
} SceneObjectSlot;

static SceneObjectSlot slots[MELEE_HOST_SCENE_MAX_OBJECTS];
static bool runtime_initialized;
static u64 paused_links;
static mh_u64 frame_count;

static MeleeHostSceneObject handle_of(u32 index)
{
    return (MeleeHostSceneObject) (((u32) slots[index].generation << 16) |
                                   (index + 1));
}

static SceneObjectSlot* slot_of(MeleeHostSceneObject object)
{
    u32 index = (object & 0xFFFFU);
    SceneObjectSlot* slot;

    if (index == 0 || index > MELEE_HOST_SCENE_MAX_OBJECTS) {
        return NULL;
    }
    index -= 1;
    slot = &slots[index];
    if (!slot->in_use || slot->generation != (u16) (object >> 16)) {
        return NULL;
    }
    return slot;
}

static void host_object_proc(HSD_GObj* gobj)
{
    SceneObjectSlot* slot = HSD_GObjGetUserData(gobj);

    if (slot != NULL && slot->proc != NULL) {
        slot->proc(slot->user_data);
    }
}

/* Invoked by GObj_RemoveUserData while the original library tears the GObj
 * down, including the deferred teardown that HSD_GObj_RunProcs performs when a
 * process frees its own GObj. */
static void host_object_release(void* user_data)
{
    SceneObjectSlot* slot = user_data;

    slot->gobj = NULL;
    slot->proc = NULL;
    slot->user_data = NULL;
    slot->in_use = false;
    slot->generation += 1;
}

MeleeHostStatus melee_host_scene_runtime_init(void)
{
    HSD_GObjLibInitDataType init_data;

    if (runtime_initialized) {
        return MELEE_HOST_OK;
    }
    if (melee_host_baselib_bootstrap() != MELEE_HOST_OK) {
        return MELEE_HOST_INTERNAL_ERROR;
    }

    memset(slots, 0, sizeof(slots));
    paused_links = 0;
    frame_count = 0;

    HSD_GObjSetInitDefaults(&init_data);
    init_data.gproc_pri_max = MELEE_HOST_SCENE_PROC_PRIORITY_MAX;
    init_data.unk_2 = &paused_links;
    HSD_GObjInit(&init_data);

    if (HSD_GObjPLinkHead == NULL || HSD_GObj_GObjProcHead == NULL ||
        HSD_GObj_ProcList == NULL)
    {
        return MELEE_HOST_INTERNAL_ERROR;
    }
    runtime_initialized = true;
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_scene_runtime_run_frame(void)
{
    if (!runtime_initialized) {
        return MELEE_HOST_NOT_READY;
    }
    HSD_GObj_RunProcs();
    frame_count += 1;
    return MELEE_HOST_OK;
}

MeleeHostStatus
melee_host_scene_runtime_stats(MeleeHostSceneRuntimeStats* out_stats)
{
    if (out_stats == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (!runtime_initialized) {
        return MELEE_HOST_NOT_READY;
    }
    out_stats->frame_count = frame_count;
    out_stats->objects_live = HSD_ObjAllocGetUsing(&gobj_alloc_data);
    out_stats->objects_pooled = HSD_ObjAllocGetFreed(&gobj_alloc_data);
    out_stats->procs_live = HSD_ObjAllocGetUsing(&gobjproc_alloc_data);
    out_stats->procs_pooled = HSD_ObjAllocGetFreed(&gobjproc_alloc_data);
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_scene_runtime_set_paused_links(mh_u64 mask)
{
    if (!runtime_initialized) {
        return MELEE_HOST_NOT_READY;
    }
    paused_links = mask;
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_scene_runtime_get_paused_links(mh_u64* out_mask)
{
    if (out_mask == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (!runtime_initialized) {
        return MELEE_HOST_NOT_READY;
    }
    *out_mask = paused_links;
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_scene_runtime_add_object(
    mh_u16 classifier, mh_u8 p_link, mh_u8 priority, mh_u8 proc_priority,
    MeleeHostSceneProc proc, void* user_data,
    MeleeHostSceneObject* out_object)
{
    u32 index;
    SceneObjectSlot* slot = NULL;
    HSD_GObj* gobj;

    if (out_object == NULL || proc == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (p_link > MELEE_HOST_SCENE_P_LINK_MAX ||
        proc_priority > MELEE_HOST_SCENE_PROC_PRIORITY_MAX)
    {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (!runtime_initialized) {
        return MELEE_HOST_NOT_READY;
    }

    for (index = 0; index < MELEE_HOST_SCENE_MAX_OBJECTS; ++index) {
        if (!slots[index].in_use) {
            slot = &slots[index];
            break;
        }
    }
    if (slot == NULL) {
        return MELEE_HOST_INTERNAL_ERROR;
    }

    gobj = GObj_Create(classifier, p_link, priority);
    if (gobj == NULL) {
        return MELEE_HOST_INTERNAL_ERROR;
    }

    slot->gobj = gobj;
    slot->proc = proc;
    slot->user_data = user_data;
    slot->in_use = true;
    GObj_InitUserData(gobj, MELEE_HOST_SCENE_USER_DATA_KIND,
                      host_object_release, slot);
    if (HSD_GObj_SetupProc(gobj, host_object_proc, proc_priority) == NULL) {
        HSD_GObjFree(gobj);
        return MELEE_HOST_INTERNAL_ERROR;
    }
    *out_object = handle_of(index);
    return MELEE_HOST_OK;
}

MeleeHostStatus
melee_host_scene_runtime_remove_object(MeleeHostSceneObject object)
{
    SceneObjectSlot* slot;

    if (!runtime_initialized) {
        return MELEE_HOST_NOT_READY;
    }
    slot = slot_of(object);
    if (slot == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    HSD_GObjFree(slot->gobj);
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_scene_runtime_object_is_live(
    MeleeHostSceneObject object, bool* out_live)
{
    if (out_live == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (!runtime_initialized) {
        return MELEE_HOST_NOT_READY;
    }
    *out_live = slot_of(object) != NULL;
    return MELEE_HOST_OK;
}
