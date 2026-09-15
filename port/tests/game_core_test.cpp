#include "test.hpp"

#include <melee_host/match_rules.h>

extern "C" {
#include <melee/lb/lbtime.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/random.h>

/* gm_1601.h pulls in the fighter and HSD headers, which C++ cannot include;
 * the CharacterKind parameter is an int-sized enum. */
float gm_80168B34(int ckind, int arg1, int arg2);
}

TEST_CASE("original HSD random generator runs natively")
{
    *HSD_RandSeedPtr = 1;
    REQUIRE(HSD_Rand() == 41);
    REQUIRE(HSD_Rand() == 51235);
    REQUIRE(HSD_Rand() == 6334);
    REQUIRE(HSD_Rand() == 59268);
    REQUIRE(HSD_Rand() == 51937);
}

TEST_CASE("original HSD random float stays in its expected range")
{
    *HSD_RandSeedPtr = 1;
    const float value = HSD_Randf();
    REQUIRE(value == 41.0F / 65536.0F);
    REQUIRE(value >= 0.0F);
    REQUIRE(value < 1.0F);
}

TEST_CASE("character icon frames follow the console's register reuse")
{
    /* The frames the console's gm_80168B34 returns.  Its C assigns base only
     * for Zelda, Sheik, Popo and the characters after Sheik; MWCC keeps base
     * in r3 with ckind, so the rest return ckind.  The HUD, the stock icons
     * and the results screen's portraits pick their image by this frame. */
    constexpr int kCaptain = 0x00;
    constexpr int kFox = 0x02;
    constexpr int kZelda = 0x12;
    constexpr int kSeak = 0x13;
    constexpr int kFalco = 0x14;
    constexpr int kGanon = 0x19;
    constexpr int kMasterHand = 0x1A;
    constexpr int kBoy = 0x1B;
    constexpr int kGigaBowser = 0x1D;
    constexpr int kCrazyHand = 0x1E;
    constexpr int kSandbag = 0x1F;
    constexpr int kPopo = 0x20;
    REQUIRE(gm_80168B34(kCaptain, 0, 0) == 0.0F);
    REQUIRE(gm_80168B34(kFox, 0, 0) == 2.0F);
    REQUIRE(gm_80168B34(kFox, 3, 1) == 32.0F);
    REQUIRE(gm_80168B34(kZelda, 0, 0) == 18.0F);
    REQUIRE(gm_80168B34(kSeak, 7, 2) == 85.0F);
    REQUIRE(gm_80168B34(kFalco, 0, 1) == 49.0F);
    REQUIRE(gm_80168B34(kGanon, 0, 0) == 24.0F);
    REQUIRE(gm_80168B34(kPopo, 0, 1) == 44.0F);
    REQUIRE(gm_80168B34(kMasterHand, 0, 1) == 28.0F);
    REQUIRE(gm_80168B34(kBoy, 0, 0) == 26.0F);
    REQUIRE(gm_80168B34(kGigaBowser, 0, 0) == 58.0F);
    REQUIRE(gm_80168B34(kCrazyHand, 0, 0) == 27.0F);
    REQUIRE(gm_80168B34(kSandbag, 0, 0) == 59.0F);
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

TEST_CASE("original VS rules store is reachable on the native host")
{
    MeleeHostMatchRules original{};
    REQUIRE(melee_host_match_rules_get(&original) == MELEE_HOST_OK);

    const MeleeHostMatchRules configured{
        .mode = 0,
        .time_limit = 8,
        .stock_count = 4,
        .handicap = 9,
        .damage_ratio = 10,
        .friendly_fire = true,
        .pause = true,
    };
    REQUIRE(melee_host_match_rules_set(&configured) == MELEE_HOST_OK);

    MeleeHostMatchRules observed{};
    REQUIRE(melee_host_match_rules_get(&observed) == MELEE_HOST_OK);
    REQUIRE(observed.mode == configured.mode);
    REQUIRE(observed.time_limit == configured.time_limit);
    REQUIRE(observed.stock_count == configured.stock_count);
    REQUIRE(observed.handicap == configured.handicap);
    REQUIRE(observed.damage_ratio == configured.damage_ratio);
    REQUIRE(observed.friendly_fire == configured.friendly_fire);
    REQUIRE(observed.pause == configured.pause);
    REQUIRE(melee_host_match_rules_set(&original) == MELEE_HOST_OK);
}

TEST_CASE("original VS default rules initialize through the host facade")
{
    REQUIRE(melee_host_match_rules_reset_defaults() == MELEE_HOST_OK);

    MeleeHostMatchRules rules{};
    REQUIRE(melee_host_match_rules_get(&rules) == MELEE_HOST_OK);
    REQUIRE(rules.mode == 0);
    REQUIRE(rules.time_limit == 2);
    REQUIRE(rules.stock_count == 3);
    REQUIRE(rules.handicap == 0);
    REQUIRE(rules.damage_ratio == 10);
    REQUIRE(!rules.friendly_fire);
    REQUIRE(rules.pause);
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
