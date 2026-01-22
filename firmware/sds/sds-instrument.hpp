#ifndef PLATFORM_SDS_INSTRUMENT_H
#define PLATFORM_SDS_INSTRUMENT_H

#include "../../lib/buttons.hpp"
#include "../../lib/attackordecay.hpp"
#include "../../lib/gpio.hpp"
#include "../../lib/ladder.hpp"
#include "../../lib/metro.hpp"
#include "../../lib/oscillator.hpp"
#include "../../lib/pots.hpp"
#include "../../lib/utils.hpp"
#include "../../lib/quantize.hpp"
#include "sds-controller.hpp"
#include "sds-state.hpp"

#include <algorithm>
#include <functional>
#include <math.h>
#include <random>


// This is kinda a balance - I don't want too many algorithms because the
// labelling becomes very busy. I figure ramps are the first to go because you
// can just use the first half of a corresponding triangle.
// This could also be replaced with actual arpeggios, but those work better with
// more controls like length and which chord to use and so on, kinda clashing
// with sequence length and scale.
#define ALGORITHM_NONE 0
//#define ALGORITHM_RAMP_UP 1
//#define ALGORITHM_RAMP_DOWN 2
#define ALGORITHM_TRIANGLE_UP 1
#define ALGORITHM_TRIANGLE_DOWN 2
#define ALGORITHM_TWO_TRIANGLES_UP 3
#define ALGORITHM_TWO_TRIANGLES_DOWN 4
#define ALGORITHM_FOUR_TRIANGLES_UP 5
#define ALGORITHM_FOUR_TRIANGLES_DOWN 6
//#define ALGORITHM_RAMP_UP_DOWN 7
//#define ALGORITHM_RAMP_DOWN_UP 8
#define ALGORITHM_TRIANGLE_UP_DOWN 7
#define ALGORITHM_TRIANGLE_DOWN_UP 8

namespace platform {

float maybeAttackDecay(float env, float value) {
  // close to the center means sustain
  // TODO: we should probably subtract this when calculating the ends of the envelope in attackdecay
  if (value > 0.45 && value < 0.55) {
    // sustain
    return 0.5f;
  }
  return value;
}

struct SDSInstrument {
  SDSInstrument(Pots& pots, ButtonInput& bootButton)
    : controller{pots},
      bootButton{bootButton},
      isExternalClock{false},
      externalClockTicks{0},
      clockTicks{0},
      samplesSinceLastClockTick{0},
      externalClockFrequency{0.f},
      playedPitchChanged{true},
      cachedRawBasePitch{0},
      lastPlayedPitchAmount{0},
      lastPlayedFilterAmount{0},
      previousAlgorithm{0},
      previousClockState{false},
      previousScale{0},
      lastNoise{0.f},
      noiseSteps{0},
      minSample{0},
      maxSample{0}
      {};

  void init(float sampleRateIn) {
    sampleRate = sampleRateIn;
    oscillator.init(sampleRate);
    oscillator.setAmp(1.f);
    oscillator.setWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    clock.init(getTickFrequency(), sampleRate);

    volumeEnvelope.init(sampleRate);
    cutoffEnvelope.init(sampleRate);

    filter.init(sampleRate);

    randomizeSequence();
  }

