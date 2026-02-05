#ifndef PLATFORM_TEP_INSTRUMENT_H
#define PLATFORM_TEP_INSTRUMENT_H

#include "../../lib/arpeggio.hpp"
#include "../../lib/attackordecay.hpp"
#include "../../lib/buttons.hpp"
#include "../../lib/inoutclock.hpp"
#include "../../lib/ladder.hpp"
#include "../../lib/metro.hpp"
#include "../../lib/oscillator.hpp"
#include "../../lib/pots.hpp"
#include "../../lib/quantize.hpp"
#include "../../lib/rhythms.hpp"
#include "../../lib/utils.hpp"
#include "tep-controller.hpp"
#include "tep-state.hpp"

#include <algorithm>
#include <functional>
#include <math.h>
#include <random>

namespace platform {

void printRhythm(std::vector<bool>* rhythm) {
  printf("[");
  for (int i = 0; i < rhythm->size(); i++) {
    printf("%d", (*rhythm)[i] ? 1 : 0);
  }
  // printf("; nextStep[%d]; result: %d\n", nextStep, result);
  printf("] ");
}

float processOverdrive(float sample, float amount, float volume) {
  float level = 1.f - volume;
  if (level == 0) {
    return sample;
  }

  float in = softClip(sample / level);

  // times 2 is arbitrary
  float out = powf(fabs(in), 1.f / (1.f + amount * 2.f));
  if (in < 0.f) {
    out = -out;
  }

  out = out * level;
  return out;
}

float lerpByPhase(float a, float b, float amount, float phase) {
  if (phase >= amount) {
    return b;
  }

  float t = phase / amount;
  return a + t * (b - a);
};

// Two-tone Euclidean Polymeters
struct TEPInstrument {
  TEPInstrument(Pots& pots, ButtonInput& bootButton)
    : controller{pots},
      bootButton{bootButton},
      started{false},
      minSample{0.f},
      maxSample{0.f},
      previousOscillatorFrequency{0.f},
      previousCutoff{0.f},
      previousDistortion{0.f},
      nextOscillatorFrequency{0.f},
      nextCutoff{0.f},
      nextDistortion{0.f} {};

  void init(float sampleRateIn) {
    printf("init\n");
    sampleRate = sampleRateIn;

    oscillator1.init(sampleRate);
    oscillator1.setAmp(1.0f);
    oscillator1.setWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    oscillator2.init(sampleRate);
    oscillator2.setAmp(1.0f);
    oscillator2.setWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    filter.init(sampleRate);
    inOutClock.init(sampleRate);
    // clock.init(inOutClock.getTickFrequency(state.bpm.getScaled()), sampleRate);
    clock.init(1.f, sampleRate);

    filter.setFilterMode(LadderFilter::FilterMode::LP24);
  }

  void update() {
    controller.update(state);
  }

  float getVolume() {
    float value = state.volume.getScaled();
    value *= 3.f;

    value = powf(value, 2.f);

    return value;
  }

  float getOscillatorFrequency() {
    // rootIndex is the root note of the scale
    int rootIndex = 3;  // C1
    rootIndex += state.octave.getScaled() * 12.f;
    int degreeOffset = (int)roundf(state.degree.getScaled() * 7.f);

    // chordIndex is the root note of the chord
    int chordIndex = rootIndex + degreeOffset;

    // chordType is major, minor, diminished or augmented
    int chordType = getChordTypeForNote(SCALE_NATURAL_MINOR, degreeOffset);
    int* offsets = getChordOffsetsForType(chordType);

    std::vector<float> arpeggioValues;
    for (int i = 0; i < 3; i++) {
      // noteIndex is the actual note in the chord
      int noteIndex = chordIndex + offsets[i];
      if (noteIndex < 0) {
        noteIndex = 0;
      } else if (noteIndex > 87) {
        noteIndex = 87;
      }
      float freq = notes[noteIndex];
      arpeggioValues.push_back(freq);
    }

    arpeggio.setValues(arpeggioValues);
    arpeggio.setMode(static_cast<ArpeggioMode>(state.arpeggioMode.getScaled()));

    // if the next step in the rhythm is on, then advance the arpeggio.
    // otherwise play the same note as last time. This gives the user the option
    // to play 'stochastic' or to just sustain drones longer before changing the
    // note.
    return degreeRhythm.process() ? arpeggio.process() : arpeggio.getLastValue();
  }

