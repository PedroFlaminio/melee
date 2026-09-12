#include "test.hpp"

#include "hsd_include.hpp"

#include <cstdint>

MELEE_HOST_TEST_HSD_BEGIN
#include <dolphin/ai.h>
#include <sysdolphin/baselib/synth.h>
MELEE_HOST_TEST_HSD_END

TEST_CASE("native synth audio allocation stays aligned before audio init")
{
    void* const allocation = HSD_AudioMalloc(73);
    REQUIRE(allocation != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(allocation) % 32U == 0U);
    HSD_AudioFree(allocation);
}

TEST_CASE("native synth initialization configures the deterministic audio facade")
{
    HSD_SynthInit(0, 0, 0, 0x1000);

    REQUIRE(AIGetDSPSampleRate() == AI_SAMPLERATE_32KHZ);
    REQUIRE(AIGetStreamVolLeft() == 0xFFU);
    REQUIRE(AIGetStreamVolRight() == 0xFFU);
}
