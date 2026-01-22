#ifndef PLATFORM_SDS_INSTRUMENT_H
#define PLATFORM_SDS_INSTRUMENT_H

#include "../../lib/attackordecay.hpp"
#include "../../lib/buttons.hpp"
#include "../../lib/metro.hpp"
#include "../../lib/oscillator.hpp"
#include "../../lib/pm2.hpp"
#include "../../lib/quantize.hpp"
#include "../../lib/sequencer.hpp"
#include "pmd-controller.hpp"
#include "pmd-state.hpp"

namespace platform {

struct PMDInstrument {
  PMDInstrument(Pots& pots, ButtonInput& bootButton)
    : controller{pots},
      bootButton{bootButton},
      isExternalClock{false},
      externalClockTicks{0},
      clockTicks{0},
      samplesSinceLastClockTick{0},
      externalClockFrequency{0.f},
      previousClockState{false},
      started{false},
      envelopeValueSample{0.f} {};

  void init(float sampleRateIn) {
    sampleRate = sampleRateIn;
    envelope.init(sampleRate);
    for (int i = 0; i < 3; i++) {
      pm2[i].init(sampleRate);
    }
    clock.init(getTickFrequency(), sampleRate);
    lfoEnvelope.init(sampleRate);
    lfoEnvelope.setWaveform(Oscillator::WAVE_SIN);
    lfoEnvelope.setFreq(0.5f); // 0.5 Hz
    lfoEnvelope.setAmp(1.f);
    lfoTembre.init(sampleRate);
    lfoTembre.setWaveform(Oscillator::WAVE_SIN);
    lfoTembre.setFreq(0.5f); // 0.5 Hz
    lfoTembre.setAmp(1.f);

    // start the sequencer with random seeds every time we reset
    sequencer.setCVSeed(rand());
    sequencer.setCVPaletteSeed(rand());
  }

  float getTickFrequency() {
    // TODO: move into a custom BPM parameter so we don't unnecessarily keep
    // recalculating this
    if (isExternalClock) {
      if (externalClockTicks < 2) {
        // stop the clock until ticks arrive
        return 0.f;
      }
      float position = round(state.bpm.value * 14.f);

      // 0-6 is divider with 0 being 1/8, 7 is * 1, 8-14 is multiplier with 8 being 2 and 14 being 8
      float multiplier = (position < 7.f) ? 1.f / (8.f - position) : position - 6.f;

      return externalClockFrequency * multiplier;
    } else {
      float value = state.bpm.getScaled();
      if (value < 0.005) {
        // if it is close to zero, then just stop the clock
        return 0.f;
      }
      return value / 60.f * 4.f;  // 16th notes, not quarter notes
    }
  }

  bool isClockTick() {
    samplesSinceLastClockTick++;

    bool tick = false;

    bool clockConnectedState = gpio_get(CLOCK_IN_CONNECTED_PIN);
    if (clockConnectedState) {
      // external clock
      isExternalClock = true;

      // the pin is inverted because it is tied to an NPN transistor
      bool clockState = !gpio_get(CLOCK_IN_PIN);
      if (clockState != previousClockState) {
        // clock state changed
        previousClockState = clockState;

        if (clockState) {
          externalClockFrequency = sampleRate / samplesSinceLastClockTick * 2.f;
          samplesSinceLastClockTick = 0.f;

          float position = round(state.bpm.value * 14.f);
          if (position < 7.f) {
            float divider = 8.f - position;
            // printf("clockTicks: %d, divider: %.2f\n", externalClockTicks, divider);

            if (externalClockTicks % (int)divider == 0) {
              if (clock.getPhase() > 0.5f) {
                // Make the clock tick now because the external clock is faster than
                // our calculations and we'd miss a tick if we don't. If the phase
                // is < 0.5 then we assume the click is slower than our calculations
                // and we don't tick again because we'd tick twice in quick succession.

                tick = true;
              }
              clock.reset();
            }
          } else {
            if (clock.getPhase() > 0.5f) {
              // Make the clock tick now because the external clock is faster than
              // our calculations and we'd miss a tick if we don't. If the phase
              // is < 0.5 then we assume the click is slower than our calculations
              // and we don't tick again because we'd tick twice in quick succession.

              tick = true;
            }
            clock.reset();
          }

          // (Don't change the clock frequency now because we assume we'll set
          // the frequency in the process method based on what the
          // divider/multiplier is set to anyway)

          externalClockTicks++;
        }
      }
    } else {
      // internal clock
      isExternalClock = false;
    }

    // either internal or external
    return clock.process() || tick;
  }


  void update() {
    controller.update(state);
  }