  void update() {
    controller.update(state);
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

  float getVolume() {
    float value = state.volume.getScaled();
    
    // 3^2 = 9 so this can be come bigger than "unity" but because of filters
    // and decays and so on that's kinda reasonable. If we restrict it to 1,
    // then it is basically impossible to get to a reasonable volume under most
    // circumstances. It _can_ clip the sample at max volume, though.
    value *= 3; 

    value = powf(value, 2.f);

    return value;
  }

  float _getNote(int scale) {
    // if the pitch is not quantized, then the pitch always changes, otherwise
    // only when we get to a played step
    playedPitchChanged = scale ? playedPitchChanged : true;

    float rawValue = playedPitchChanged || !scale || !cachedRawBasePitch ? state.basePitch.getScaled() : cachedRawBasePitch;
    cachedRawBasePitch = rawValue;

    float note = 76.f * rawValue;
    return note;
  }

  float getOscillatorFrequency() {
    // TODO: the curve still needs work.

    int scale = state.scale.getScaled();
    float note = _getNote(scale);
    float baseFrequency = getFrequencyForNote(scale, note);

    /*
    float rawOffsetValue = playedPitchChanged || !scale || !cachedRawPitchOffset ? state.pitchOffset.getScaled() : cachedRawPitchOffset;
    cachedRawPitchOffset = rawOffsetValue;
    float pitchOffsetSemitones = _getSemitoneOffsetForNote(scale, rawOffsetValue, playedPitchChanged && scale);
    if (playedPitchChanged && scale) {
      printf("scale: %d, note: %f, baseFrequency: %f, rawOffsetValue: %f, pitchOffsetSemitones: %f\n", scale, note, baseFrequency, rawOffsetValue, pitchOffsetSemitones);
    }
      */

    // scale up up to 1 octave
    // TODO: 2 might be nice?
    float rawAmount = state.pitchAmount.value * lastPlayedPitchAmount;
    // TODO: This actually has to respond to the new transposed pitch, not the root note
    float pitchAmountOffsetSemitones = getSemitoneOffsetForNote(scale, rawAmount);

    float value;
    if (scale) {
      //int newNote = note + pitchOffsetSemitones + pitchAmountOffsetSemitones;
      int newNote = note + pitchAmountOffsetSemitones;

      // clamp it just in case
      if (newNote < 0) {
        newNote = 0;
      } else if (newNote > 87) {
        newNote = 87;
      }

      value = notes[newNote];

    } else {
      //value = addSemitonesToFrequency(baseFrequency, pitchOffsetSemitones);
      //      //value = addSemitonesToFrequency(value, pitchAmountOffsetSemitones);
      value = addSemitonesToFrequency(baseFrequency, pitchAmountOffsetSemitones);

      // clamp it just in case
      value = fclamp(value, 0.f, 22050.f); 
    }

    if (playedPitchChanged) {
      playedPitchChanged = false;
      previousScale = scale;
    }

    return value;
  }

  float getFilterCutoff() {
    // All the way counter clockwise is low pass 5Hz. The middle is lowpass
    // HALF_SAMPLE_RATE or high pass 5Hz. All the way clockwise is highpass
    // HALF_SAMPLE_RATE.
    float cutoffValue = state.cutoff.value <= 0.f ? 1.f + state.cutoff.value : state.cutoff.value;
    float value = powf(cutoffValue, 3.f) * (HALF_SAMPLE_RATE - 5.f) + 5.f;

    float normalisedValue = fabs(state.cutoffAmount.value);
    float rawFilterAmount = powf(normalisedValue * lastPlayedFilterAmount, 0.5f);
    float amountValue = powf(rawFilterAmount, 3.f) * (HALF_SAMPLE_RATE - 5.f) + 5.f;

    // when in lowpass mode, amount moves the filter up, allowing more frequencies through.
    // when in highpass mode, amount moves the filter down, allowing more frequencies through.
    if (state.cutoff.value <= 0.f) {
      // low pass: add when increasing
      value += (state.cutoffAmount.value <= 0.f ? -amountValue : amountValue);
    } else {
      // high pass: subtract when increasing
      value -= (state.cutoffAmount.value <= 0.f ? -amountValue : amountValue);
    }

    float min = 5.f;
    float max = HALF_SAMPLE_RATE;

    value = fclamp(value, min, max);

    return value;
  }

  void sortByAlgorithm() {
    // TODO: we can probably move this
    std::random_device rd;
    std::mt19937 g(rd());

    // get back to the original random order
    // (we could also just shuffle again?)
    for (int i = 0; i < 32; i++) {
      state.pitchAmounts[i] = state.pitchAmountsBackup[i];
    }

    switch (state.algorithm.getScaled()) {
      case ALGORITHM_NONE:
        break;

      /*
      case ALGORITHM_RAMP_UP: {
        // ramp up
        std::sort(std::begin(state.pitchAmounts), std::end(state.pitchAmounts));
        break;
      }

      case ALGORITHM_RAMP_DOWN: {
        // ramp down
        std::sort(std::begin(state.pitchAmounts), std::end(state.pitchAmounts), std::greater<float>{});
        break;
      }
      */

      case ALGORITHM_TRIANGLE_UP: {
        // triangle up
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::greater<float>{});
        break;
      }

      case ALGORITHM_TRIANGLE_DOWN: {
        // triangle down
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::less<float>{});
        break;
      }

      case ALGORITHM_TWO_TRIANGLES_UP: {
        // two triangles up
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 8,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 24,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 24,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::greater<float>{});
        break;
      }

