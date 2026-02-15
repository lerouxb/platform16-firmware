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
      previousVolume{0.f},
      nextOscillatorFrequency{0.f},
      nextCutoff{0.f},
      nextVolume{0.f},
      lastChordIndex{0},
      lastArpeggioMode{0} {};

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

  float getOscillatorFrequency() {
    // rootIndex is the root note of the scale
    int rootIndex = 3;  // C1
    rootIndex += state.octave.getScaled() * 12.f;
    int degreeOffset = (int)roundf(state.degree.getScaled() * 7.f);

    // chordIndex is the root note of the chord
    int chordIndex = rootIndex + degreeOffset;

    ArpeggioMode arpeggioMode = static_cast<ArpeggioMode>(state.arpeggioMode.getScaled());

    // caching so we don't keep recalculating and setting vectors and doing a
    // bunch of maths unnecessarily when the note and arpeggio mode haven't
    // changed.
    if (chordIndex == lastChordIndex && arpeggioMode == lastArpeggioMode) {
      return arpeggio.getLastValue();
    }

    printf("caching chordIndex: %d, arpeggioMode: %d\n", chordIndex, (int)arpeggioMode);
    lastChordIndex = chordIndex;
    lastArpeggioMode = arpeggioMode;

    arpeggio.setMode(arpeggioMode);

    // chordType is major, minor, diminished or augmented
    int chordType = getChordTypeForNote(SCALE_NATURAL_MINOR, degreeOffset);
    int* offsets = getChordOffsetsForType(chordType);
    std::vector<int> offsetsVec{offsets, offsets + 3};

    // add the 7th
    //offsetsVec.push_back(naturalMinorOffsets[6]);
    // add the 6th
    offsetsVec.push_back(naturalMinorOffsets[5]);
    std::sort(offsetsVec.begin(), offsetsVec.end());

    std::vector<float> arpeggioValues;
    for (int i = 0; i < 4; i++) {
      // noteIndex is the actual note in the chord
      int noteIndex = chordIndex + offsetsVec[i];
      if (noteIndex < 0) {
        noteIndex = 0;
      } else if (noteIndex > 87) {
        noteIndex = 87;
      }
      float freq = notes[noteIndex];
      arpeggioValues.push_back(freq);
    }

    arpeggio.setValues(arpeggioValues);

    // return degreeRhythm.getLastValue() ? arpeggio.process() : arpeggio.getLastValue();
    return arpeggio.getLastValue();
  }

  float getCutoff() {
    float cutoffValue = state.cutoff.getScaled();
    if (cutoffRhythm.getLastValue()) {
      cutoffValue += state.cutoffAccent.getScaled();
    }

    if (cutoffValue > 1.f) {
      cutoffValue = 1.f;
    }

    // TODO: cache by cutoffValue (probably quantized)

    float value = powf(cutoffValue, 3.f) * (HALF_SAMPLE_RATE - 5.f) + 5.f;

    float min = 5.f;
    float max = HALF_SAMPLE_RATE;

    value = fclamp(value, min, max);
    return value;
  }

  float getResonance() {
    return state.resonance.getScaled() * 1.8f;
  }

  float getVolume() {
    float value = state.volume.getScaled();
    float volumeAccent = state.volumeAccent.getScaled();
    bool isAccent = volumeRhythm.getLastValue();
    
    // TODO: cache by value, volumeAccent and isAccent

    value *= 3.f;

    if (isAccent) {
      // TODO: this needs work. accent should somehow multiply it so that volume
      // 0 means no sound even with accent, but accent should always make a big
      // difference even if volume is low
      value *= (1.f + volumeAccent * 2.f);
    }
    
    /*
    if (value > 1.f) {
      value = 1.f;
    }
    */

    value = powf(value, 2.f);

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
      previousVolume = nextVolume;

      if (degreeRhythm.process()) {
        // if the next step in the rhythm is on, then advance the arpeggio.
        // otherwise play the same note as last time. This gives the user the option
        // to play 'stochastic' or to just sustain drones longer before changing the
        // note.

        arpeggio.process();
      }
      volumeRhythm.process();
      cutoffRhythm.process();

      // printf("minSample: %.2f, maxSample: %.2f\n", minSample, maxSample);
      minSample = 0;
      maxSample = 0;

      std::vector<bool>* volumer = &euclideanRhythms[state.volumeRhythm.getScaled()];
      volumeRhythm.setRhythm(volumer);
      std::vector<bool>* cutoffr = &euclideanRhythms[state.cutoffRhythm.getScaled()];
      cutoffRhythm.setRhythm(cutoffr);
      std::vector<bool>* degreer = &euclideanRhythms[state.degreeRhythm.getScaled()];
      degreeRhythm.setRhythm(degreer);

      /*
      printRhythm(volumer);
      printRhythm(cutoffr);
      printRhythm(degreer);

      printf("arp: %d, volume: %d, cutoff: %d, degree: %d\n",
             state.arpeggioMode.getScaled(),
             state.volumeRhythm.getScaled(),
             state.cutoffRhythm.getScaled(),
             state.degreeRhythm.getScaled());
      */
    }

    if (!inOutClock.getClockTicks()) {
      return 0.f;
    }

    // update all these on every tick for now
    nextOscillatorFrequency = getOscillatorFrequency();
    nextCutoff = getCutoff();
    nextVolume = getVolume();

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
    sample = processOverdrive(sample, state.distortion.getScaled(), 0.35f);

    minSample = std::min(minSample, sample);
    maxSample = std::max(maxSample, sample);

    float volume = lerpByPhase(previousVolume, nextVolume, glideAmount, clockPhase);
    sample *= volume;
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

  Rhythm volumeRhythm;
  Rhythm cutoffRhythm;
  Rhythm degreeRhythm;

  Arpeggio arpeggio;

  float minSample;
  float maxSample;

  float previousOscillatorFrequency;
  float previousCutoff;
  float previousVolume;

  float nextOscillatorFrequency;
  float nextCutoff;
  float nextVolume;

  int lastChordIndex;
  ArpeggioMode lastArpeggioMode;
};

}  // namespace platform

#endif  // PLATFORM_TEP_INSTRUMENT_H
