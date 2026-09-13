/* The game's mode and scene tables, as far as the host has the scenes.
 *
 * gmscdata.c keeps one table of every game mode and one of every scene, and
 * each entry names that mode's or scene's callbacks, so compiling it would link
 * the whole game.  The host keeps the same shape with the entries it can run,
 * copied from gmscdata.c, and ends them with the same GM_COUNT and GS_COUNT
 * markers.  runGameMode and gm_FindGameSceneHandler walk them exactly as they
 * walk the originals.  A mode or a scene missing here is one the host does not
 * run yet, and asking for it is refused before the game would reach through
 * the NULL it finds. */

#include <melee_host/boot.h>

#include <melee/gm/gm_1A3F.h>
#include <melee/gm/gmscdata.h>
#include <melee/gm/gmtitle.h>
#include <melee/gm/gmtitlemode.h>
#include <melee/gm/types.h>

#include <stddef.h>
#include <string.h>

static GameScene host_scenes[] = {
    {
        GS_TITLE,
        gm_Scene_Title_OnFrame,
        gm_Scene_Title_OnEnter,
        NULL,
        NULL,
    },
    {
        GS_COUNT,
        NULL,
        NULL,
        NULL,
        NULL,
    },
};

static GameMode host_modes[] = {
    {
        true,
        GM_TITLE,
        NULL,
        NULL,
        NULL,
        gm_Mode_Title_States,
    },
    {
        false,
        GM_COUNT,
        NULL,
        NULL,
        NULL,
        NULL,
    },
};

GameScene* gm_GetAllGameScenes(void)
{
    return host_scenes;
}

GameMode* gm_GetAllGameModes(void)
{
    return host_modes;
}

bool melee_host_game_mode_available(mh_u32 mode)
{
    const GameMode* entry;

    for (entry = host_modes; entry->kind != GM_COUNT; entry++) {
        if (entry->kind == mode) {
            return true;
        }
    }
    return false;
}

static struct {
    mh_u32 drawn_frames;
    MeleeHostGxFrameSink frame_sink;
    void* frame_sink_user_data;
} mode_run;

static void mode_frame_drawn(void* user_data)
{
    (void) user_data;
    mode_run.drawn_frames += 1;
    if (mode_run.frame_sink != NULL) {
        mode_run.frame_sink(mode_run.frame_sink_user_data);
    }
}

MeleeHostStatus melee_host_game_run_mode(mh_u32 mode,
                                         MeleeHostGxFrameSink frame_sink,
                                         void* user_data,
                                         MeleeHostGameModeReport* out_report)
{
    if (out_report == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    memset(out_report, 0, sizeof(*out_report));
    if (!melee_host_game_mode_available(mode)) {
        return MELEE_HOST_UNSUPPORTED;
    }
    memset(&mode_run, 0, sizeof(mode_run));
    mode_run.frame_sink = frame_sink;
    mode_run.frame_sink_user_data = user_data;

    melee_host_gx_set_frame_sink(mode_frame_drawn, NULL);
    out_report->next_mode = runGameMode((u8) mode);
    melee_host_gx_set_frame_sink(NULL, NULL);

    out_report->drawn_frames = mode_run.drawn_frames;
    return MELEE_HOST_OK;
}
