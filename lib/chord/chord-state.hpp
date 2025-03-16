#ifndef PLATFORM_CHORD_STATE_H
#define PLATFORM_CHORD_STATE_H

#include "../parameters.hpp"
#include <algorithm>
#include <array>

namespace platform {

using FrequencyParameter = ExponentialParameter<50.f, 1000.f, 3.f>;
using LowFrequencyParameter = ExponentialParameter<0.f, 10.f, 3.f>;
using CutoffParameter = ExponentialParameter<5.f, HALF_SAMPLE_RATE, 0.3f>;

struct ChordState {
  RawParameter lfoShape;
  LowFrequencyParameter lfoRate;
  RawParameter lfoLevel;
  RawParameter volume;

  RawParameter oscillatorShape;
  CutoffParameter filterCutoff;
  RawParameter filterResonance;
  // TODO: filter type (lowpass, allpass, highpass) (or distortion rather?)

  std::array<RawParameter, 4> volumes;
  std::array<FrequencyParameter, 4> frequencies;

  ChordState()
    : lfoShape{0},
      lfoRate{0},
      lfoLevel{0},
      volume{0.25},
      oscillatorShape{0},
      filterCutoff{16000},
      filterResonance{0} {
    for (size_t i = 0; i < 4; i++) {
      volumes[i].setScaled(0.f);
      frequencies[i].setScaled(0.f);
    }
  }
};

}  // namespace platform

#endif  // PLATFORM_CHORD_STATE_H