/* Game functions the host links against but does not run yet.
 *
 * Each one is reachable from code that is ported, so the link needs it, but
 * the path that would call it is not part of what the host executes.  Rather
 * than return something plausible, each stops with its own name, so the day
 * the path is taken the report says which module has to come next.
 */

#include <melee/ef/efasync.h>
#include <melee/ft/ftdata.h>
#include <melee/gr/grdatfiles.h>

#include <dolphin/os.h>

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
