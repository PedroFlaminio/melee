/* The part of gmMain that builds the game's memory before the first scene.
 *
 * On the console main() runs OSInit, which leaves an arena between
 * OSGetArenaLo and OSGetArenaHi.  HSD_InitComponent carves the XFBs, the GX
 * FIFO, the audio heap and the HSD heap out of it, and the lb layer lays its
 * own heaps over what remains: lbMemory_8001564C sets up the allocator that
 * manages ARAM and the scene heaps, lbHeap_80015F3C maps the heaps, and a
 * scene's setup ends in lbHeap_80015900, which creates them.  This file runs
 * that sequence, in that order, on an arena from host memory.  What it leaves
 * out is said where it is left out.
 */

#include <melee_host/boot.h>

#include <melee_host/dolphin_os.h>
#include <melee_host/os_heap.h>

#include <dolphin/dvd.h>
#include <dolphin/gx/GXFrameBuffer.h>
#include <dolphin/os.h>
#include <dolphin/pad.h>
#include <melee/db/db.h>
#include <melee/gm/gmmain_lib.h>
#include <melee/lb/lbdvd.h>
#include <melee/lb/lb_0195.h>
#include <melee/lb/lbarchive.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbheap.h>
#include <melee/lb/lbmemory.h>
#include <melee/sc/types.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/fog.h>
#include <sysdolphin/baselib/initialize.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/sislib.h>
#include <sysdolphin/baselib/sobjlib.h>
#include <sysdolphin/baselib/video.h>

/* After dolphin/os.h: this header repeats the SDK's clock types for code that
 * does not include it, and skips them when it has been included. */
#include <melee_host/dolphin_time.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

enum { LB_HEAP_COUNT = 6 };

static bool boot_memory_ready;
static size_t boot_arena_bytes;

/* gmMain_8015FD24's storage, gmMain_8046B108 and gmMain_8046B1F8: five queued
 * pad samples and twelve rumble entries. */
static HSD_PadData boot_pad_queue[5];
static HSD_PadRumbleListData boot_rumble_list[12];

/* gmMain_8015FD24, which is static in gmmain.c: the pad library, and the
 * stick and trigger clamps the game plays with. */
static void boot_pad_init(void)
{
    PADSetSpec(5);
    HSD_PadInit(5, boot_pad_queue, 12, boot_rumble_list);
    HSD_PadLibData.clamp_stickType = 0;
    HSD_PadLibData.clamp_stickShift = 1;
    HSD_PadLibData.clamp_stickMax = 80;
    HSD_PadLibData.clamp_stickMin = 0;
    HSD_PadLibData.scale_stick = 80;
    HSD_PadLibData.clamp_analogLRShift = 1;
    HSD_PadLibData.clamp_analogLRMax = 140;
    HSD_PadLibData.clamp_analogLRMin = 0;
    HSD_PadLibData.scale_analogLR = 140;
}

/* gmMain_8015FDA0, gmMain's post-retrace callback, which does nothing. */
static void boot_post_retrace(u32 retrace_count)
{
    (void) retrace_count;
}

/* lbfile.c calls this in a loop until a disc read's callback fires, lbdvd.c
 * while a preload is still arriving, and gm_801A4D34 while it waits for the
 * next frame's pad sample.  On the console the read finishes by interrupt, the
 * sample arrives by alarm, and the poll only watches for drive errors, the
 * reset button and the memory card.  On the host a read finishes when the
 * scheduler steps, so stepping it is most of the poll's job.  Drive-error
 * screens, reset and card polling are not part of the host yet. */
void lb_800195D0(void)
{
    s64 fire;

    if (melee_host_dvd_step_backend() == MELEE_HOST_UNSUPPORTED) {
        OSPanic(__FILE__, __LINE__,
                "a disc read is waiting and no DVD backend is active");
    }
    /* A frozen clock stands still while the game works, so the next pad
     * sample only comes if the wait itself moves time.  gm_801A4D34 polls here
     * once before it runs the frames queued so far and once after, when the
     * queue is empty, so the clock jumps once a frame. */
    if (melee_host_os_time_frozen() && HSD_PadGetRawQueueCount() == 0 &&
        melee_host_os_next_alarm(&fire))
    {
        melee_host_os_time_advance(fire - OSGetTime());
    }
    /* The console keeps taking interrupts while it waits, alarms included. */
    melee_host_os_fire_alarms();
}

