#include "test.hpp"

#include <melee_host/host.h>
#include <melee_host/input.h>

TEST_CASE("input is committed at a deterministic simulation boundary")
{
    MeleeHostContext* context = nullptr;
    const MeleeHostConfig config{ .resource_root = nullptr, .headless = true };
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);

    const MeleeHostPadState submitted{
        .buttons = 0x0100,
        .stick_x = 42,
        .stick_y = -17,
        .c_stick_x = 0,
        .c_stick_y = 0,
        .trigger_left = 0,
        .trigger_right = 255,
        .connected = true,
    };
    REQUIRE(melee_host_submit_pad_state(context, 0, &submitted) == MELEE_HOST_OK);
    REQUIRE(melee_host_submit_pad_state(context, MELEE_HOST_MAX_CONTROLLERS,
                                        &submitted) == MELEE_HOST_INVALID_ARGUMENT);

    MeleeHostInputSnapshot snapshot{};
    REQUIRE(melee_host_input_snapshot(context, &snapshot) == MELEE_HOST_OK);
    REQUIRE(snapshot.tick == 0);
    REQUIRE(snapshot.pads[0].connected == false);

    REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);
    REQUIRE(melee_host_input_snapshot(context, &snapshot) == MELEE_HOST_OK);
    REQUIRE(snapshot.tick == 1);
    REQUIRE(snapshot.pads[0].buttons == submitted.buttons);
    REQUIRE(snapshot.pads[0].stick_x == submitted.stick_x);
    REQUIRE(snapshot.pads[0].stick_y == submitted.stick_y);
    REQUIRE(snapshot.pads[0].trigger_right == submitted.trigger_right);
    REQUIRE(snapshot.pads[0].connected == true);
    melee_host_destroy(context);
}
