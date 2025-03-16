#ifndef PLATFORM_STEP_STATE_H
#define PLATFORM_STEP_STATE_H

#include "../parameters.hpp"
#include <algorithm>
#include <array>

namespace platform {

// TODO: FloatRangeParameter parameter
using BPMParameter = IntegerRangeParameter<30, 240>;
using ShapeParameter = IntegerRangeParameter<0, 2>;
using StepsParameter = IntegerRangeParameter<0, 32>;
using DestinationParameter = IntegerRangeParameter<0, 2>;

using FrequencyParameter = ExponentialParameter<50.f, 1000.f, 3.f>;
using CutoffParameter = ExponentialParameter<5.f, HALF_SAMPLE_RATE, 3.f>;
using DecayParameter = ExponentialParameter<0.f, 10.f, 2.5f>;

struct StepState {
  BPMParameter bpm;
  StepsParameter stepCount;
  RawParameter skips;
  RawParameter mods;

  ShapeParameter shape;
  RawParameter filterResonance;
  OverdriveParameter drive;
  RawParameter volume;

  FrequencyParameter frequency;
  CutoffParameter filterCutoff;
  RawParameter noise;
  DestinationParameter modDestination;
  
  DecayParameter pitchDecay;
  DecayParameter cutoffDecay;
  DecayParameter noiseDecay;
  RawParameter modAmount;

  uint step = 0;
  std::array<float, 16> steps;  
  std::array<float, 16> amounts;  

  StepState()
    : bpm{120.f},
      stepCount{0},
      skips{0},
      mods{0},

      shape{0.5},
      filterResonance{0},
      drive{0},
      volume{0},

      frequency{0},
      filterCutoff{16000},
      noise{0},
      modDestination{1},
      
      pitchDecay{0},
      cutoffDecay{0},
      noiseDecay{0},
      modAmount{0},
      
      step{0} {
    for (size_t i = 0; i < 16; i++) {
      steps[i] = 0.f;
      amounts[i] = 0.f;
    }
  }
};

}  // namespace platform

#endif  // PLATFORM_STEP_STATE_H