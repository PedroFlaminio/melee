/* Game functions the host links against but does not run yet.
 *
 * Each one is reachable from code that is ported, so the link needs it, but
 * the path that would call it is not part of what the host executes.  Rather
 * than return something plausible, each stops with its own name, so the day
 * the path is taken the report says which module has to come next.
 */

#include <melee/ef/efasync.h>
#include <melee/ft/ftdata.h>
#include <melee/gm/gm_16F1.h>
#include <melee/gr/grdatfiles.h>
#include <sysdolphin/baselib/hsd_3982.h>
#include <sysdolphin/baselib/leak.h>

#include <dolphin/os.h>
#include <dolphin/os/OSThread.h>

#define MELEE_HOST_UNPORTED(name)                                             \
    OSPanic(__FILE__, __LINE__, "%s is not ported to the host yet", name)

/* ftdata.c registers a fighter's files with the preload cache.  Its tables
 * name every character's own data, so it comes in with the fighters.
 * lbDvd_80017960 only reaches these when a scene keeps all its heaps, which
 * the title screen does not. */
void ftData_800855C8(FighterKind kind, u8 color)
{
    (void) kind;
    (void) color;
    MELEE_HOST_UNPORTED("ftData_800855C8");
}

void ftData_8008578C(int arg0, u8 color)
{
    (void) arg0;
    (void) color;
    MELEE_HOST_UNPORTED("ftData_8008578C");
}

/* lbDvd_GetPreloadedArchive hands a preloaded effect archive or stage archive
 * to these.  Only a scene that preloads effects or a stage creates such an
 * entry, and the title screen does neither. */
void efAsync_OnLoad(HSD_Archive* archive, u8* data, u32 length, int index)
{
    (void) archive;
    (void) data;
    (void) length;
    (void) index;
    MELEE_HOST_UNPORTED("efAsync_OnLoad");
}

void grDatFiles_801C5FC0(HSD_Archive* archive, void* data, size_t length)
{
    (void) archive;
    (void) data;
    (void) length;
    MELEE_HOST_UNPORTED("grDatFiles_801C5FC0");
}

/* lb_8001B14C lists the save files on a mounted memory card and keeps the ones
 * whose company and game code match the disc's.  The host never mounts a card,
 * and it does not read the disc header yet. */
struct DVDDiskID* DVDGetCurrentDiskID(void)
{
    MELEE_HOST_UNPORTED("DVDGetCurrentDiskID");
    return NULL;
}

/* gm_801A4D34 runs gm_801A4970, the debug pause and report handler, and checks
 * the thread list only at the debug levels, which the host does not select.
 * These are what the report and the check call. */
int HSD_Leak_80387DF8(int arg0)
{
    (void) arg0;
    MELEE_HOST_UNPORTED("HSD_Leak_80387DF8");
    return 0;
}

HSD_GObj* hsd_80398310(u16 arg0, u8 arg1, u8 arg2, u32 arg3)
{
    (void) arg0;
    (void) arg1;
    (void) arg2;
    (void) arg3;
    MELEE_HOST_UNPORTED("hsd_80398310");
    return NULL;
}

long OSCheckActiveThreads(void)
{
    MELEE_HOST_UNPORTED("OSCheckActiveThreads");
    return 0;
}
