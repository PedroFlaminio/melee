#include "test.hpp"

extern "C" {
#include <melee/lb/lbtime.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/random.h>
}

TEST_CASE("original HSD random generator runs natively")
{
    *seed_ptr = 1;
    REQUIRE(HSD_Rand() == 41);
    REQUIRE(HSD_Rand() == 51235);
    REQUIRE(HSD_Rand() == 6334);
    REQUIRE(HSD_Rand() == 59268);
    REQUIRE(HSD_Rand() == 51937);
}

TEST_CASE("original HSD random float stays in its expected range")
{
    *seed_ptr = 1;
    const float value = HSD_Randf();
    REQUIRE(value == 41.0F / 65536.0F);
    REQUIRE(value >= 0.0F);
    REQUIRE(value < 1.0F);
}

TEST_CASE("original bounded time arithmetic runs natively")
{
    REQUIRE(lbTime_8000AEC8(10, 20) == 30);
    REQUIRE(lbTime_8000AEC8(0xFFFFFFF0U, 0x20U) == 0xFFFFFFFFU);
    REQUIRE(lbTime_8000AEE4(10, -20) == 0);
    REQUIRE(lbTime_8000AEE4(30, -20) == 10);
    REQUIRE(lbTime_8000AF74(0x12U, 5) == 0x17U);
    REQUIRE(lbTime_8000AF74(0xFEU, 5) == 0xFFU);
}

TEST_CASE("GameCube calendar epoch is preserved by host OS time")
{
    OSCalendarTime calendar{};
    OSTicksToCalendarTime(0, &calendar);
    REQUIRE(calendar.year == 2000);
    REQUIRE(calendar.mon == 0);
    REQUIRE(calendar.mday == 1);
    REQUIRE(calendar.hour == 0);
    REQUIRE(calendar.min == 0);
    REQUIRE(calendar.sec == 0);
    REQUIRE(OSCalendarTimeToTicks(&calendar) == 0);
}

TEST_CASE("original vector gameplay helpers run natively")
{
    Vec3 vector{ 3.0F, 4.0F, 0.0F };
    REQUIRE(lbVector_Normalize(&vector) == 5.0F);
    REQUIRE(vector.x == 0.6F);
    REQUIRE(vector.y == 0.8F);
    REQUIRE(vector.z == 0.0F);

    Vec3 from{ 2.0F, 4.0F, 6.0F };
    Vec3 to{ 4.0F, 8.0F, 10.0F };
    Vec3 result{};
    lbVector_Lerp(&from, &to, &result, 0.5F);
    REQUIRE(result.x == 3.0F);
    REQUIRE(result.y == 6.0F);
    REQUIRE(result.z == 8.0F);
}
