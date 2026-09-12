/* Host stand-ins for the HSD graphics-object layer referenced by the GObj
 * runtime.
 *
 * `gobj.c` links four object kinds into the library: camera, light, joint and
 * fog.  Their render and destroy callbacks live in `cobj.c`, `lobj.c`,
 * `jobj.c` and `fog.c`, which still carry GX and matrix code that has not been
 * ported.  The GObj scheduler itself never calls them: a GObj only reaches one
 * of these entry points after `HSD_GObjObject_80390A70` attaches a graphics
 * object to it.
 *
 * Each stub therefore aborts instead of returning a neutral value.  A silent
 * no-op here would let an unported code path look like it succeeded, which is
 * exactly the failure mode the port is trying to avoid. */

#include <Runtime/platform.h>

#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fog.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>

#include <stdio.h>
#include <stdlib.h>

ATTRIBUTE_NORETURN static void unported(const char* symbol)
{
    fprintf(stderr,
            "melee_host: %s belongs to the HSD graphics-object layer, which "
            "is not ported yet\n",
            symbol);
    abort();
}

void HSD_LObjRemoveAll(HSD_LObj* lobj)
{
    (void) lobj;
    unported("HSD_LObjRemoveAll");
}

void HSD_LObj_803668EC(HSD_LObj* lobj)
{
    (void) lobj;
    unported("HSD_LObj_803668EC");
}

void HSD_LObjSetupInit(HSD_CObj* cobj)
{
    (void) cobj;
    unported("HSD_LObjSetupInit");
}

void HSD_JObjRemoveAll(HSD_JObj* jobj)
{
    (void) jobj;
    unported("HSD_JObjRemoveAll");
}

void HSD_JObjDispAll(HSD_JObj* jobj, Mtx vmtx, u32 flags, u32 rendermode)
{
    (void) jobj;
    (void) vmtx;
    (void) flags;
    (void) rendermode;
    unported("HSD_JObjDispAll");
}

void HSD_FogSet(HSD_Fog* fog)
{
    (void) fog;
    unported("HSD_FogSet");
}

bool HSD_CObjSetCurrent(HSD_CObj* cobj)
{
    (void) cobj;
    unported("HSD_CObjSetCurrent");
}

void HSD_CObjEndCurrent(void)
{
    unported("HSD_CObjEndCurrent");
}

HSD_CObj* HSD_CObjGetCurrent(void)
{
    unported("HSD_CObjGetCurrent");
}
