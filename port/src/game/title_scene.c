/* The title screen, run by gmtitle.c. */

#include <melee_host/boot.h>

#include <melee_host/gx.h>

#include <dolphin/os.h>
#include <dolphin/vi.h>
#include <melee/gm/gmscene.h>
#include <melee/gm/gmtitle.h>
#include <melee/gm/gmtitlemode.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/objalloc.h>

#include <string.h>

/* lobj.h declares the class as hsdLobj, but lobj.c defines hsdLObj. */
extern HSD_LObjInfo hsdLObj;

static bool title_entered;

static struct {
    mh_u32 scene_frames;
    mh_u32 drawn_frames;
    mh_u32 last_frame_triangles;
    /* The host's own sink, handed each frame before the capture clears. */
    MeleeHostGxFrameSink frame_sink;
    void* frame_sink_user_data;
} title_run;

void melee_host_title_scene_enter(void)
{
    /* gmtitlemode.c's only state; gm_801A4014 finds it by id 0. */
    GameModeState* const state = &gm_Mode_Title_States[0];

    gm_801A4BD4();
    gm_801A4B88(&state->info);
    gm_Scene_Title_OnEnter(state->info.enter_data);
    title_entered = true;
}

MeleeHostStatus
melee_host_title_scene_report(MeleeHostTitleSceneReport* out_report)
{
    u32 link;
    HSD_GObj* gobj;

    if (out_report == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    memset(out_report, 0, sizeof(*out_report));
    if (HSD_GObjPLinkHead == NULL) {
        return MELEE_HOST_NOT_READY;
    }
    /* GObj_Create accepts p_link up to p_link_max, inclusive. */
    for (link = 0; link <= HSD_GObjLibInitData.p_link_max; link++) {
        for (gobj = HSD_GObjPLinkHead[link]; gobj != NULL; gobj = gobj->next) {
            out_report->gobjs += 1;
            if (gobj->render_cb != NULL) {
                out_report->rendered += 1;
            }
            if (gobj->obj_kind == HSD_GOBJ_OBJ_NONE) {
                continue;
            }
            if (gobj->obj_kind == HSD_GObj_CameraKind) {
                out_report->cameras += 1;
            } else if (gobj->obj_kind == (u8) HSD_GObj_LightKind) {
                out_report->lights += 1;
            } else if (gobj->obj_kind == (u8) HSD_GObj_FogKind) {
                out_report->fogs += 1;
            } else if (gobj->obj_kind == HSD_GObj_JObjKind) {
                out_report->models += 1;
            }
        }
    }
    out_report->procs = HSD_ObjAllocGetUsing(&gobjproc_alloc_data);
    out_report->jobjs = hsdJObj.parent.parent.head.nb_exist;
    out_report->lobjs = hsdLObj.parent.parent.head.nb_exist;
    return MELEE_HOST_OK;
}

static void title_on_frame(void)
{
    title_run.scene_frames += 1;
    gm_Scene_Title_OnFrame();
}

static void title_frame_drawn(void* user_data)
{
    (void) user_data;
    title_run.drawn_frames += 1;
    title_run.last_frame_triangles = (mh_u32) melee_host_gx_triangle_count();
    if (title_run.frame_sink != NULL) {
        title_run.frame_sink(title_run.frame_sink_user_data);
    }
}

MeleeHostStatus melee_host_title_scene_run(MeleeHostGxFrameSink frame_sink,
                                           void* user_data,
                                           MeleeHostTitleRunReport* out_report)
{
    GameModeState* const state = &gm_Mode_Title_States[0];
    u32 first_retrace;
    OSTime start;

    if (out_report == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (!title_entered) {
        return MELEE_HOST_NOT_READY;
    }
    memset(out_report, 0, sizeof(*out_report));
    memset(&title_run, 0, sizeof(title_run));
    title_run.frame_sink = frame_sink;
    title_run.frame_sink_user_data = user_data;

    first_retrace = VIGetRetraceCount();
    start = OSGetTime();
    melee_host_gx_set_frame_sink(title_frame_drawn, NULL);
    /* gm_801A4014 passes the scene's on_frame; the host only counts its
     * calls. */
    gm_801A4D34(title_on_frame, &state->info);
    melee_host_gx_set_frame_sink(NULL, NULL);

    out_report->scene_frames = title_run.scene_frames;
    out_report->drawn_frames = title_run.drawn_frames;
    out_report->last_frame_triangles = title_run.last_frame_triangles;
    out_report->retraces = VIGetRetraceCount() - first_retrace;
    out_report->exit_buttons = (mh_u32) *(int*) state->info.exit_data;
    out_report->elapsed_ticks = (mh_u64) (OSGetTime() - start);
    return MELEE_HOST_OK;
}

void melee_host_title_scene_request_exit(void)
{
    gm_801A4B60();
}
