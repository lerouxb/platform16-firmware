#ifndef PLATFORM_ATTACKDECAY_H
#define PLATFORM_ATTACKDECAY_H

#include <math.h>

namespace platform {




float safeAttackDecayTime(float time) {
  // 1/48000 (the sample rate) = 0.000020833333333333333
  // 1/24000 = 0.000041666666666666665
  // (this just deals with division by 0)
  return time == 0 ? 0.000041666666666666665 : time;
}

class AttackDecayEnvelope {
  public:
  AttackDecayEnvelope() = default;
  ~AttackDecayEnvelope() = default;

  void init(float sampleRateIn) {
    sampleRate = sampleRateIn;
    setTimeAndDirection(1.0);
    value = 0.0f;
  }

  void trigger() {
    value = 1.0f;
  }

  /*
  When mapping 0-1 to attack or decay, assuming the envelope is for volume:
  0 is very slow attack (so it starts quiet and very slowly gets louder)
  1 is very fast decay (so it starts loud and almost immediately gets quiet)
  0.51 is a slow decay (so loud immediately and then almost indefinitely)
  0.49 is a very fast attack (so almost immediately loud)
  */
  void setTimeAndDirection(float valueIn) {
    // https://www.musicdsp.org/en/latest/Synthesis/189-fast-exponential-envelope-generator.html

    // direction -1 is decay, 1 is attack
    // left of center is attack, right of center is decay
    direction = (valueIn >= 0.5f) ? -1 : 1;
    float value = (valueIn >= 0.5f) ? 1.f - ((valueIn - 0.5f) * 2.f) : 1.f - (valueIn * 2.f);
    // time is in seconds, 10 seconds max
    if (direction > 0) {
      // attack
      time = safeAttackDecayTime(powf(value, 6.f) * 20.0f);
    } else {
      // decay
      time = safeAttackDecayTime(powf(value, 2.f) * 20.0f);
    }

    /*
1/Math.pow(2, 16) = 0.0000152587890625
Math.log(0.0000152587890625) = -11.090354888959125
Math.log(0.0000152587890625*2) = -10.39720770839918
Math.log(1) = 0

so Math.log(end) - Math.log(start) is about -10.5ish

    */

    ///coeff = 1.0f + (logf(0.0001f)) /
    //      (time * sampleRate + 1);
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

#endif // PLATFORM_ATTACKDECAY_H