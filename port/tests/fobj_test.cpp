#include "test.hpp"

#include <melee_host/baselib.h>

extern "C" {
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/spline.h>
}

#include <cmath>

namespace {

bool near(float actual, float expected)
{
    return std::fabs(actual - expected) < 0.0001F;
}

} // namespace

TEST_CASE("native HSD FObj loads and frees an animation channel chain")
{
    REQUIRE(melee_host_baselib_bootstrap() == MELEE_HOST_OK);

    u8 first_data[] = { 0x11, 0x00 };
    u8 second_data[] = { 0x11, 0x00 };
    HSD_FObjDesc second{ nullptr, sizeof(second_data), 4.0F, 8,
                         HSD_A_FRAC_U8, HSD_A_FRAC_U8, 0, second_data };
    HSD_FObjDesc first{ &second, sizeof(first_data), 2.0F, 7,
                        HSD_A_FRAC_U8, HSD_A_FRAC_U8, 0, first_data };

    HSD_FObj* channel = HSD_FObjLoadDesc(&first);
    REQUIRE(channel != nullptr);
    REQUIRE(channel->next != nullptr);
    REQUIRE(channel->startframe == 2);
    REQUIRE(channel->obj_type == 7);
    REQUIRE(channel->ad_head == first_data);
    REQUIRE(channel->next->startframe == 4);
    REQUIRE(channel->next->obj_type == 8);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_FObjGetAllocData()) == 2);

    HSD_FObjRemoveAll(channel);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_FObjGetAllocData()) == 0);
}

TEST_CASE("portable HSD Hermite evaluator preserves endpoints")
{
    REQUIRE(near(splGetHelmite(0.1F, 0.0F, 3.0F, 9.0F, 0.0F, 0.0F),
                 3.0F));
    REQUIRE(near(splGetHelmite(0.1F, 10.0F, 3.0F, 9.0F, 0.0F, 0.0F),
                 9.0F));
}