MeleeHostStatus melee_host_boot_memory_init(size_t arena_bytes)
{
    MeleeHostStatus status;

    if (boot_memory_ready) {
        return MELEE_HOST_OK;
    }

    /* OSInit's share: the arena, with nothing carved from it yet. */
    status = melee_host_os_arena_init(arena_bytes);
    if (status != MELEE_HOST_OK) {
        return status;
    }
    boot_arena_bytes =
        (size_t) ((char*) OSGetArenaHi() - (char*) OSGetArenaLo());

    /* gmMain_8015FDA4, which is static in gmmain.c: a retail disc has no
     * /develop.ini, and the game then runs at the master level.  The debug
     * levels a development disc selects with the launch buttons are not
     * mirrored. */
    if (DVDConvertPathToEntrynum("/develop.ini") == -1) {
        DbLevel = DbLKind_Master;
    } else {
        OSReport("host boot: /develop.ini present; debug levels are not "
                 "selected on the host\n");
    }

    /* gmMain, in order. */
    HSD_SetInitParameter(HSD_INIT_XFB_MAX_NUM, 2);
    HSD_SetInitParameter(HSD_INIT_RENDER_MODE_OBJ, &GXNtsc480IntDf);
    HSD_SetInitParameter(HSD_INIT_FIFO_SIZE, 0x40000);
    HSD_SetInitParameter(HSD_INIT_HEAP_MAX_NUM, 4);
    HSD_AllocateXFB(2, &GXNtsc480IntDf);
    /* The FIFO is carved out of the arena as on the console, but it is not
     * handed to GXInit: the host GX records state and keeps no command
     * buffer. */
    HSD_AllocateFifo(0x40000);
    HSD_InitComponent();
    /* The audio setup takes its ARAM before the lb allocator claims the rest,
     * and it sets the budget lbAudioAx_80027168 checks every sound bank load
     * against. */
    lbAudioAx_8002838C();
    /* The pad library, and the periodic alarm that samples it once a frame
     * (lb_80019628).  From here on a wait for the disc takes pad samples too. */
    lb_80019AAC(boot_pad_init);
    HSD_VISetUserPostRetraceCallback(boot_post_retrace);
    /* What makes a drawn XFB displayable once GX reports it finished. */
    HSD_VISetUserGXDrawDoneCallback(HSD_VIDrawDoneXFB);
    HSD_VISetBlack(0);
    lbMemory_8001564C();
    lbHeap_80015F3C();
    lbDvd_80018F68();
    gmMainLib_8015FCC0();
    HSD_SisLib_803A6048(0xC000);
    gmMainLib_8015FBA4();
    /* gmMain ends here.  lbHeap_80015900 is how the first scene's heap setup
     * ends (lbDvd_80018CF4); with every scene heap still transient, it
     * creates the main heap and the ARAM heap. */
    lbHeap_80015900();

    boot_memory_ready = true;
    return MELEE_HOST_OK;
}

