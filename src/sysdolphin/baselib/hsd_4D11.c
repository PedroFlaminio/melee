/**
 * Card work area .bss/.sbss used by the card functions in hsd_3A94.c
 * (and the JPEG decoder in hsd_3B34.c). Kept in its own TU: the card
 * functions only match when this data is referenced as extern, so it
 * cannot be defined alongside them.
 */

#include <Runtime/platform.h>

#ifdef MELEE_HOST
#include <sysdolphin/baselib/hsd_3A94.h>

/* hsd_3A94.c reads 0x804D1138 through 0x804D2648 as one CardContext: the three
 * objects below, in this order, with nothing between them.  A host compiler
 * places globals independently, so the host keeps them in one object at the
 * DOL offsets, and hsd_3A94.h names the parts. */
/* 4D1138 */ struct HSD_HostCardBlock hsd_HostCardBlock_804D1138;
_Static_assert(offsetof(struct HSD_HostCardBlock, x1148) == 0x10,
               "hsd_804D1148 must stay at its DOL offset in the block");
_Static_assert(offsetof(struct HSD_HostCardBlock, x2348) == 0x1210,
               "hsd_804D2348 must stay at its DOL offset in the block");
#else
/* 4D2348 */ u8 hsd_804D2348[0x300];
/* 4D1148 */ u32 hsd_804D1148[0x80][0x9];
/* 4D1138 */ u8 hsd_804D1138[0x10];
#endif

/* 4D799C */ s32 hsd_804D799C;
/* 4D7998 */ s32 hsd_804D7998;
/* 4D7994 */ s32 hsd_804D7994;
/* 4D7990 */ s32 hsd_804D7990;
/* 4D798C */ s32 hsd_804D798C;
/* 4D7988 */ s32 hsd_804D7988;
/* 4D7984 */ volatile s32 hsd_804D7984;
/* 4D7980 */ volatile s32 hsd_804D7980;
