#ifndef PLATFORM_STEP_INSTRUMENT_H
#define PLATFORM_STEP_INSTRUMENT_H

#include "../decay.hpp"
#include "../ladder.hpp"
#include "../metro.hpp"
#include "../oscillator.hpp"
#include "../pots.hpp"
#include "../utils.hpp"
#include "step-controller.hpp"
#include "step-state.hpp"

#include <math.h>

#define DESTINATION_NOISE 0
#define DESTINATION_PITCH 1
#define DESTINATION_CUTOFF 2

#define SHAPE_TRIANGLE 0
#define SHAPE_SAW 1
#define SHAPE_SQUARE 2

namespace platform {

float safeDecayTime(float decayTime) {
  // 1/48000 (the sample rate) = 0.000020833333333333333
  // 1/24000 = 0.000041666666666666665
  return decayTime == 0 ? 0.000041666666666666665 : decayTime;
}

float maybeDecay(float decay, float value) {
  return decay > 9.999 ? 1.f : value;
}


struct StepInstrument {
  StepInstrument(Pots& pots)
    : controller{pots}, changed{true}, cachedFrequency{0}, cachedCutoff{0} {};

  void init(float sampleRateIn) {
    sampleRate = sampleRateIn;
    oscillator.init(sampleRate);
    oscillator.setWaveform(getShape());
    clock.init(getTickFrequency(), sampleRate);

    pitchEnvelope.init(sampleRate);
    cutoffEnvelope.init(sampleRate);
    noiseEnvelope.init(sampleRate);

    filter.init(sampleRate);
    // filter.SetDrive(0.8);

    randomizeSequence();
  }

  void update() {
    controller.update(state);
  }

  float getTickFrequency() {
    // TODO: move into a custom BPM parameter so we don't unnecessarily keep
    // recalculating this
    return state.bpm.getScaled() / 60.f * 4.f;  // 16th notes, not quarter notes
  }

  bool isPlayedStep() {
    /*
    1. skips is a probability of 0 to 1
    2. each step is a random value between 0 and 1 which we only chanhe when
       stepCount goes to 0.
    3. so the higher skips value, the more likely a step will be skipped

    We only reset steps (and amounts) whenever stepCount is 0 (ie. the
    stepCount knob is all the way left) and therefore whenever either the
    stepCount and skips returns to the same position it will play the same
    rhythm. Until the stepCount goes back to 0 again.

    In other words you can switch up the rhythm by turning the skips knob, but
    you can return back to where you were so it is performable.

    And as long as you don't turn the stepCount knob all the way left, you can
    also shorten or lengthen the sequence, but still return back exactly where
    you were.

    Idea inspired by the brilliant Body Synths Metal Fetishist, but I'm not
    entirely certain the amounts and and skips work exactly the same.

    The only thing I'm unsure of at this point is wheter to quantize skips so
    that you can't end up in a spot where it drifts back and forth over a
    step's value. But that could also be a feature.
    */
    return state.steps[state.step] >= state.skips.getScaled();
  }

  bool isModStep() {
    /*
    Similar to isPlayedStep() above, except turning the mods knob to the right
    makes it more likely that a step is a mod step.
    */
    return state.amounts[state.step] >= (1.f - state.mods.getScaled());
  }

  uint getDestination() {
    //return (int)roundf(state.modDestination.getScaled());
    return state.modDestination.getScaled();
  }

  uint getShape() {
    auto shape = state.shape.getScaled();
    if (shape == SHAPE_TRIANGLE) {
      return Oscillator::WAVE_POLYBLEP_TRI;
    }
    if (shape == SHAPE_SAW) {
      return Oscillator::WAVE_POLYBLEP_SAW;
    }
    return Oscillator::WAVE_POLYBLEP_SQUARE;
  }

  float getNoiseAmount() {
    float value = state.noise.getScaled() * scaleBipolar(randomProb());
    if (getDestination() == DESTINATION_NOISE && isModStep()) {
      value += state.modAmount.getScaled() * scaleBipolar(randomProb());
    }
    return value;
  }

