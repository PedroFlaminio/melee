#ifndef _DOLPHIN_CMATH_H_
#define _DOLPHIN_CMATH_H_

#if defined(MELEE_HOST)
#include <math.h>
#else
f32 powf(f32 x, f32 y);
f32 tanf(f32);
#endif

#endif // _DOLPHIN_CMATH_H_
