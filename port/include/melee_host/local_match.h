#ifndef MELEE_HOST_LOCAL_MATCH_H
#define MELEE_HOST_LOCAL_MATCH_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { MELEE_HOST_LOCAL_MATCH_MAX_PLAYERS = 6 };

/* Host-safe observation of the original StartMeleeData prepared for the VS
 * scene.  It deliberately contains no game pointers or PPC-layout structs. */
typedef struct MeleeHostPreparedMatch {
    mh_u8 match_kind;
    bool timer_enabled;
    mh_u32 time_limit_seconds;
    mh_u16 stage_kind;
    mh_u8 player_count;
    mh_s8 characters[MELEE_HOST_LOCAL_MATCH_MAX_PLAYERS];
    mh_u8 player_kinds[MELEE_HOST_LOCAL_MATCH_MAX_PLAYERS];
    mh_s8 stocks[MELEE_HOST_LOCAL_MATCH_MAX_PLAYERS];
} MeleeHostPreparedMatch;

typedef struct MeleeHostPlayerState {
    mh_s8 character;
    mh_u8 player_kind;
    mh_s8 stocks;
} MeleeHostPlayerState;

/* Creates a staging copy of the initial data consumed by the native VS scene.
 * Character kinds use Melee's original 0..25 playable-character IDs;
 * `stage_kind` accepts the original stage IDs 0..31. */
MeleeHostStatus melee_host_prepare_local_two_player_match(
    mh_s8 first_character, mh_s8 second_character, mh_u16 stage_kind);
MeleeHostStatus melee_host_prepared_match_get(MeleeHostPreparedMatch* out_match);

/* The same observation of the VS mode's own selection: what character select
 * and stage select wrote, and what the VS scene's StartMeleeData is built
 * from.  It lives in the game's saved data, so the boot memory setup must
 * have run. */
MeleeHostStatus melee_host_vs_selection_get(MeleeHostPreparedMatch* out_match);

/* Resets the original six-slot player state, including stale moves, attack
 * statistics and bonus state, then applies the prepared match's player data.
 * Fighter objects are deliberately not created here. */
MeleeHostStatus melee_host_initialize_prepared_player_state(void);
MeleeHostStatus melee_host_player_state_get(mh_u8 slot,
                                            MeleeHostPlayerState* out_state);

#ifdef __cplusplus
}
#endif

#endif
