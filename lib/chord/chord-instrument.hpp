#ifndef PLATFORM_CHORD_INSTRUMENT_H
#define PLATFORM_CHORD_INSTRUMENT_H

#include "../pots.hpp"
#include "../ladder.hpp"
#include "../variablesawosc.hpp"
#include "chord-state.hpp"
#include "chord-controller.hpp"

namespace platform {
struct ChordInstrument {
  ChordInstrument(Pots &pots) : controller{pots} {};

  void init(float sampleRate) {
    filter.init(sampleRate);
    //filter.SetDrive(0.8);
    lfo.init(sampleRate);
    lfo.setPW(0.f);
    for (int i=0; i<4; i++) {
      oscillators[i].init(sampleRate);
      oscillators[i].setPW(0.f);
    }
  }

  void update() {
    controller.update(state);
  }

  float process() {
    // TODO: only change values if they are significantly different

    auto lfoRateChanged = state.lfoRate.changed;
    float lfoRate = state.lfoRate.getScaled();
    if (lfoRateChanged) {
      lfo.setFreq(lfoRate);
    }
    if (state.lfoShape.changed) {
      lfo.setWaveshape(state.lfoShape.getScaled());
    }

    // TODO: the lfo filter amount should probably be logarithmic and better
    // thought through rather than linear and 0-1000 hardcoded

    // don't get the filter "stuck on" if the lfo is at zero
    // TODO: we probably need a "close to zero" check rather
    float lfoFilterAmount = lfoRate ? lfo.process()*state.lfoLevel.getScaled()*1000.f : 0;

    float filterFreq = std::max(std::min(state.filterCutoff.getScaled() + lfoFilterAmount, 16000.f), 0.f);
    filter.setFreq(filterFreq);
    if (state.filterResonance.changed) {
      filter.setRes(state.filterResonance.getScaled()*1.8f);
    }

    float sample = 0.f;
    for (int i=0; i<4; i++) {
      auto frequencyChanged = state.frequencies[i].changed;
      float frequency = state.frequencies[i].getScaled();
      if (frequencyChanged) {
        oscillators[i].setFreq(frequency);
      }
      if (state.oscillatorShape.changed) {
        oscillators[i].setWaveshape(state.oscillatorShape.getScaled());
      }
      // only add it if it is "on"
      // TODO: this probably needs a "is close to zero" check rather
      if (frequency) {
        float value = oscillators[i].process();
        sample += value * state.volumes[i].getScaled();
      }
    }

    sample = filter.process(sample);

    // hard clip it down to -1 to 1.0 after scaling it up
    //float distorted = filter.Low() * state.distortion.getScaled();
    //distorted = std::max(std::min(distorted, 1.0f), -1.0f);

    // TODO: volume should probably be smarter than linear?
    //return distorted * state.volume.getScaled();

    sample = sample * state.volume.getScaled();
    return sample;
  }

private:
  ChordState state;
  ChordController controller;
  LadderFilter filter;
  VariableSawOscillator lfo;
  VariableSawOscillator oscillators[4];
};

} // namespace platform

#endif // PLATFORM_CHORD_INSTRUMENT_H