#include "test.hpp"

#include "render/play_window.hpp"

#include <cstdint>

TEST_CASE("the play window's gamepad axes keep the GameCube's up positive")
{
    using melee::render::gamepad_axis_x;
    using melee::render::gamepad_axis_y;
    REQUIRE(gamepad_axis_x(0) == 0);
    REQUIRE(gamepad_axis_x(32767) == 127);
    REQUIRE(gamepad_axis_x(-32768) == -127);
    /* SDL gives up as negative. */
    REQUIRE(gamepad_axis_y(-32768) == 127);
    REQUIRE(gamepad_axis_y(32767) == -127);
    REQUIRE(gamepad_axis_y(0) == 0);
    REQUIRE(gamepad_axis_y(-2580) == 10);
    REQUIRE(gamepad_axis_y(2580) == -10);
}

TEST_CASE("the play window's frame rate meter reports once per period")
{
    constexpr std::uint64_t kSecond = 1'000'000'000ULL;
    melee::render::FrameRateMeter meter(kSecond / 2);
    double rate = -1.0;
    /* 60 Hz for one second: the first frame starts the count, and the frames
     * at 0.5 s and 1 s close a period each. */
    int reports = 0;
    for (std::uint64_t frame = 0; frame <= 60; ++frame) {
        if (meter.add_frame(frame * kSecond / 60, &rate)) {
            ++reports;
            REQUIRE(rate > 59.99);
            REQUIRE(rate < 60.01);
        }
    }
    REQUIRE(reports == 2);

    /* Half the rate over the next period. */
    reports = 0;
    for (std::uint64_t frame = 1; frame <= 15; ++frame) {
        if (meter.add_frame(kSecond + frame * kSecond / 30, &rate)) {
            ++reports;
        }
    }
    REQUIRE(reports == 1);
    REQUIRE(rate > 29.99);
    REQUIRE(rate < 30.01);

    /* A clock that goes back starts the count again. */
    REQUIRE(!meter.add_frame(0, &rate));
    REQUIRE(!meter.add_frame(kSecond / 4, &rate));
    REQUIRE(meter.add_frame(kSecond / 2, &rate));
    REQUIRE(rate > 3.99);
    REQUIRE(rate < 4.01);
}

TEST_CASE("the play window's title carries the frame rate")
{
    REQUIRE(melee::render::play_window_title() ==
            "Melee PC — Enter é START, WASD o analógico; Esc sai");
    REQUIRE(melee::render::play_window_title(59.94) ==
            "Melee PC — 59.9 FPS — Enter é START, WASD o analógico; Esc sai");
}
