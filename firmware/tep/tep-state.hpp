#ifndef PLATFORM_TEP_STATE_H
#define PLATFORM_TEP_STATE_H

#include "../../lib/parameters.hpp"
#include <algorithm>
#include <array>

namespace platform {

using BPMParameter = ExponentialParameter<0.f, 240.f, 1.5f>;
//using RhythmParameter = IntegerRangeParameter<0, 80>; 
using RhythmParameter = IntegerRangeParameter<0, 22>; 

struct TEPState {
  BPMParameter bpm;
  IntegerRangeParameter<0, 6> octave;
  RawParameter glide;
  RawParameter resonance;

  RawParameter volume;
  RawParameter volumeAccent;
  RhythmParameter volumeRhythm;
  
  RawParameter cutoff;
  RawParameter cutoffAccent;
  RhythmParameter cutoffRhythm;

  RawParameter degree;
  IntegerRangeParameter<0, 9> arpeggioMode;
  RhythmParameter degreeRhythm;

  ExponentialParameter<0.f, 10.f, 2.f> detune;
  RawParameter rotate;
  RawParameter distortion;

  TEPState(): bpm{},
              octave{},
              glide{},
              resonance{},
              volume{},
              volumeAccent{},
              volumeRhythm{},
              cutoff{},
              cutoffAccent{},
              cutoffRhythm{},
              degree{},
              arpeggioMode{},
              degreeRhythm{},
              detune{},
              rotate{},
              distortion{}   {
  }
};

}  // namespace platform

#endif  // PLATFORM_TEP_STATE_H
