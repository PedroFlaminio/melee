#include "test.hpp"

#include <melee_host/dolphin_os.h>
#include <melee_host/dolphin_time.h>

#include <cstdint>

/* dolphin/os.h is not C++ clean, so the one alarm call the test needs is
 * declared here. */
extern "C" void OSInitAlarm(void);

namespace {

/* One 60 Hz frame, the period of the scene's pad alarm. */
constexpr OSTime kFrameTicks = static_cast<OSTime>(OS_TIMER_CLOCK / 60U);

} // namespace

TEST_CASE("the f32 to u16 fast cast saturates and drops the fraction")
{
    // gm_80166378 reads each player's xE through this cast; with no host body
    // the value was whatever the stack held.
    REQUIRE(melee_host_os_f32_to_u16(12.9F) == 12);
    REQUIRE(melee_host_os_f32_to_u16(0.5F) == 0);
    REQUIRE(melee_host_os_f32_to_u16(-3.0F) == 0);
    REQUIRE(melee_host_os_f32_to_u16(65535.0F) == 0xFFFF);
    REQUIRE(melee_host_os_f32_to_u16(70000.0F) == 0xFFFF);
    const float not_a_number = 0.0F / 0.0F;
    REQUIRE(melee_host_os_f32_to_u16(not_a_number) == 0);
}

TEST_CASE("a frozen OS clock moves only when the host advances it")
{
    melee_host_os_time_freeze();
    REQUIRE(melee_host_os_time_frozen() == TRUE);

    const OSTime frozen = OSGetTime();
    REQUIRE(OSGetTime() == frozen);
    REQUIRE(OSGetTick() ==
            static_cast<OSTick>(static_cast<std::uint64_t>(frozen) &
                                0xFFFFFFFFU));

    melee_host_os_time_advance(kFrameTicks);
    REQUIRE(OSGetTime() == frozen + kFrameTicks);

    // A step that is not positive leaves the clock where it is.
    melee_host_os_time_advance(0);
    melee_host_os_time_advance(-kFrameTicks);
    REQUIRE(OSGetTime() == frozen + kFrameTicks);

    // Freezing again keeps the frozen time instead of rereading the host.
    melee_host_os_time_freeze();
    REQUIRE(OSGetTime() == frozen + kFrameTicks);

    melee_host_os_time_thaw();
    REQUIRE(melee_host_os_time_frozen() == FALSE);
}

TEST_CASE("the OS clock freezes at a chosen calendar time")
{
    OSCalendarTime release{};
    release.year = 2001;
    release.mon = 11;
    release.mday = 3;
    const OSTime instant = OSCalendarTimeToTicks(&release);

    melee_host_os_time_freeze_at(instant);
    REQUIRE(melee_host_os_time_frozen() == TRUE);
    REQUIRE(OSGetTime() == instant);
    OSCalendarTime seen{};
    OSTicksToCalendarTime(OSGetTime(), &seen);
    REQUIRE(seen.year == 2001);
    REQUIRE(seen.mon == 11);
    REQUIRE(seen.mday == 3);
    REQUIRE(seen.hour == 0);
    REQUIRE(seen.min == 0);
    REQUIRE(seen.sec == 0);

    // An already frozen clock moves to the new time.
    melee_host_os_time_advance(kFrameTicks);
    melee_host_os_time_freeze_at(instant);
    REQUIRE(OSGetTime() == instant);

    melee_host_os_time_thaw();
    REQUIRE(melee_host_os_time_frozen() == FALSE);
}

TEST_CASE("the alarm queue has no next alarm when none is armed")
{
    OSInitAlarm();
    s64 fire = 123;
    REQUIRE(melee_host_os_next_alarm(&fire) == FALSE);
    REQUIRE(fire == 123);
    REQUIRE(melee_host_os_next_alarm(nullptr) == FALSE);
}
