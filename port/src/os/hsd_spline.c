#include <sysdolphin/baselib/spline.h>

/* Portable form of the scalar Hermite evaluator used by HSD_FObj. */
f32 splGetHelmite(f32 inverse_duration, f32 time, f32 start, f32 end,
                  f32 start_slope, f32 end_slope)
{
    const f32 time_squared = time * time;
    const f32 inverse_squared = inverse_duration * inverse_duration;
    const f32 t2_over_duration = time_squared * inverse_duration;
    const f32 t3_over_duration_squared =
        inverse_squared * (time_squared * time);
    const f32 twice_t3 =
        2.0F * t3_over_duration_squared * inverse_duration;
    const f32 three_t2 = 3.0F * time_squared * inverse_squared;

    return end_slope * (t3_over_duration_squared - t2_over_duration) +
           start_slope *
               (time + (t3_over_duration_squared - 2.0F * t2_over_duration)) +
           start * (1.0F + twice_t3 - three_t2) +
           end * (-twice_t3 + three_t2);
}