MeleeHostStatus
melee_host_boot_memory_stats(MeleeHostBootMemoryStats* out_stats)
{
    int heap;

    if (out_stats == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (!boot_memory_ready) {
        return MELEE_HOST_NOT_READY;
    }
    out_stats->arena_bytes = boot_arena_bytes;
    out_stats->lb_heaps_created = 0;
    for (heap = 0; heap < LB_HEAP_COUNT; heap++) {
        if (lbHeap_80015BB8(heap) == LbHeapStatus_Create) {
            out_stats->lb_heaps_created += 1;
        }
    }
    return MELEE_HOST_OK;
}

static mh_u32 count_jobjs(HSD_JObj* jobj, bool animated_only)
{
    mh_u32 count = 0;

    for (; jobj != NULL; jobj = jobj->next) {
        if (!animated_only || jobj->aobj != NULL) {
            count += 1;
        }
        if (!(jobj->flags & JOBJ_INSTANCE)) {
            count += count_jobjs(jobj->child, animated_only);
        }
    }
    return count;
}

/* gmTitle_801A165C and fn_801A1498_inline: load the model, attach its three
 * animations, request a frame and interpret once. */
static void load_title_model(StaticModelDesc* model, f32 frame,
                             mh_u32* out_jobjs, mh_u32* out_animated)
{
    HSD_JObj* const jobj = HSD_JObjLoadJoint(model->joint);

    if (jobj == NULL) {
        return;
    }
    HSD_JObjAddAnimAll(jobj, model->animjoint, model->matanim_joint,
                       model->shapeanim_joint);
    HSD_JObjReqAnimAll(jobj, frame);
    HSD_JObjAnimAll(jobj);
    *out_jobjs = count_jobjs(jobj, false);
    *out_animated = count_jobjs(jobj, true);
    HSD_JObjRemoveAll(jobj);
}

MeleeHostStatus
melee_host_boot_load_title_archive(MeleeHostTitleArchiveReport* out_report)
{
    /* The storage gmtitle.c keeps: two static models, a camera, a light
     * table, a fog and the title mark. */
    StaticModelDesc title = { 0 };
    StaticModelDesc background = { 0 };
    HSD_CameraDescPerspective* camera = NULL;
    LightList** lights = NULL;
    HSD_FogDesc* fog_desc = NULL;
    HSD_SObjDesc* mark = NULL;
    HSD_Archive* archive;
    mh_u32 count;

    if (out_report == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (!boot_memory_ready) {
        return MELEE_HOST_NOT_READY;
    }
    memset(out_report, 0, sizeof(*out_report));

    /* The game opens GmTtAll.usd when the language setting is English.
     * gmTitle_801A1AC0 ends its list with a plain 0; a 64-bit host reads the
     * terminator back through va_arg as a pointer, so NULL is the same end
     * marker at the width the callee reads. */
    archive = lbArchive_LoadSymbols(
        "GmTtAll.usd", &title.joint, "TtlMoji_Top_joint", &title.animjoint,
        "TtlMoji_Top_animjoint", &title.matanim_joint,
        "TtlMoji_Top_matanim_joint", &title.shapeanim_joint,
        "TtlMoji_Top_shapeanim_joint", &camera, "ScTitle_cam_int1_camera",
        &lights, "ScTitle_scene_lights", &fog_desc, "ScTitle_fog",
        &background.joint, "TtlBg_Top_joint", &background.animjoint,
        "TtlBg_Top_animjoint", &background.matanim_joint,
        "TtlBg_Top_matanim_joint", &background.shapeanim_joint,
        "TtlBg_Top_shapeanim_joint", &mark, "TitleMark_sobjdesc", NULL);
    if (archive == NULL) {
        return MELEE_HOST_IO_ERROR;
    }
    out_report->file_bytes = archive->header.file_size;

    count = 0;
    count += title.joint != NULL;
    count += title.animjoint != NULL;
    count += title.matanim_joint != NULL;
    count += title.shapeanim_joint != NULL;
    count += camera != NULL;
    count += lights != NULL;
    count += fog_desc != NULL;
    count += background.joint != NULL;
    count += background.animjoint != NULL;
    count += background.matanim_joint != NULL;
    count += background.shapeanim_joint != NULL;
    count += mark != NULL;
    out_report->symbols_resolved = count;

    /* loop_settings_0 starts the title at frame 0; the background's inline
     * setup starts it at loop_settings_1.start_frame, also 0. */
    load_title_model(&title, 0.0F, &out_report->title_jobjs,
                     &out_report->title_animated_jobjs);
    load_title_model(&background, 0.0F, &out_report->background_jobjs,
                     &out_report->background_animated_jobjs);

    /* gmTitle_801A185C loads the camera through lb_80013B14, which only
     * adjusts aspect and scissor after this call. */
    if (camera != NULL) {
        HSD_CObj* const cobj = HSD_CObjLoadDesc((HSD_CObjDesc*) camera);
        if (cobj != NULL) {
            out_report->camera_loaded = 1;
            hsdDelete(cobj);
        }
    }

    /* gmTitle_801A19AC's lb_80011AC4: one light per list, its first
     * animation attached when it has any. */
    if (lights != NULL) {
        LightList** list;
        for (list = lights; *list != NULL; list++) {
            HSD_LObj* const lobj = HSD_LObjLoadDesc((*list)->desc);
            if (lobj == NULL) {
                continue;
            }
            if ((*list)->anims != NULL) {
                HSD_LObjAddAnimAll(lobj, (*list)->anims[0]);
            }
            out_report->lights += 1;
            HSD_LObjRemoveAll(lobj);
        }
    }

    /* gmTitle_801A1A3C. */
    if (fog_desc != NULL) {
        HSD_Fog* const fog = HSD_FogLoadDesc(fog_desc);
        if (fog != NULL) {
            out_report->fog_loaded = 1;
            hsdDelete(fog);
        }
    }

    out_report->mark_has_image = mark != NULL && mark->image != NULL;
    return MELEE_HOST_OK;
}