  float getCutoff() {
    float cutoffValue = state.cutoff.getScaled();
    if (cutoffRhythm.process()) {
      cutoffValue += state.cutoffAccent.getScaled();
    }

    if (cutoffValue > 1.f) {
      cutoffValue = 1.f;
    }

    float value = powf(cutoffValue, 3.f) * (HALF_SAMPLE_RATE - 5.f) + 5.f;

    float min = 5.f;
    float max = HALF_SAMPLE_RATE;

    value = fclamp(value, min, max);
    return value;
  }

  float getResonance() {
    return state.resonance.getScaled() * 1.8f;
  }

  float getDistortion() {
    float value = state.distortion.getScaled();
    if (distortionRhythm.process()) {
      value += state.distortionAccent.getScaled();
    }
    if (value > 1.f) {
      value = 1.f;
    }

    return value;
  }

  float getClockFrequency() {
    float value = inOutClock.getTickFrequency(state.bpm.getScaled());
    return value;
  }

  float process() {
    clock.setFreq(getClockFrequency());
    filter.setRes(getResonance());

    bool tick = inOutClock.process(state.bpm.getScaled());

    if (tick) {
      previousOscillatorFrequency = nextOscillatorFrequency;
      previousCutoff = nextCutoff;
      previousDistortion = nextDistortion;

      nextOscillatorFrequency = getOscillatorFrequency();
      nextCutoff = getCutoff();
      nextDistortion = getDistortion();

      // printf("minSample: %.2f, maxSample: %.2f\n", minSample, maxSample);
      minSample = 0;
      maxSample = 0;

      std::vector<bool>* distortionr = &euclideanRhythms[state.distortionRhythm.getScaled()];
      distortionRhythm.setRhythm(distortionr);
      std::vector<bool>* cutoffr = &euclideanRhythms[state.cutoffRhythm.getScaled()];
      cutoffRhythm.setRhythm(cutoffr);
      std::vector<bool>* degreer = &euclideanRhythms[state.degreeRhythm.getScaled()];
      degreeRhythm.setRhythm(degreer);

      printRhythm(distortionr);
      printRhythm(cutoffr);
      printRhythm(degreer);

      printf("arp: %d, distortion: %d, cutoff: %d, degree: %d\n",
             state.arpeggioMode.getScaled(),
             state.distortionRhythm.getScaled(),
             state.cutoffRhythm.getScaled(),
             state.degreeRhythm.getScaled());
    }

    if (!inOutClock.getClockTicks()) {
      return 0.f;
    }

    float glideAmount = state.glide.getScaled();
    float clockPhase = clock.getPhase() / TWOPI_F;

    // printf("glide: %.2f, phase: %.2f\n", glideAmount, clockPhase);

    float freq =
      lerpByPhase(previousOscillatorFrequency, nextOscillatorFrequency, glideAmount, clockPhase);
    oscillator1.setFreq(freq);
    oscillator2.setFreq(freq - state.detune.getScaled());  // TODO
    float cutoff = lerpByPhase(previousCutoff, nextCutoff, glideAmount, clockPhase);
    filter.setFreq(cutoff);

    float sample = oscillator1.process();
    sample += oscillator2.process();
    // if (oscillator1.isEOC()) {
    // oscillator2.reset();
    //}
    sample = filter.process(sample);

    // TODO: distortion

    // overdrive
    // why 0.35? because I just measured the likely min/max value. Just applying
    // this so that overdrive doesn't increase the volume too much.
    // TODO: re-measure if it is really still 0.35 with this new firmware
    float distortion = lerpByPhase(previousDistortion, nextDistortion, glideAmount, clockPhase);
    sample = processOverdrive(sample, distortion, 0.35f);

    minSample = std::min(minSample, sample);
    maxSample = std::max(maxSample, sample);

    sample *= getVolume();
    return softClip(sample);
  }

  TEPState* getState() {
    return &state;
  }

  TEPController controller;
  TEPState state;
  ButtonInput& bootButton;

  float sampleRate;
  bool started;

  Metro clock;
  InOutClock inOutClock{clock};
  Oscillator oscillator1;
  Oscillator oscillator2;
  LadderFilter filter;

  Rhythm distortionRhythm;
  Rhythm cutoffRhythm;
  Rhythm degreeRhythm;

  Arpeggio arpeggio;

  float minSample;
  float maxSample;

  float previousOscillatorFrequency;
  float previousCutoff;
  float previousDistortion;

  float nextOscillatorFrequency;
  float nextCutoff;
  float nextDistortion;
};

}  // namespace platform

#endif  // PLATFORM_TEP_INSTRUMENT_H