      case ALGORITHM_TWO_TRIANGLES_DOWN: {
        // two triangles down
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 8,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 24,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 24,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::less<float>{});
        break;
      }

      case ALGORITHM_FOUR_TRIANGLES_UP: {
        // four triangles up
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 4,
                          std::begin(state.pitchAmounts) + 4,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 4,
                          std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 8,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 12,
                          std::begin(state.pitchAmounts) + 12,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 12,
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 20,
                          std::begin(state.pitchAmounts) + 20,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 20,
                          std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 24,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 28,
                          std::begin(state.pitchAmounts) + 28,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 28,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::greater<float>{});
        break;
      }

      case ALGORITHM_FOUR_TRIANGLES_DOWN: {
        // four triangles down
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 4,
                          std::begin(state.pitchAmounts) + 4,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 4,
                          std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 8,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 12,
                          std::begin(state.pitchAmounts) + 12,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 12,
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 20,
                          std::begin(state.pitchAmounts) + 20,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 20,
                          std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 24,
                          std::less<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 28,
                          std::begin(state.pitchAmounts) + 28,
                          std::greater<float>{});
        std::partial_sort(std::begin(state.pitchAmounts) + 28,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::less<float>{});
        break;
      }

      /*
      case ALGORITHM_RAMP_UP_DOWN: {
        // ramp up down
        float sorted[32];
        std::copy(
          std::begin(state.pitchAmountsBackup), std::end(state.pitchAmountsBackup), std::begin(sorted));
        std::sort(std::begin(sorted), std::end(sorted), std::less<float>{});

        for (int i = 0; i < 32; i++) {
          if (i % 2 == 0) {
            state.pitchAmounts[i] = sorted[i / 2];
          } else {
            state.pitchAmounts[i] = sorted[16 + (i / 2)];
          }
        }
        break;
      }


      case ALGORITHM_RAMP_DOWN_UP: {
        // ramp down up
        float sorted[32];
        std::copy(
          std::begin(state.pitchAmountsBackup), std::end(state.pitchAmountsBackup), std::begin(sorted));
        std::sort(std::begin(sorted), std::end(sorted), std::greater<float>{});

        for (int i = 0; i < 32; i++) {
          if (i % 2 == 0) {
            state.pitchAmounts[i] = sorted[i / 2];
          } else {
            state.pitchAmounts[i] = sorted[16 + (i / 2)];
          }
        }
        break;
      }
      */

      case ALGORITHM_TRIANGLE_UP_DOWN: {
        // triangle up down
        float sorted[32];
        std::copy(
          std::begin(state.pitchAmountsBackup), std::end(state.pitchAmountsBackup), std::begin(sorted));

        std::partial_sort(std::begin(sorted),
                          std::begin(sorted) + 16,
                          std::begin(sorted) + 16,
                          std::less<float>{});
        std::partial_sort(std::begin(sorted) + 16,
                          std::end(sorted),
                          std::end(sorted),
                          std::greater<float>{});

        for (int i = 0; i < 32; i++) {
          if (i % 2 == 0) {
            state.pitchAmounts[i] = sorted[i / 2];
          } else {
            state.pitchAmounts[i] = sorted[16 + (i / 2)];
          }
        }
        break;
      }


      case ALGORITHM_TRIANGLE_DOWN_UP: {
        // triangle down up
        float sorted[32];
        std::copy(
          std::begin(state.pitchAmountsBackup), std::end(state.pitchAmountsBackup), std::begin(sorted));

        std::partial_sort(std::begin(sorted),
                          std::begin(sorted) + 16,
                          std::begin(sorted) + 16,
                          std::greater<float>{});
        std::partial_sort(std::begin(sorted) + 16,
                          std::end(sorted),
                          std::end(sorted),
                          std::less<float>{});

        for (int i = 0; i < 32; i++) {
          if (i % 2 == 0) {
            state.pitchAmounts[i] = sorted[i / 2];
          } else {
            state.pitchAmounts[i] = sorted[16 + (i / 2)];
          }
        }
        break;
      }
    }
  }

  void randomizeSequence() {
    // randomize the whole sequence
    for (int i = 0; i < 32; i++) {
      state.steps[i] = randomProb();
      state.pitchAmounts[i] = randomProb();
      state.pitchAmountsBackup[i] = state.pitchAmounts[i];
      state.filterAmounts[i] = randomProb();
    }

    if (state.stepCount.getScaled() != 0) {
      // always play the down beat, otherwise when you shorten stepCount a sequence might sound off
      state.steps[0] = 1.f;
    }

    sortByAlgorithm();
  }

  float processOverdrive(float sample, float amount, float volume) {
    float level = 1.f - volume;
    if (level == 0) {
      return sample;
    }

    float in = softClip(sample / level);

    // times 2 is arbitrary
    float out = powf(fabs(in), 1.f/(1.f+amount*2.f)); 
    if (in < 0.f) {
      out = -out;
    }

    out = out * level;
    return out;
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
          externalClockFrequency =  sampleRate / samplesSinceLastClockTick * 2.f;
          samplesSinceLastClockTick = 0.f;

          float position = round(state.bpm.value * 14.f);
          if (position < 7.f) {
            float divider = 8.f - position;
            //printf("clockTicks: %d, divider: %.2f\n", externalClockTicks, divider);

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

  float process() {
    uint stepCount = state.stepCount.getScaled();

    // -1 to 1
    float volumeEnv = state.volumeEnvelope.value;
    float cutoffEnv = state.cutoffEnvelope.value;

    // doing this before we might trigger the envelopes to make sure that the initial direction is set properly
    volumeEnvelope.setTimeAndDirection(volumeEnv);
    cutoffEnvelope.setTimeAndDirection(cutoffEnv);

    if (isClockTick()) {
      clockTicks++;
      //printf("minSample: %.2f, maxSample: %.2f\n", minSample, maxSample);
      minSample = 0;
      maxSample = 0;

      // Going with the teenage engineering approach of clock ticks on every
      // second 16th note. Same as for interpreting clock inputs.
      if (clockTicks % 2 == 0) {
        // the pin is inverted because it is tied to an NPN transistor
        gpio_put(CLOCK_OUT_PIN, false);
      } else {
        gpio_put(CLOCK_OUT_PIN, true);
      }

      if (stepCount == 0) {
        randomizeSequence();
      }

      if (isPlayedStep()) {
        // recalculate volume, frequency and cutoff, the steps..
        playedPitchChanged = true;
        lastPlayedPitchAmount = state.pitchAmounts[state.step];
        lastPlayedFilterAmount = state.filterAmounts[state.step];

        float evolve = state.evolve.value;
        float evolveAbs = fabs(evolve);
        // only evolve if the random probability is greater than the current
        // absolute evolve value
        bool evolved = false;
        if (evolveAbs/4.f > randomProb()) {
          evolved = true;
          if (evolve > 0.f) {
            state.filterAmounts[state.step] = randomProb();
            // change the backup, because we're going to sort by algorithm
            state.pitchAmountsBackup[state.step] = randomProb();
          }
          else {
            state.steps[state.step] = randomProb();
          }

          if (state.stepCount.getScaled() != 0) {
            // always play the down beat, otherwise when you shorten stepCount a sequence might sound off
            state.steps[0] = 1.f;
          }
        }

        // trigger notes, advance sequencer, etc
        int algorithm = state.algorithm.getScaled();
        // if the algorithm has changed or the sequence evolved, resort the
        // amounts. So all algorithms other than random keep their basic shape
        if (algorithm != previousAlgorithm || evolved) {
          previousAlgorithm = algorithm;
          sortByAlgorithm();
        }

        volumeEnvelope.trigger();
        cutoffEnvelope.trigger();
      } else {
        // TODO: evolve non-played steps so they can come back to life.
        // Otherwise if you evolve skips the sequence eventually empties.
      }

      state.step++;

      if (state.step >= stepCount) {
        state.step = 0;
      }
    }


    clock.setFreq(getTickFrequency());
    filter.setRes(state.resonance.getScaled() * 1.8f);


    float sample = 0.f;

    // oscillator (if not stopped)
    float frequency = getOscillatorFrequency();
    if (frequency > 28.f) {
      oscillator.setFreq(frequency);
      sample = oscillator.process(); // -0.5 to 0.5
    }

    // noise
    if (state.noise.value > 0.f) {
      noiseSteps++;
      if (noiseSteps >= (int) ((1.f - state.noise.getScaled()) * 1000.f)) {
        noiseSteps = 0;
        lastNoise = (randomProb() * 2.f - 1.f) * state.noise.getScaled();
      } 
      sample += lastNoise;
    }

    // filter
    float filterCutoff = getFilterCutoff();
    // when in lowpass mode, the envelope closes the filter towards 5Hz.
    // when in highpass mode, the envelope closes the filter towards HALF_SAMPLE_RATE.
    float cutoff = state.cutoff.value <= 0.f
      ? filterCutoff * maybeAttackDecay(cutoffEnv, cutoffEnvelope.process())
      : filterCutoff + ((HALF_SAMPLE_RATE - filterCutoff) * (1.f - maybeAttackDecay(cutoffEnv, cutoffEnvelope.process())));

    if (state.cutoff.value <= 0.f) {
      // low pass
      filter.setFilterMode(LadderFilter::FilterMode::LP24);
    } else {
      // high pass
      filter.setFilterMode(LadderFilter::FilterMode::HP24);

    }
    filter.setFreq(fmax(5.f, cutoff));
    sample = filter.process(sample);


    // overdrive
    // why 0.35? because I just measured the likely min/max value. Just applying
    // this so that overdrive doesn't increase the volume too much.
    sample = processOverdrive(sample, state.drive.getScaled(), 0.35f);

    minSample = std::min(minSample, sample);
    maxSample = std::max(maxSample, sample);

    // volume
    sample = sample * getVolume() * maybeAttackDecay(volumeEnv, volumeEnvelope.process());


    // cache what we can until the next clock tick
    // The raw pitch value can drift very slightly and then quantize to
    // an adjacent pitch on different samples within the same step. So when
    // we're in a step we cache the raw pitch value

    sample = softClip(sample);
    
    return sample;
  }

  SDSState* getState() {
    return &state;
  }

  private:
  bool isExternalClock;
  int externalClockTicks;
  int clockTicks;
  int samplesSinceLastClockTick;
  float externalClockFrequency;
  float sampleRate;
  bool playedPitchChanged;
  float cachedRawBasePitch;
  //float cachedRawPitchOffset;
  // TODO: also cache the filter, pitch and volume?
  float lastPlayedPitchAmount;
  float lastPlayedFilterAmount;
  int previousAlgorithm;
  bool previousClockState;
  int previousScale;
  float lastNoise;
  int noiseSteps;
  ButtonInput& bootButton;
  SDSState state;
  SDSController controller;
  Metro clock;
  AttackOrDecayEnvelope volumeEnvelope;
  AttackOrDecayEnvelope cutoffEnvelope;
  Oscillator oscillator;
  LadderFilter filter;

  float minSample;
  float maxSample;
};

}  // namespace platform

#endif  // PLATFORM_SDS_INSTRUMENT_H