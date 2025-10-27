#ifndef PLATFORM_STEP_STATE_H
#define PLATFORM_STEP_STATE_H

#include "../../lib/parameters.hpp"
#include <algorithm>
#include <array>

namespace platform {

// TODO: it is unclear whether having all these parameters separate from just
// 0-1 really helps. In reality it is often clearer to just do the conversion in
// the instrument file because various combinations of them once you add
// "amounts" and "amount" dynamically ends up requiring the conversion again.

// Pocket Operators say they can do 240 BPM, but when clocked in SY4 it seems to
// only reliably handle up to about 200. Options are to lower this limit or to
// just be mindful when connecting a pocket operator to clock out
using BPMParameter = ExponentialParameter<0.f, 240.f, 1.5f>;
using StepsParameter = IntegerRangeParameter<0, 31>;
using ScaleParameter = IntegerRangeParameter<0, 6>;

using AlgorithmParameter = IntegerRangeParameter<0, 8>;

using FrequencyParameter = ExponentialParameter<27.5f, 1000.f, 2.0f>;

using NoiseParameter = DeadzoneExponentialParameter<0.f, 1.f, 0.1f, 0.05f>;


struct SDSState {
  BPMParameter bpm;
  RawParameter volume;
  RawParameter basePitch;
  BipolarParameter<0.05f> cutoff;
  //NoiseParameter noise;
  BipolarParameter<0.05f> pitchOffset;

  StepsParameter stepCount;
  BipolarParameter<0.05f> volumeEnvelope;
  BipolarParameter<0.05f> cutoffEnvelope;

  RawParameter skips;
  BipolarParameter<0.05f> pitchAmount;
  BipolarParameter<0.05f> cutoffAmount;

  AlgorithmParameter algorithm;
  RawParameter drive;
  BipolarParameter<0.05f> evolve;
  ScaleParameter scale;
  RawParameter resonance;

  uint step = 0;
  std::array<float, 32> steps;  
  // amounts are the amount of modulation for each step
  std::array<float, 32> pitchAmounts;
  std::array<float, 32> filterAmounts;
  //std::array<float, 32> volumeAmounts;
  // keep a backup so we can sort them by algorithm, yet go back to the original random order
  std::array<float, 32> pitchAmountsBackup;  

  SDSState()
    : bpm{120.f},
      volume{0},
      basePitch{0},
      cutoff{0},
      pitchOffset{0},

      stepCount{0},
      volumeEnvelope{0},
      cutoffEnvelope{0},

      algorithm{0},
      evolve{0},
      scale{0},
      resonance{0},

      skips{0},
      //volumeAmount{0},
      pitchAmount{0},
      cutoffAmount{0},
      
      step{0} {
    for (size_t i = 0; i < 16; i++) {
      steps[i] = 0.f;
      pitchAmounts[i] = 0.f;
      filterAmounts[i] = 0.f;
      //volumeAmounts[i] = 0.f;
      pitchAmountsBackup[i] = 0.f;
    }
  }
};

}  // namespace platform

#endif  // PLATFORM_STEP_STATE_H