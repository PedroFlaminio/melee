#include <melee_host/host.h>

#include <chrono>

extern "C" mh_u64 melee_host_monotonic_nanoseconds(void)
{
    using Clock = std::chrono::steady_clock;
    using Nanoseconds = std::chrono::nanoseconds;

    const auto elapsed = Clock::now().time_since_epoch();
    return static_cast<mh_u64>(
        std::chrono::duration_cast<Nanoseconds>(elapsed).count());
}

