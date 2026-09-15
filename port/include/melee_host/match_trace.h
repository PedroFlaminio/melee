#ifndef MELEE_HOST_MATCH_TRACE_H
#define MELEE_HOST_MATCH_TRACE_H

#include <melee_host/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A narrow C ABI for observing the live VS scene without exposing the
 * PowerPC-layout fighter headers to host C++ tools. */
bool melee_host_match_fighter_position(mh_u32 slot, mh_f32* x, mh_f32* y,
                                        mh_f32* z);
bool melee_host_match_fighter_motion(mh_u32 slot, mh_s32* motion);
/* The falls (KOs suffered) the game counts for a player slot; false for an
 * empty slot. */
bool melee_host_match_player_falls(mh_u32 slot, mh_s32* falls);
/* The VS rules the menus keep for the next match: the mode (0 time, 1
 * stock, 2 coin, 3 bonus), the time limit in minutes and the stock count. */
void melee_host_match_rules(mh_u32* mode, mh_u32* time_limit,
                            mh_u32* stock_count);
/* The match end the results screen reads: its outcome (a MatchOutcome), the
 * number of winners, the first winner's slot and the stocks left to the
 * first two slots.  False while no match end is set. */
bool melee_host_match_result(mh_u32* outcome, mh_u32* winners,
                             mh_u32* first_winner, mh_s32* stocks_p1,
                             mh_s32* stocks_p2);

#ifdef __cplusplus
}
#endif

#endif
