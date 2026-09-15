#ifndef MELEE_RENDER_PLAY_WINDOW_HPP
#define MELEE_RENDER_PLAY_WINDOW_HPP

#include <cstdint>
#include <cstdio>
#include <string>

/* The pieces of the play window that do not need SDL, so the unit tests reach
 * them. */
namespace melee::render {

inline constexpr const char* kPlayWindowName = "Melee PC";
inline constexpr const char* kPlayWindowControls =
    "Enter é START, WASD o analógico; Esc sai";

/* SDL reads a gamepad axis from -32768 to 32767 with right and down positive.
 * The GameCube stick, like the keyboard's WASD, has right and up positive, so
 * a vertical axis changes sign on the way to PADStatus. */
[[nodiscard]] constexpr std::int8_t gamepad_axis_x(std::int16_t value)
{
    return static_cast<std::int8_t>(static_cast<int>(value) / 258);
}

[[nodiscard]] constexpr std::int8_t gamepad_axis_y(std::int16_t value)
{
    return static_cast<std::int8_t>(-static_cast<int>(value) / 258);
}

/* Counts presented frames and gives their rate once per period of wall-clock
 * time.  The first frame only starts the count, so a rate covers the frame
 * intervals inside the period. */
class FrameRateMeter {
public:
    explicit constexpr FrameRateMeter(std::uint64_t period_nanoseconds)
        : period_nanoseconds_(period_nanoseconds)
    {
    }

    /* Returns true, with the rate in *frames_per_second, on the frame that
     * closes a period. */
    [[nodiscard]] bool add_frame(std::uint64_t now_nanoseconds,
                                 double* frames_per_second)
    {
        if (!started_ || now_nanoseconds < start_nanoseconds_) {
            started_ = true;
            start_nanoseconds_ = now_nanoseconds;
            frames_ = 0;
            return false;
        }
        ++frames_;
        const std::uint64_t elapsed = now_nanoseconds - start_nanoseconds_;
        if (elapsed < period_nanoseconds_) {
            return false;
        }
        *frames_per_second = static_cast<double>(frames_) * 1e9 /
                             static_cast<double>(elapsed);
        start_nanoseconds_ = now_nanoseconds;
        frames_ = 0;
        return true;
    }

private:
    std::uint64_t period_nanoseconds_;
    std::uint64_t start_nanoseconds_ = 0;
    std::uint64_t frames_ = 0;
    bool started_ = false;
};

[[nodiscard]] inline std::string play_window_title()
{
    return std::string(kPlayWindowName) + " — " + kPlayWindowControls;
}

[[nodiscard]] inline std::string play_window_title(double frames_per_second)
{
    char rate[32];
    std::snprintf(rate, sizeof(rate), "%.1f FPS", frames_per_second);
    return std::string(kPlayWindowName) + " — " + rate + " — " +
           kPlayWindowControls;
}

} // namespace melee::render

#endif
