#include "test.hpp"

#include <melee_host/host.h>

#include <array>

namespace {

void record_task(void* user_data)
{
    auto* state = static_cast<std::pair<std::array<int, 3>*, int>*>(user_data);
    for (int& value : *state->first) {
        if (value == 0) {
            value = state->second;
            return;
        }
    }
}

void mark_cleanup(void* user_data)
{
    *static_cast<bool*>(user_data) = true;
}

} // namespace

TEST_CASE("host C ABI validates arguments")
{
    REQUIRE(melee_host_create(nullptr, nullptr) == MELEE_HOST_INVALID_ARGUMENT);
    REQUIRE(melee_host_step(nullptr) == MELEE_HOST_INVALID_ARGUMENT);
}

TEST_CASE("host scheduler is ordered by simulation tick and insertion")
{
    MeleeHostContext* context = nullptr;
    const MeleeHostConfig config{ .resource_root = nullptr, .headless = true };
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);

    std::array<int, 3> order{};
    std::pair state_one{ &order, 1 };
    std::pair state_two{ &order, 2 };
    std::pair state_three{ &order, 3 };
    REQUIRE(melee_host_schedule_task(context, 2, record_task, &state_one) ==
            MELEE_HOST_OK);
    REQUIRE(melee_host_schedule_task(context, 1, record_task, &state_two) ==
            MELEE_HOST_OK);
    REQUIRE(melee_host_schedule_task(context, 2, record_task, &state_three) ==
            MELEE_HOST_OK);

    REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);
    REQUIRE(melee_host_tick_count(context) == 1);
    const std::array<int, 3> after_first{ 2, 0, 0 };
    REQUIRE(order == after_first);
    REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);
    REQUIRE(melee_host_tick_count(context) == 2);
    const std::array<int, 3> after_second{ 2, 1, 3 };
    REQUIRE(order == after_second);
    melee_host_destroy(context);
}

TEST_CASE("host lifecycle and monotonic clock")
{
    MeleeHostContext* context = nullptr;
    const MeleeHostConfig config{ .resource_root = nullptr, .headless = true };
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);
    REQUIRE(context != nullptr);
    REQUIRE(melee_host_monotonic_nanoseconds() <=
            melee_host_monotonic_nanoseconds());
    REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);
    melee_host_destroy(context);
}

TEST_CASE("host cleans up tasks that are still pending during destruction")
{
    MeleeHostContext* context = nullptr;
    const MeleeHostConfig config{ .resource_root = nullptr, .headless = true };
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);
    bool cleaned = false;
    REQUIRE(melee_host_schedule_task_with_cleanup(
                context, 10, [](void*) {}, mark_cleanup, &cleaned) ==
            MELEE_HOST_OK);
    melee_host_destroy(context);
    REQUIRE(cleaned);
}
