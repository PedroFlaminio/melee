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

TEST_CASE("the alarm queue has no next alarm when none is armed")
{
    OSInitAlarm();
    s64 fire = 123;
    REQUIRE(melee_host_os_next_alarm(&fire) == FALSE);
    REQUIRE(fire == 123);
    REQUIRE(melee_host_os_next_alarm(nullptr) == FALSE);
}
