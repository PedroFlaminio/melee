#include <melee_host/dolphin_time.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>

namespace {

constexpr std::int64_t kTicksPerSecond = 40'500'000;
constexpr auto kGameCubeEpoch =
    std::chrono::sys_days{ std::chrono::year{ 2000 } / std::chrono::January / 1 };

/* Game code reads the clock from one thread; the atomics keep a host thread
 * that freezes or advances it from racing that reader. */
std::atomic<bool> time_frozen{ false };
std::atomic<OSTime> frozen_time{ 0 };

OSTime duration_to_ticks(std::chrono::nanoseconds duration)
{
    const auto seconds =
        std::chrono::duration_cast<std::chrono::seconds>(duration);
    const auto remainder = duration - seconds;
    return static_cast<OSTime>(seconds.count() * kTicksPerSecond +
                               remainder.count() * kTicksPerSecond /
                                   1'000'000'000);
}

OSTime host_time()
{
    const auto now = std::chrono::system_clock::now();
    return duration_to_ticks(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - kGameCubeEpoch));
}

OSTick low_word(OSTime ticks)
{
    return static_cast<OSTick>(static_cast<std::uint64_t>(ticks) & 0xFFFFFFFFU);
}

} // namespace

extern "C" OSTime OSGetTime(void)
{
    if (time_frozen.load()) {
        return frozen_time.load();
    }
    return host_time();
}

/* Frozen, the tick is the low word of the frozen time, so both readings move
 * together. */
extern "C" OSTick OSGetTick(void)
{
    if (time_frozen.load()) {
        return low_word(frozen_time.load());
    }
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return low_word(duration_to_ticks(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now)));
}

extern "C" void melee_host_os_time_freeze(void)
{
    if (!time_frozen.load()) {
        frozen_time.store(host_time());
        time_frozen.store(true);
    }
}

extern "C" void melee_host_os_time_thaw(void)
{
    time_frozen.store(false);
}

extern "C" BOOL melee_host_os_time_frozen(void)
{
    return time_frozen.load() ? TRUE : FALSE;
}

extern "C" void melee_host_os_time_advance(s64 ticks)
{
    if (time_frozen.load() && ticks > 0) {
        frozen_time.fetch_add(ticks);
    }
}

extern "C" void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime* output)
{
    if (output == nullptr) {
        return;
    }

    std::int64_t seconds = ticks / kTicksPerSecond;
    std::int64_t subsecond_ticks = ticks % kTicksPerSecond;
    if (subsecond_ticks < 0) {
        subsecond_ticks += kTicksPerSecond;
        --seconds;
    }

    const auto time = kGameCubeEpoch + std::chrono::seconds{ seconds };
    const auto day = std::chrono::floor<std::chrono::days>(time);
    const std::chrono::year_month_day date{ day };
    const std::chrono::hh_mm_ss time_of_day{ time - day };
    const auto start_of_year =
        std::chrono::sys_days{ date.year() / std::chrono::January / 1 };

    output->sec = static_cast<int>(time_of_day.seconds().count());
    output->min = static_cast<int>(time_of_day.minutes().count());
    output->hour = static_cast<int>(time_of_day.hours().count());
    output->mday = static_cast<int>(static_cast<unsigned>(date.day()));
    output->mon = static_cast<int>(static_cast<unsigned>(date.month())) - 1;
    output->year = static_cast<int>(date.year());
    output->wday = static_cast<int>(std::chrono::weekday{ day }.c_encoding());
    output->yday = static_cast<int>((day - start_of_year).count());

    const std::int64_t microseconds =
        subsecond_ticks * 1'000'000 / kTicksPerSecond;
    output->msec = static_cast<int>(microseconds / 1000);
    output->usec = static_cast<int>(microseconds % 1000);
}

extern "C" OSTime OSCalendarTimeToTicks(OSCalendarTime* input)
{
    if (input == nullptr) {
        return 0;
    }

    const auto first_of_year =
        std::chrono::year{ input->year } / std::chrono::January / 1;
    const auto first_of_month =
        std::chrono::year_month{ first_of_year.year(), std::chrono::January } +
        std::chrono::months{ input->mon };
    const auto day = std::chrono::sys_days{ first_of_month / 1 } +
                     std::chrono::days{ input->mday - 1 };
    const auto time = day + std::chrono::hours{ input->hour } +
                      std::chrono::minutes{ input->min } +
                      std::chrono::seconds{ input->sec } +
                      std::chrono::milliseconds{ input->msec } +
                      std::chrono::microseconds{ input->usec };
    return duration_to_ticks(
        std::chrono::duration_cast<std::chrono::nanoseconds>(time - kGameCubeEpoch));
}