  float getOscillatorFrequency() {
    if (!changed) {
      return cachedFrequency;
    }

    float value = state.frequency.getScaled();
    if (getDestination() == DESTINATION_PITCH && isModStep()) {
      // scale up/down an octave, so from half of frequency to double frequency
      float factor = scaleBipolar(state.modAmount.getScaled());
      if (factor > 0) {
        value += value * factor;
      } else {
        value += value / 2 * factor;
      }
    }
    cachedFrequency = value;
    return value;
  }

  float getFilterCutoff() {
    if (!changed) {
      return cachedCutoff;
    }

    // TODO: we can cache this per step
    float value = state.filterCutoff.getScaled();

    if (getDestination() == DESTINATION_CUTOFF && isModStep()) {
      // TODO: not sure how far to go with this. maybe only a portion of the full range?
      value += (scaleBipolar(state.modAmount.getScaled()) * HALF_SAMPLE_RATE);
    }

    // TODO: have sample rate and half sample rate as constants somewhere
    value = fclamp(value, 5.f, HALF_SAMPLE_RATE);

    cachedCutoff = value;
    return value;
  }

  void randomizeSequence() {
    // randomize the whole sequence
    for (int i = 0; i < 16; i++) {
      state.steps[i] = randomProb();
      state.amounts[i] = randomProb();
    }
  }

  float process() {
    // TODO: 0, 2, 4, 8, 10, 16, 32?
    uint stepCount = state.stepCount.getScaled();

    if (stepCount != 0) {
      // always play the down beat, otherwise when you shorten stepCount a sequence might sound off
      state.steps[0] = 1.f;
    }

    float pitchDecay = state.pitchDecay.getScaled();
    float cutoffDecay = state.cutoffDecay.getScaled();
    float noiseDecay = state.noiseDecay.getScaled();

    if (clock.process()) {
      //printf("%.2f\n", sampleRate);
      // recalculate frequency and cutoff
      changed = true;

      // trigger notes, advance sequencer, etc
      if (stepCount == 0) {
        randomizeSequence();
      }

      if (isPlayedStep()) {
        pitchEnvelope.trigger();
        cutoffEnvelope.trigger();
        noiseEnvelope.trigger();
      }

      state.step++;

      if (state.step >= stepCount) {
        state.step = 0;
      }
    }

    oscillator.setWaveform(getShape());
    clock.setFreq(getTickFrequency());
    filter.setRes(state.filterResonance.getScaled() * 1.8f);

    pitchEnvelope.setDecayTime(safeDecayTime(pitchDecay));
    cutoffEnvelope.setDecayTime(safeDecayTime(cutoffDecay));
    noiseEnvelope.setDecayTime(safeDecayTime(noiseDecay));

    float frequency = getOscillatorFrequency() * maybeDecay(pitchDecay, pitchEnvelope.process());
    oscillator.setFreq(frequency);
    float cutoff = getFilterCutoff() * maybeDecay(cutoffDecay, cutoffEnvelope.process());
    filter.setFreq(fmax(5.f, cutoff));

    float sample = oscillator.process();

    // apply noise
    float noise = getNoiseAmount();
    sample += noise * maybeDecay(noiseDecay, noiseEnvelope.process());

    // apply the filter
    sample = filter.process(sample);

    // overdrive
    sample = softClip(sample * state.drive.getPreGain()) * state.drive.getPostGain();

    // volume
    // sample = sample * state.volume.getScaled();
    sample = sample * powf(state.volume.getScaled(), 2.f);

    // cache what we can until the next clock tick
    changed = true;

    return softClip(sample);
  }

  StepState* getState() {
    return &state;
  }

  private:
  float sampleRate;
  bool changed;
  float cachedFrequency;
  float cachedCutoff;
  StepState state;
  StepController controller;
  Metro clock;
  LadderFilter filter;
  // TODO: just use normal saw, ramp and triangle oscillators then interpolate between them
  Oscillator oscillator;
  DecayEnvelope pitchEnvelope;
  DecayEnvelope cutoffEnvelope;
  DecayEnvelope noiseEnvelope;
};

}  // namespace platform

#endif  // PLATFORM_STEP_INSTRUMENT_H