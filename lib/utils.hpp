
#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#include <math.h>

#ifndef M_PI
#define M_PI 3.1415927410125732421875f
#endif

#ifndef TWOPI_F
  #define TWOPI_F (2.0f * M_PI)
#endif


namespace platform {


/** quick fp clamp
 */
inline float fclamp(float in, float min, float max) {
  return fmin(fmax(in, min), max);
}

inline float scaleBipolar(float value) {
  return value * 2.f - 1.f;
}

inline float randomProb() {
  return ((float)rand()) / RAND_MAX;
}

/** Soft Limiting function ported extracted from pichenettes/stmlib via daisysp*/
inline float softLimit(float x) {
  return x * (27.f + x * x) / (27.f + 9.f * x * x);
}

/** Soft Clipping function extracted from pichenettes/stmlib via daisysp */
inline float softClip(float x) {
  if (x < -3.0f)
    return -1.0f;
  else if (x > 3.0f)
    return 1.0f;
  else
    return softLimit(x);
}

} // namespace platform

#endif
