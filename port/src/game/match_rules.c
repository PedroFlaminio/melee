#include <melee_host/match_rules.h>

#include <melee/gm/gmmain_lib.h>

extern GameRules gmMainLib_DefaultGameRules;

MeleeHostStatus melee_host_match_rules_get(MeleeHostMatchRules* out_rules)
{
    GameRules* rules;

    if (out_rules == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    rules = gmMainLib_GetGameRules();
    if (rules == NULL) {
        return MELEE_HOST_NOT_READY;
    }
    out_rules->mode = rules->mode;
    out_rules->time_limit = rules->time_limit;
    out_rules->stock_count = rules->stock_count;
    out_rules->handicap = rules->handicap;
    out_rules->damage_ratio = rules->damage_ratio;
    out_rules->friendly_fire = rules->friendly_fire != 0;
    out_rules->pause = rules->pause != 0;
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_match_rules_set(const MeleeHostMatchRules* input)
{
    GameRules* rules;

    if (input == NULL) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    rules = gmMainLib_GetGameRules();
    if (rules == NULL) {
        return MELEE_HOST_NOT_READY;
    }
    rules->mode = input->mode;
    rules->time_limit = input->time_limit;
    rules->stock_count = input->stock_count;
    rules->handicap = input->handicap;
    rules->damage_ratio = input->damage_ratio;
    rules->friendly_fire = input->friendly_fire;
    rules->pause = input->pause;
    return MELEE_HOST_OK;
}

MeleeHostStatus melee_host_match_rules_reset_defaults(void)
{
    GameRules* rules = gmMainLib_GetGameRules();

    if (rules == NULL) {
        return MELEE_HOST_NOT_READY;
    }
    *rules = gmMainLib_DefaultGameRules;
    return MELEE_HOST_OK;
}
