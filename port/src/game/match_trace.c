#include <melee_host/match_trace.h>

#include <melee/ft/ftlib.h>
#include <melee/gm/gmmain_lib.h>
#include <melee/gm/gmresult.h>
#include <melee/gm/types.h>
#include <melee/ft/inlines.h>
#include <melee/pl/player.h>

bool melee_host_match_fighter_position(mh_u32 slot, mh_f32* x, mh_f32* y,
                                        mh_f32* z)
{
    HSD_GObj* fighter;
    Vec3 position;

    if (slot >= 4 || x == NULL || y == NULL || z == NULL) {
        return false;
    }
    fighter = Player_GetEntity((s32) slot);
    if (fighter == NULL) {
        return false;
    }
    ftLib_80086644(fighter, &position);
    *x = position.x;
    *y = position.y;
    *z = position.z;
    return true;
}

bool melee_host_match_fighter_motion(mh_u32 slot, mh_s32* motion)
{
    HSD_GObj* fighter;

    if (slot >= 4 || motion == NULL) {
        return false;
    }
    fighter = Player_GetEntity((s32) slot);
    if (fighter == NULL) {
        return false;
    }
    *motion = GET_FIGHTER(fighter)->motion_id;
    return true;
}

bool melee_host_match_player_falls(mh_u32 slot, mh_s32* falls)
{
    if (slot >= 4 || falls == NULL ||
        Player_GetPlayerSlotType((s32) slot) == Gm_PKind_NA)
    {
        return false;
    }
    *falls = Player_GetFalls((s32) slot);
    return true;
}

void melee_host_match_rules(mh_u32* mode, mh_u32* time_limit,
                            mh_u32* stock_count)
{
    GameRules* rules = gmMainLib_GetGameRules();

    *mode = rules->mode;
    *time_limit = rules->time_limit;
    *stock_count = rules->stock_count;
}

bool melee_host_match_result(mh_u32* outcome, mh_u32* winners,
                             mh_u32* first_winner, mh_s32* stocks_p1,
                             mh_s32* stocks_p2)
{
    MatchEnd* end = fn_80174274();

    if (end == NULL) {
        return false;
    }
    *outcome = end->outcome;
    *winners = end->n_winners;
    *first_winner = end->winners[0];
    *stocks_p1 = end->player_standings[0].stocks;
    *stocks_p2 = end->player_standings[1].stocks;
    return true;
}
