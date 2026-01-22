#ifndef PLATFORM_ATTACKORDECAY_H
#define PLATFORM_ATTACKODECAY_H

#include <math.h>

namespace platform {




float safeAttackDecayTime(float time) {
  // 1/48000 (the sample rate) = 0.000020833333333333333
  // 1/24000 = 0.000041666666666666665
  // (this just deals with division by 0)
  return time == 0 ? 0.000041666666666666665 : time;
}

class AttackOrDecayEnvelope {
  public:
  AttackOrDecayEnvelope() = default;
  ~AttackOrDecayEnvelope() = default;

  void init(float sampleRateIn) {
    sampleRate = sampleRateIn;
    setTimeAndDirection(1.0);
    value = 0.0f;
  }

  void trigger() {
    value = 1.0f;
  }

  void setTimeAndDirection(float valueIn) {
    // https://www.musicdsp.org/en/latest/Synthesis/189-fast-exponential-envelope-generator.html

    // direction -1 is decay, 1 is attack
    // left of center is attack, right of center is decay
    direction = (valueIn >= 0.f) ? -1 : 1;
    float value = valueIn >= 0.f ? 1.f - valueIn : fabs(valueIn);
    // time is in seconds, 10 seconds max
    if (direction > 0.f) {
      // attack
      time = safeAttackDecayTime(powf(value, 6.f) * 20.0f);
    } else {
      // decay
      time = safeAttackDecayTime(powf(value, 2.f) * 20.0f);
    }

    coeff = 1.f + (-10.5f / (time * sampleRate + 1.f));
  }

  float process() {
    value = fmaxf(fminf(1.f, coeff*value), 0.f);
    return direction > 0 ? 1.f - value : value;
  }

  private:
  float sampleRate;
  float time;
  float coeff;
  float value;
  int direction;
};

} // namespace platform

#endif // PLATFORM_ATTACKORDECAY_H