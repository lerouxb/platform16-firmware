#ifndef PLATFORM_METRO_H
#define PLATFORM_METRO_H

#include "utils.hpp"

namespace platform {

/**
 * Creates a clock signal at a specific frequency.
 * ported from daisysp
 */
class Metro {
  public:
  Metro() {}
  ~Metro() {}
  /** Initializes Metro module.
      Arguments:
      - freq: frequency at which new clock signals will be generated
          Input Range:
      - sample_rate: sample rate of audio engine
          Input range:
  */
  void init(float freqIn, float sampleRateIn) {
    freq = freqIn;
    phs = 0.0f;
    sampleRate = sampleRateIn;
    phsInc = (TWOPI_F * freq) / sampleRateIn;
  }

  /** checks current state of Metro object and updates state if necesary.
   */
  uint8_t process() {
    phs += phsInc;
    if(phs >= TWOPI_F)
    {
        phs -= TWOPI_F;
        return 1;
    }
    return 0;
  }

  /** resets phase to 0
   */
  inline void reset() {
    phs = 0.0f;
  }
  /** Sets frequency at which Metro module will run at.
   */
  void setFreq(float freq) {
    freq = freq;
    phsInc = (TWOPI_F * freq) / sampleRate;
  }

  /** Returns current value for frequency.
   */
  inline float getFreq() {
    return freq;
  }

  private:
  float freq;
  float phs, sampleRate, phsInc;
};

}  // namespace platform

#endif  // PLATFORM_METRO_H