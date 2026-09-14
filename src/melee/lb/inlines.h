#ifndef MELEE_LB_INLINES_H
#define MELEE_LB_INLINES_H

#include <melee/lb/lbcardgame.h>
#include <melee/lb/lbcardnew.h>

/// @todo Is a macro the best way?
#define SKIP_CMD(cmd, n)                                                      \
    do {                                                                      \
        int i;                                                                \
        for (i = 0; i < (n); i++) {                                           \
            ++(cmd)->u;                                                       \
        }                                                                     \
    } while (0);

#define NEXT_CMD(cmd)                                                         \
    do {                                                                      \
        ++(cmd)->u;                                                           \
    } while (0);

/* A byte or a half of the command stream at `u`, counted in stream order.  The
 * host converts each word to native order, which on a little-endian target
 * mirrors the bytes and the halves within the word. */
#ifdef MELEE_HOST
#define CMD_U8(u, i) ((u8*) u)[(i) ^ 3]
#define CMD_U16(u, i) ((u16*) u)[(i) ^ 1]
#define CMD_S16(u, i) ((s16*) u)[(i) ^ 1]
#else
#define CMD_U8(u, i) ((u8*) u)[i]
#define CMD_U16(u, i) ((u16*) u)[i]
#define CMD_S16(u, i) ((s16*) u)[i]
#endif

static inline void lbCardGame_SetupArchive(void)
{
    lbCardNew_AllocWorkArea();
    lbCardGame_LoadArchive(0);
    lbCardGame_UpdatePowerTime();
}

#endif
