/* The title screen, run by gmtitle.c. */

#include <melee_host/boot.h>

#include <melee/gm/gmscene.h>
#include <melee/gm/gmtitle.h>

void melee_host_title_scene_enter(void)
{
    /* gm_801A4014: the scene manager's setup, then the scene's own entry. */
    gm_801A4BD4();
    gm_Scene_Title_OnEnter(NULL);
}
