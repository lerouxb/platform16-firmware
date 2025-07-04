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
//using BPMParameter = IntegerRangeParameter<30, 240>;
using StepsParameter = IntegerRangeParameter<0, 31>;
//using DestinationParameter = IntegerRangeParameter<0, 2>;
using ScaleParameter = IntegerRangeParameter<0, 6>;

using AlgorithmParameter = IntegerRangeParameter<0, 12>;

using FrequencyParameter = ExponentialParameter<27.5f, 1000.f, 2.0f>;
using CutoffParameter = ExponentialParameter<5.f, HALF_SAMPLE_RATE, 3.f>;
//using AttackDecayParameter = ExponentialParameter<-10.f, 10.f, 3.f>;
//using AttackDecayParameter = FloatRangeParameter<-1.f, 1.f>;
using EvolveParameter = FloatRangeParameter<-1.f, 1.f>;


struct SDSState {
  BPMParameter bpm;
  RawParameter volume;
  RawParameter pitch;
  CutoffParameter cutoff;

  StepsParameter stepCount;
  RawParameter volumeEnvelope;
  RawParameter pitchEnvelope;
  RawParameter cutoffEnvelope;

  RawParameter skips;
  RawParameter volumeAmount;
  RawParameter pitchAmount;
  RawParameter cutoffAmount;

  AlgorithmParameter algorithm;
  //OverdriveParameter drive;
  EvolveParameter evolve;
  ScaleParameter scale;
  RawParameter resonance;

  uint step = 0;
  std::array<float, 32> steps;  
  // TODO: make separate arrays for each destination
  std::array<float, 32> amounts;  
  std::array<float, 32> amountsBackup;  

  SDSState()
    : bpm{120.f},
      volume{0},
      pitch{0},
      cutoff{16000},

      stepCount{0},
      volumeEnvelope{0},
      pitchEnvelope{0},
      cutoffEnvelope{0},

      algorithm{0},
      //drive{0.5},
      evolve{0},
      scale{0},
      resonance{0},

      skips{0},
      volumeAmount{0},
      pitchAmount{0},
      cutoffAmount{0},
      
      step{0} {
    for (size_t i = 0; i < 16; i++) {
      steps[i] = 0.f;
      amounts[i] = 0.f;
      amountsBackup[i] = 0.f;
    }
  }
};

}  // namespace platform

#endif  // PLATFORM_STEP_STATE_H