#ifndef PLATFORM_DECAY_H
#define PLATFORM_DECAY_H

#include <math.h>

namespace platform {

class DecayEnvelope {
  public:
  DecayEnvelope() = default;
  ~DecayEnvelope() = default;

  void init(float sampleRateIn) {
    sampleRate = sampleRateIn;
    setDecayTime(1.0f);
    value = 0.0f;
  }

  void trigger() {
    value = 1.0f;
  }

  void setDecayTime(float decayTimeIn) {
    // decayTime is in seconds
    decayTime = decayTimeIn;
    decayRate = 1.0f - expf(-1.0f / (sampleRate * decayTime));
  }

  float process() {
    value = fmaxf(0.0f, value - decayRate);
    return value;
  }

  private:
  float sampleRate;
  float decayTime;
  float decayRate;
  float value;
};

} // namespace platform

#endif // PLATFORM_DECAY_H