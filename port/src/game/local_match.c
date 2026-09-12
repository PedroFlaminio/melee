#include <melee_host/local_match.h>

#include <melee/gm/gmmain_lib.h>
#include <melee/gr/forward.h>
#include <melee/mn/types.h>
#include <melee/pl/forward.h>
#include <melee/pl/player.h>

#include <string.h>

static bool prepared;
/* This retains the exact game-owned layout without pulling the complete VS
 * state machine (and its as-yet-unported stage catalog) into host binaries. */
static StartMeleeData prepared_start;

static void setup_player(PlayerInitData* player, s8 character, u8 slot_type,
                         u8 slot, s8 stocks)
{
    memset(player, 0, sizeof(*player));
    player->ckind = character;
    player->slot_type = slot_type;
    player->stocks = stocks;
    player->slot = slot;
    player->spawn_pos = -1;
    player->handicap = 9;
    player->nametag = GM_NAMETAG_NONE;
    player->xC_b1 = true;
    player->cpu_kind = 4;
    player->attack_ratio = 1.0F;
    player->defense_ratio = 1.0F;
    player->model_scale = 1.0F;
}

MeleeHostStatus melee_host_prepare_local_two_player_match(
    mh_s8 first_character, mh_s8 second_character, mh_u16 stage_kind)
{
    GameRules* game_rules = gmMainLib_GetGameRules();
    StartMeleeData* start = &prepared_start;

    if (game_rules == NULL) {
        return MELEE_HOST_NOT_READY;
    }
    if (first_character < 0 || first_character >= CKind_Playable_Count ||
        second_character < 0 || second_character >= CKind_Playable_Count ||
        stage_kind >= St_Kind_Last || game_rules->mode > 3)
    {
        return MELEE_HOST_INVALID_ARGUMENT;
    }

    memset(start, 0, sizeof(*start));
    start->rules.match_kind = game_rules->mode;
    start->rules.timer_enabled = game_rules->mode != 1 &&
                                 game_rules->time_limit != 0;
    start->rules.time_limit = start->rules.timer_enabled
                                  ? (u32) game_rules->time_limit * 60U
                                  : 0;
    start->rules.is_stock = game_rules->mode == 1;
    start->rules.friendly_fire = game_rules->friendly_fire != 0;
    start->rules.disable_pausing = game_rules->pause == 0;
    start->rules.is_vs = true;
    start->rules.stkind = stage_kind;
    start->rules.item_freq = 2;
    start->rules.sd_penalty = 0;
    start->rules.x20 = UINT64_MAX;
    start->rules.x2C = 1.0F;
    start->rules.x30 = (float) game_rules->damage_ratio / 10.0F;
    start->rules.game_speed = 1.0F;

    for (u8 slot = 0; slot < GM_MAX_PLAYERS; ++slot) {
        setup_player(&start->players[slot], ChKind_None, Gm_PKind_NA, slot, 0);
    }
    setup_player(&start->players[0], first_character, Gm_PKind_Human, 0,
                 (s8) game_rules->stock_count);
    setup_player(&start->players[1], second_character, Gm_PKind_Human, 1,
                 (s8) game_rules->stock_count);
    prepared = true;
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_prepared_match_get(MeleeHostPreparedMatch* out_match)
{
    if (out_match == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    if (!prepared) {
        return MELEE_HOST_NOT_READY;
    }

    const StartMeleeData* start = &prepared_start;
    memset(out_match, 0, sizeof(*out_match));
    out_match->match_kind = start->rules.match_kind;
    out_match->timer_enabled = start->rules.timer_enabled != 0;
    out_match->time_limit_seconds = start->rules.time_limit;
    out_match->stage_kind = start->rules.stkind;
    for (u8 slot = 0; slot < GM_MAX_PLAYERS; ++slot) {
        out_match->characters[slot] = start->players[slot].ckind;
        out_match->player_kinds[slot] = start->players[slot].slot_type;
        out_match->stocks[slot] = start->players[slot].stocks;
        if (start->players[slot].slot_type != Gm_PKind_NA) {
            ++out_match->player_count;
        }
    }
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_initialize_prepared_player_state(void)
{
    if (!prepared) {
        return MELEE_HOST_NOT_READY;
    }
    Player_InitAllPlayers();
    for (u8 slot = 0; slot < GM_MAX_PLAYERS; ++slot) {
        StaticPlayer* player = Player_GetPtrForSlot(slot);
        const PlayerInitData* input = &prepared_start.players[slot];
        player->ckind = input->ckind;
        player->pkind = input->slot_type;
        player->costume_id = input->color;
        player->controller_index = (s8) slot;
        player->team = input->team;
        player->player_id = slot;
        player->cpu_level = input->cpu_level;
        player->cpu_type = input->cpu_kind;
        player->handicap = input->handicap;
        player->attack_ratio = input->attack_ratio;
        player->defense_ratio = input->defense_ratio;
        player->model_scale = input->model_scale;
        player->staminas.byName.damage_percent = (s16) input->damage;
        player->staminas.byName.damage_percent_alt_or_start_hp =
            (s16) input->damage1;
        player->staminas.byName.stamina = (s16) input->hp;
        player->stocks = input->stocks;
        player->nametag_slot_id = input->nametag;
        player->flags.b0 = input->rumble_enabled;
    }
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_player_state_get(mh_u8 slot,
                                            MeleeHostPlayerState* out_state)
{
    if (out_state == NULL || slot >= GM_MAX_PLAYERS) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    const StaticPlayer* player = Player_GetPtrForSlot(slot);
    out_state->character = player->ckind;
    out_state->player_kind = player->pkind;
    out_state->stocks = player->stocks;
    return MELEE_HOST_OK;
}
