#include <melee_host/baselib.h>

#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/list.h>
#include <sysdolphin/baselib/mtx.h>

MeleeHostStatus melee_host_baselib_bootstrap(void)
{
    static int initialized;

    if (initialized) {
        return MELEE_HOST_OK;
    }

    HSD_ListInitAllocData();
    HSD_VecInitAllocData();
    HSD_MtxInitAllocData();
    HSD_IDInitAllocData();
    HSD_FObjInitAllocData();
    HSD_AObjInitAllocData();
    HSD_AObjInitEndCallBack();
    HSD_IDSetup();
    initialized = 1;
    return MELEE_HOST_OK;
}
