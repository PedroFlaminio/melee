#ifndef MELEE_HOST_MATCH_RULES_H
#define MELEE_HOST_MATCH_RULES_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Host-safe representation of the fields used to configure a local VS match.
 * It is copied to/from the original GameRules storage; no native game struct
 * crosses this ABI boundary. */
typedef struct MeleeHostMatchRules {
    mh_u8 mode;
    mh_u8 time_limit;
    mh_u8 stock_count;
    mh_u8 handicap;
    mh_u8 damage_ratio;
    bool friendly_fire;
    bool pause;
} MeleeHostMatchRules;

MeleeHostStatus melee_host_match_rules_get(MeleeHostMatchRules* out_rules);
MeleeHostStatus melee_host_match_rules_set(
    const MeleeHostMatchRules* rules);
MeleeHostStatus melee_host_match_rules_reset_defaults(void);

#ifdef __cplusplus
}
#endif

#endif
