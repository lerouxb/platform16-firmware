#ifndef PLATFORM_PMD_STATE_H
#define PLATFORM_PMD_STATE_H

#include "../../lib/parameters.hpp"
#include <algorithm>
#include <array>

namespace platform {

using LFOParameter = ExponentialParameter<0.f, 50.f, 5.5f>;
using BPMParameter = ExponentialParameter<0.f, 240.f, 1.5f>;


struct PMDState {
  IntegerRangeParameter<1, 32> length;
  RawParameter density;
  IntegerRangeParameter<1, 32> complexity;
  RawParameter spread;
  RawParameter bias;

  BPMParameter bpm;

  RawParameter volume;

  RawParameter baseFreq; // key
  // TODO: exponential?
  RawParameter range;

  ExponentialParameter<0.f, 1.f, 3.2f> modulatorDepth;

  RawParameter decay;

  RawParameter scramble;

  LFOParameter tembreLFORate;
  ExponentialParameter<0.f, 1.f, 3.f> tembreLFODepth;

  // TODO: change envelope LFO to the bias LFO rather. Or replace these with an
  // LFO and resonance? Try some options :)
  // also consider a random "oscillator" so we only need one knob. Maybe add
  // something that will move the step spread around a bit?
  // swing?
  LFOParameter envelopeLFORate;
  ExponentialParameter<0.f, 1.f, 1.f> envelopeLFODepth;

  PMDState()
    : length{16},
      density{0.5f},
      complexity{16},
      spread{0.5f},
      bias{0.4f},
      bpm{120.f},
      volume{0.8f},
      baseFreq{0.5f},
      range{0.5f},
      modulatorDepth{0.5f},
      decay{0.5f},
      scramble{0.0f},
      tembreLFORate{0.5f},
      tembreLFODepth{0.0f},
      envelopeLFORate{0.5f},
      envelopeLFODepth{0.0f}
      {
  }
};

}  // namespace platform

#endif  // PLATFORM_PMD_STATE_H