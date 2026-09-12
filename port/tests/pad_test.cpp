#include "test.hpp"

#include <dolphin/pad.h>
#include <melee_host/host.h>
#include <melee_host/input.h>

TEST_CASE("PADRead adapts the active host input snapshot")
{
    MeleeHostContext* context = nullptr;
    const MeleeHostConfig config{ .resource_root = nullptr, .headless = true };
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);
    REQUIRE(melee_host_activate_pad_backend(context) == MELEE_HOST_OK);
    REQUIRE(PADInit() == TRUE);

    const MeleeHostPadState pad{ .buttons = PAD_BUTTON_A,
                                 .stick_x = -70,
                                 .stick_y = 30,
                                 .c_stick_x = 12,
                                 .c_stick_y = -9,
                                 .trigger_left = 45,
                                 .trigger_right = 90,
                                 .connected = true };
    REQUIRE(melee_host_submit_pad_state(context, 2, &pad) == MELEE_HOST_OK);
    REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);

    PADStatus status[PAD_MAX_CONTROLLERS]{};
    const u32 unavailable = PADRead(status);
    REQUIRE(unavailable == (PAD_CHAN0_BIT | PAD_CHAN1_BIT | PAD_CHAN3_BIT));
    REQUIRE(status[2].err == PAD_ERR_NONE);
    REQUIRE(status[2].button == PAD_BUTTON_A);
    REQUIRE(status[2].stickX == -70);
    REQUIRE(status[2].substickY == -9);
    REQUIRE(status[2].triggerLeft == 45);
    REQUIRE(status[0].err == PAD_ERR_NO_CONTROLLER);
    melee_host_destroy(context);
}

TEST_CASE("PADClamp uses the original GameCube normalization rules")
{
    PADStatus status[PAD_MAX_CONTROLLERS]{};
    status[0].err = PAD_ERR_NONE;
    status[0].stickX = 127;
    status[0].stickY = 0;
    status[0].substickX = 127;
    status[0].substickY = 0;
    status[0].triggerLeft = 45;
    status[0].triggerRight = 200;

    status[1].err = PAD_ERR_NO_CONTROLLER;
    status[1].stickX = 127;
    status[1].triggerLeft = 45;
    PADClamp(status);

    REQUIRE(status[0].stickX == 72);
    REQUIRE(status[0].stickY == 0);
    REQUIRE(status[0].substickX == 59);
    REQUIRE(status[0].substickY == 0);
    REQUIRE(status[0].triggerLeft == 15);
    REQUIRE(status[0].triggerRight == 150);
    REQUIRE(status[1].stickX == 127);
    REQUIRE(status[1].triggerLeft == 45);
}
