#include "test.hpp"

#include <melee_host/host.h>
#include <melee_host/input.h>
#include <melee_host/menu_native.h>

extern "C" {
#include <dolphin/pad.h>
}

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

TEST_CASE("host menu input mirrors GameCube buttons and analog navigation")
{
    const MeleeHostPadState state{
        .buttons = static_cast<mh_u16>(PAD_BUTTON_A | PAD_BUTTON_X),
        .stick_x = 55,
        .stick_y = -55,
        .c_stick_x = 0,
        .c_stick_y = 0,
        .trigger_left = 64,
        .trigger_right = 0,
        .connected = true,
    };
    const mh_u16 input = melee_host_menu_input_from_pad(&state);
    REQUIRE((input & MELEE_HOST_MENU_CONFIRM) != 0);
    REQUIRE((input & MELEE_HOST_MENU_A) != 0);
    REQUIRE((input & MELEE_HOST_MENU_X) != 0);
    REQUIRE((input & MELEE_HOST_MENU_RIGHT) != 0);
    REQUIRE((input & MELEE_HOST_MENU_DOWN) != 0);
    REQUIRE((input & MELEE_HOST_MENU_L_TRIGGER) != 0);
    REQUIRE(melee_host_menu_input_from_pad(nullptr) == 0);
}

TEST_CASE("host menu filter edges actions and repeats held directions")
{
    MeleeHostMenuInputFilter filter{};
    melee_host_menu_input_filter_reset(&filter);
    const mh_u16 held = MELEE_HOST_MENU_RIGHT | MELEE_HOST_MENU_CONFIRM;
    REQUIRE(melee_host_menu_input_filter_update(&filter, held) == held);
    REQUIRE(melee_host_menu_input_filter_update(&filter, held) == 0);
    for (int frame = 0; frame < 13; ++frame) {
        REQUIRE(melee_host_menu_input_filter_update(&filter, held) == 0);
    }
    REQUIRE(melee_host_menu_input_filter_update(&filter, held) ==
            MELEE_HOST_MENU_RIGHT);
    REQUIRE(melee_host_menu_input_filter_update(&filter, held) == 0);
    REQUIRE(melee_host_menu_input_filter_update(&filter, held) == 0);
    REQUIRE(melee_host_menu_input_filter_update(&filter, held) == 0);
    REQUIRE(melee_host_menu_input_filter_update(&filter, held) ==
            MELEE_HOST_MENU_RIGHT);
}

TEST_CASE("native menu evaluator consumes the host PAD snapshot")
{
    MeleeHostContext* context = nullptr;
    const MeleeHostConfig config{ .resource_root = nullptr, .headless = true };
    REQUIRE(melee_host_create(&config, &context) == MELEE_HOST_OK);
    REQUIRE(melee_host_activate_pad_backend(context) == MELEE_HOST_OK);
    REQUIRE(melee_host_native_menu_init() == MELEE_HOST_OK);

    const MeleeHostPadState state{
        .buttons = PAD_BUTTON_A,
        .stick_x = 0,
        .stick_y = 0,
        .c_stick_x = 0,
        .c_stick_y = 0,
        .trigger_left = 0,
        .trigger_right = 0,
        .connected = true,
    };
    REQUIRE(melee_host_submit_pad_state(context, 0, &state) == MELEE_HOST_OK);
    REQUIRE(melee_host_step(context) == MELEE_HOST_NOT_READY);

    mh_u32 events = 0;
    REQUIRE(melee_host_native_menu_update(MELEE_HOST_MAX_CONTROLLERS, &events) ==
            MELEE_HOST_OK);
    REQUIRE((events & MELEE_HOST_NATIVE_MENU_CONFIRM) != 0);
    REQUIRE((events & MELEE_HOST_NATIVE_MENU_A) != 0);

    melee_host_destroy(context);
}
