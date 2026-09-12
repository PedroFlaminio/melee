#include "test.hpp"

#include <melee_host/baselib.h>

extern "C" {
#include <sysdolphin/baselib/aobj.h>
}

TEST_CASE("native HSD AObj owns and releases its FObj channels")
{
    REQUIRE(melee_host_baselib_bootstrap() == MELEE_HOST_OK);

    u8 animation_data[] = { 0x01, 12, 1 };
    HSD_FObjDesc channel_desc{ nullptr,
                               sizeof(animation_data),
                               0.0F,
                               3,
                               HSD_A_FRAC_U8,
                               HSD_A_FRAC_U8,
                               0,
                               animation_data };
    HSD_AObjDesc animation_desc{ AOBJ_LOOP, 30.0F, &channel_desc, 0 };

    HSD_AObj* animation = HSD_AObjLoadDesc(&animation_desc);
    REQUIRE(animation != nullptr);
    REQUIRE(animation->fobj != nullptr);
    REQUIRE((animation->flags & AOBJ_LOOP) != 0);
    REQUIRE((animation->flags & AOBJ_NO_ANIM) != 0);
    REQUIRE(animation->end_frame == 30.0F);
    REQUIRE(animation->framerate == 1.0F);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_AObjGetAllocData()) == 1);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_FObjGetAllocData()) == 1);

    HSD_AObjReqAnim(animation, 5.0F);
    REQUIRE(animation->curr_frame == 5.0F);
    REQUIRE((animation->flags & AOBJ_NO_ANIM) == 0);
    REQUIRE((animation->flags & AOBJ_FIRST_PLAY) != 0);

    HSD_AObjRemove(animation);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_AObjGetAllocData()) == 0);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_FObjGetAllocData()) == 0);
}