  float process() {
    bool tick = false;
    if (isClockTick()) {
      tick = true;

      clockTicks++;
      // printf("minSample: %.2f, maxSample: %.2f\n", minSample, maxSample);
      // minSample = 0;
      // maxSample = 0;

      // Going with the teenage engineering approach of clock ticks on every
      // second 16th note. Same as for interpreting clock inputs.
      if (clockTicks % 2 == 0) {
        // the pin is inverted because it is tied to an NPN transistor
        gpio_put(CLOCK_OUT_PIN, false);
      } else {
        gpio_put(CLOCK_OUT_PIN, true);
      }
    }

    lfoEnvelope.setFreq(state.envelopeLFORate.getScaled());
    lfoTembre.setFreq(state.tembreLFORate.getScaled());

    // TODO: sample and hold a random value on each tick to use as
    // modulation for that value, but have that random sequence reset (and
    // scrable) with the other ones. Then we can modulate up to 4 things.
    float lfoEnvelopeValue = lfoEnvelope.process();

    if (tick) {
      if (sequencer.getCurrentStep() == 0) {
        if (randomProb() < state.scramble.getScaled()) {
          printf("scramble!\n");
          sequencer.setCVSeed(sequencer.getCVSeed() + 1);
          sequencer.setCVPaletteSeed(sequencer.getCVPaletteSeed() + 1);
        }
      }
      //printf("%.2f\n", smoothResult.value);

      // TODO: make these parameters "sticky" so they only update when changed
      // enough. To prevent oscillation.
      // TODO: only set these if the values changed
      sequencer.setSequenceLength(state.length.getScaled());
      sequencer.setComplexity(state.complexity.getScaled());
      sequencer.setDensity(state.density.getScaled());
      sequencer.setSpread(state.spread.getScaled());
      sequencer.setBias(state.bias.getScaled());

      printf("length: %d, complexity: %d, bias: %.2f, density: %.2f, spread: %.2f, envLFO: %.2f, tembreLFO: %.2f\n",
             sequencer.getSequenceLength(),
             sequencer.getComplexity(),
             sequencer.getBias(),
             sequencer.getDensity(),
             sequencer.getSpread(),
             state.envelopeLFORate.getScaled(),
             state.tembreLFORate.getScaled()
            );

      auto [gate, cv] = sequencer.process();

      if (gate) {
        started = true;

        envelopeValueSample = lfoEnvelopeValue;

        // TODO: also do this stuff on the first sample after reset

        int scale = SCALE_HARMONIC_MINOR;
        float note = 76.f * state.baseFreq.getScaled();
        float range = state.range.getScaled() * (cv - 0.5f);
        int *scaleNotes;
        int numScaleNotes = getScaleNotesForScale(scale, &scaleNotes);
        int scaleOffset = getScaleOffsetForNote(range, numScaleNotes);
        int semitones = (int) getSemitoneOffsetForNote(scale, range);
        float baseFrequency = addSemitonesToFrequency(getFrequencyForNote(scale, note), semitones);
        int degree = getChordScaleDegreeForNote(scale, range);
        int type = getChordTypeForNote(scale, degree);
        int *offsets = getChordOffsetsForType(type);


        pm2[0].setFrequency(baseFrequency);
        pm2[0].setRatio(1.f);

        pm2[1].setFrequency(addSemitonesToFrequency(baseFrequency, offsets[1]));
        pm2[1].setRatio(1.f);

        pm2[2].setFrequency(addSemitonesToFrequency(baseFrequency, offsets[2]));
        pm2[2].setRatio(1.f);

        envelope.trigger();

        for (int i = 0; i < 3; i++) {
          pm2[i].reset();

        }
      }
    }

    // use the envelope LFO value from from the last tick so the note plays as long as expected
    float envelopeValue = monopolar(envelopeValueSample) * state.envelopeLFODepth.getScaled();
    float decay = fclamp(state.decay.getScaled() + envelopeValue, 0.f, 1.f);
    envelope.setTimeAndDirection(1.f - decay);

    float tembreValue = monopolar(lfoTembre.process()) * state.tembreLFODepth.getScaled();
    float depth = fclamp(state.modulatorDepth.getScaled() + tembreValue, 0.f, 1.f);
    for (int i = 0; i < 3; i++) {
      pm2[i].setDepth(depth);
    }


    clock.setFreq(getTickFrequency());

    // printf("%.2f, %.2f, %.2f, %.2f\n", state.volume.getScaled(), state.carrierFreq.getScaled(),
    // ratio, state.modulatorDepth.getScaled());

    float sample = 0.f;
    if (started)  {
      float envelopeValue = envelope.process();
      for (int i = 0; i < 3; i++) {
        sample += (pm2[i].process() * envelopeValue);
      }
    }

    // TODO: non-linear volume
    // TODO: pull out master volume into its own library so we can reuse it and
    // always be sure it will work
    return softClip(sample * state.volume.getScaled());
  }

  PMDState* getState() {
    return &state;
  }

  private:
  float sampleRate;
  bool isExternalClock;
  int externalClockTicks;
  int clockTicks;
  int samplesSinceLastClockTick;
  float externalClockFrequency;
  bool previousClockState;
  bool started;

  ButtonInput& bootButton;
  PMDState state;
  PMDController controller;
  PM2 pm2[3];
  Metro clock;
  AttackOrDecayEnvelope envelope;
  Sequencer sequencer;
  Oscillator lfoTembre;
  Oscillator lfoEnvelope;
  float envelopeValueSample;
};

}  // namespace platform

#endif  // PLATFORM_PMD_INSTRUMENT_H