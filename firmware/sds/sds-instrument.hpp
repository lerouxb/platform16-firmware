#ifndef PLATFORM_STEP_INSTRUMENT_H
#define PLATFORM_STEP_INSTRUMENT_H

#include "../../lib/buttons.hpp"
#include "../../lib/attackdecay.hpp"
#include "../../lib/gpio.hpp"
#include "../../lib/ladder.hpp"
#include "../../lib/metro.hpp"
#include "../../lib/oscillator.hpp"
#include "../../lib/pots.hpp"
#include "../../lib/utils.hpp"
#include "sds-controller.hpp"
#include "sds-state.hpp"

#include <algorithm>
#include <functional>
#include <math.h>
#include <random>

// TODO: the problem here is there are only pots, no switches. So it is hard to
// tell where you are. Maybe with a pot that has an arrow and nice markings on
// the enclosure it can work.
#define SCALE_UNQUANTIZED 0
#define SCALE_CHROMATIC 1
#define SCALE_MAJOR 2
// maybe only keep natural OR harmonic OR melodic
#define SCALE_NATURAL_MINOR 3
#define SCALE_HARMONIC_MINOR 4
#define SCALE_PENTATONIC_MAJOR 5
#define SCALE_PENTATONIC_MINOR 6
// maybe the blues scale?

#define ALGORITHM_NONE 0
#define ALGORITHM_RAMP_UP 1
#define ALGORITHM_RAMP_DOWN 2
#define ALGORITHM_TRANGLE_UP 3
#define ALGORITHM_TRIANGLE_DOWN 4
#define ALGORITHM_TWO_TRIANGLES_UP 5
#define ALGORITHM_TWO_TRIANGLES_DOWN 6
#define ALGORITHM_FOUR_TRIANGLES_UP 7
#define ALGORITHM_FOUR_TRIANGLES_DOWN 8
#define ALGORITHM_RAMP_UP_DOWN 9
#define ALGORITHM_RAMP_DOWN_UP 10
#define ALGORITHM_TRIANGLE_UP_DOWN 11
#define ALGORITHM_TRIANGLE_DOWN_UP 12

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

/*
scales
0 unquantized
1 chromatic
2 major
3 natural minor
4 harmonic minor
5 pentatonic major
6 pentatonic minor
*/
int chromaticOffsets[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
int majorOffsets[] = {0, 2, 4, 5, 7, 9, 11};
int naturalMinorOffsets[] = {0, 2, 3, 5, 7, 8, 10};
int harmonicMinorOffsets[] = {0, 2, 3, 5, 7, 8, 11};
int pentatonicMajorOffsets[] = {0, 2, 4, 7, 9};
int pentatonicMinorOffsets[] = {0, 3, 5, 7, 10};

// 76+12 = 88
float notes[] = {
  /*A0*/ 27.5,
  /*A♯0/B♭0*/ 29.13524,
  /*B0*/ 30.86771,
  /*C1*/ 32.7032,
  /*C♯1/D♭1*/ 34.64783,
  /*D1*/ 36.7081,
  /*D♯1/E♭1*/ 38.89087,
  /*E1*/ 41.20344,
  /*F1*/ 43.65353,
  /*F♯1/G♭1*/ 46.2493,
  /*G1*/ 48.99943,
  /*G♯1/A♭1*/ 51.91309,
  /*A1*/ 55.f,
  /*A♯1/B♭1*/ 58.27047,
  /*B1*/ 61.73541,
  /*C2*/ 65.40639,
  /*C♯2/D♭2*/ 69.29566,
  /*D2*/ 73.41619,
  /*D♯2/E♭2*/ 77.78175,
  /*E2*/ 82.40689,
  /*F2*/ 87.30706,
  /*F♯2/G♭2*/ 92.49861,
  /*G2*/ 97.99886,
  /*G♯2/A♭2*/ 103.8262,
  /*A2*/ 110.f,
  /*A♯2/B♭2*/ 116.5409,
  /*B2*/ 123.4708,
  /*C3*/ 130.8128,
  /*C♯3/D♭3*/ 138.5913,
  /*D3*/ 146.8324,
  /*D♯3/E♭3*/ 155.5635,
  /*E3*/ 164.8138,
  /*F3*/ 174.6141,
  /*F♯3/G♭3*/ 184.9972,
  /*G3*/ 195.9977,
  /*G♯3/A♭3*/ 207.6523,
  /*A3*/ 220.f,
  /*A♯3/B♭3*/ 233.0819,
  /*B3*/ 246.9417,
  /*C4*/ 261.6256,
  /*C♯4/D♭4*/ 277.1826,
  /*D4*/ 293.6648,
  /*D♯4/E♭4*/ 311.127,
  /*E4*/ 329.6276,
  /*F4*/ 349.2282,
  /*F♯4/G♭4*/ 369.9944,
  /*G4*/ 391.9954,
  /*G♯4/A♭4*/ 415.3047,
  /*A4*/ 440.f,
  /*A♯4/B♭4*/ 466.1638,
  /*B4*/ 493.8833,
  /*C5*/ 523.2511,
  /*C♯5/D♭5*/ 554.3653,
  /*D5*/ 587.3295,
  /*D♯5/E♭5*/ 622.254,
  /*E5*/ 659.2551,
  /*F5*/ 698.4565,
  /*F♯5/G♭5*/ 739.9888,
  /*G5*/ 783.9909,
  /*G♯5/A♭5*/ 830.6094,
  /*A5*/ 880.f,
  /*A♯5/B♭5*/ 932.3275,
  /*B5*/ 987.7666,
  /*C6*/ 1046.502,
  /*C♯6/D♭6*/ 1108.731,
  /*D6*/ 1174.659,
  /*D♯6/E♭6*/ 1244.508,
  /*E6*/ 1318.51,
  /*F6*/ 1396.913,
  /*F♯6/G♭6*/ 1479.978,
  /*G6*/ 1567.982,
  /*G♯6/A♭6*/ 1661.219,
  /*A6*/ 1760.f,
  /*A♯6/B♭6*/ 1864.655,
  /*B6*/ 1975.533,
  /*C7*/ 2093.005,
  /*C♯7/D♭7*/ 2217.461,
  /*D7*/ 2349.318,
  /*D♯7/E♭7*/ 2489.016,
  /*E7*/ 2637.02,
  /*F7*/ 2793.826,
  /*F♯7/G♭7*/ 2959.955,
  /*G7*/ 3135.963,
  /*G♯7/A♭7*/ 3322.438,
  /*A7*/ 3520.f,
  /*A♯7/B♭7*/ 3729.31,
  /*B7*/ 3951.066,
  /*C8*/ 4186.009};

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
      cachedRawPitch{0},
      lastPlayedPitchAmount{0},
      lastPlayedFilterAmount{0},
      previousAlgorithm{0},
      previousClockState{false},
      previousScale{0},
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

  float getOscillatorFrequency() {
    // TODO: the curve still needs work.

    int scale = state.scale.getScaled();

    float rawValue = playedPitchChanged || !scale ? state.pitch.getScaled() : cachedRawPitch;
    // if the pitch is not quantized, then the pitch always changes, otherwise
    // only when we get to a played step
    playedPitchChanged = scale ? false : true;
    cachedRawPitch = rawValue;

    float note = 76.f * rawValue;
    int noteIndex = (int)note;
    // printf("%d\n", noteIndex);
    float noteFraction = note - noteIndex;
    float value = notes[noteIndex];


    if (!scale) {
      value = value + (notes[noteIndex + 1] - notes[noteIndex]) * noteFraction;
    }

    // scale up up to 1 octave
    // TODO: 2 might be nice?
    float rawAmount = state.pitchAmount.getScaled() * lastPlayedPitchAmount;
    // printf("%f\n", multiplier);

    if (scale) {
      int* scaleNotes;
      int numScaleNotes;

      switch (scale) {
        case SCALE_MAJOR:
          if (scale != previousScale) {
            printf("scale changed to major\n");
          }
          scaleNotes = majorOffsets;
          numScaleNotes = sizeof(majorOffsets) / sizeof(int);
          break;
        case SCALE_NATURAL_MINOR:
          if (scale != previousScale) {
            printf("scale changed to natural minor\n");
          }
          scaleNotes = naturalMinorOffsets;
          numScaleNotes = sizeof(naturalMinorOffsets) / sizeof(int);
          break;
        case SCALE_HARMONIC_MINOR:
          if (scale != previousScale) {
            printf("scale changed to harmonic minor\n");
          }
          scaleNotes = harmonicMinorOffsets;
          numScaleNotes = sizeof(harmonicMinorOffsets) / sizeof(int);
          break;
        case SCALE_PENTATONIC_MAJOR:
          if (scale != previousScale) {
            printf("scale changed to pentatonic major\n");
          }
          scaleNotes = pentatonicMajorOffsets;
          numScaleNotes = sizeof(pentatonicMajorOffsets) / sizeof(int);
          break;
        case SCALE_PENTATONIC_MINOR:
          if (scale != previousScale) {
            printf("scale changed to pentatonic minor\n");
          }
          scaleNotes = pentatonicMinorOffsets;
          numScaleNotes = sizeof(pentatonicMinorOffsets) / sizeof(int);
          break;

        default:
          if (scale != previousScale) {
            printf("scale changed to chromatic\n");
          }

          scaleNotes = chromaticOffsets;
          numScaleNotes = sizeof(chromaticOffsets) / sizeof(int);
          break;
      }

      int offset = (int)(rawAmount * numScaleNotes);
      // printf("%d\n", offset);
      value = notes[noteIndex + scaleNotes[offset]];
    } else {
      if (scale != previousScale) {
        printf("scale changed to unquantized\n");
      }
      value += value * rawAmount;
    }

    previousScale = scale;
    return value;
  }

  float getFilterCutoff() {
    // I just prefer when the cutoff does not track the pitch. If you track the
    // pitch, then pitch dominates the sequence. If you don't, then filter
    // cutoff tends to dominate. And this isn't really so much a pitch
    // sequencer? Sequences tend to be more interesting that way, although
    // that's subjective.
    float value = state.cutoff.getScaled();

    // Apply a curve to the cutoff amount so it starts to make a difference
    // without having to turn the knob all the way up.
    float rawFilterAmount = powf(state.cutoffAmount.value * lastPlayedFilterAmount, 0.5f);
    value += state.cutoff.scaleValue(rawFilterAmount);

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

      case ALGORITHM_RAMP_UP: {
        // 1. ramp up
        std::sort(std::begin(state.pitchAmounts), std::end(state.pitchAmounts));
        break;
      }

      case ALGORITHM_RAMP_DOWN: {
        // 2. ramp down
        std::sort(std::begin(state.pitchAmounts), std::end(state.pitchAmounts), std::greater{});
        break;
      }

      case ALGORITHM_TRANGLE_UP: {
        // 3. triangle up
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::greater{});
        break;
      }

      case ALGORITHM_TRIANGLE_DOWN: {
        // 4. triangle down
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::less{});
        break;
      }

      case ALGORITHM_TWO_TRIANGLES_UP: {
        // 5. two triangles up
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 8,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 24,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 24,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::greater{});
        break;
      }

      case ALGORITHM_TWO_TRIANGLES_DOWN: {
        // 6. two triangles down
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 8,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 24,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 24,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::less{});
        break;
      }

      case ALGORITHM_FOUR_TRIANGLES_UP: {
        // 7. four triangles up
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 4,
                          std::begin(state.pitchAmounts) + 4,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 4,
                          std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 8,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 12,
                          std::begin(state.pitchAmounts) + 12,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 12,
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 20,
                          std::begin(state.pitchAmounts) + 20,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 20,
                          std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 24,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 28,
                          std::begin(state.pitchAmounts) + 28,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 28,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::greater{});
        break;
      }

      case ALGORITHM_FOUR_TRIANGLES_DOWN: {
        // 8. four triangles down
        std::partial_sort(std::begin(state.pitchAmounts),
                          std::begin(state.pitchAmounts) + 4,
                          std::begin(state.pitchAmounts) + 4,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 4,
                          std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 8,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 8,
                          std::begin(state.pitchAmounts) + 12,
                          std::begin(state.pitchAmounts) + 12,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 12,
                          std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 16,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 16,
                          std::begin(state.pitchAmounts) + 20,
                          std::begin(state.pitchAmounts) + 20,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 20,
                          std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 24,
                          std::less{});
        std::partial_sort(std::begin(state.pitchAmounts) + 24,
                          std::begin(state.pitchAmounts) + 28,
                          std::begin(state.pitchAmounts) + 28,
                          std::greater{});
        std::partial_sort(std::begin(state.pitchAmounts) + 28,
                          std::end(state.pitchAmounts),
                          std::end(state.pitchAmounts),
                          std::less{});
        break;
      }

      case ALGORITHM_RAMP_UP_DOWN: {
        // 9. ramp up down
        float sorted[32];
        std::copy(
          std::begin(state.pitchAmountsBackup), std::end(state.pitchAmountsBackup), std::begin(sorted));
        std::sort(std::begin(sorted), std::end(sorted), std::less{});

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
        // 10. ramp down up
        float sorted[32];
        std::copy(
          std::begin(state.pitchAmountsBackup), std::end(state.pitchAmountsBackup), std::begin(sorted));
        std::sort(std::begin(sorted), std::end(sorted), std::greater{});

        for (int i = 0; i < 32; i++) {
          if (i % 2 == 0) {
            state.pitchAmounts[i] = sorted[i / 2];
          } else {
            state.pitchAmounts[i] = sorted[16 + (i / 2)];
          }
        }
        break;
      }

      case ALGORITHM_TRIANGLE_UP_DOWN: {
        // 11. triangle up down
        float sorted[32];
        std::copy(
          std::begin(state.pitchAmountsBackup), std::end(state.pitchAmountsBackup), std::begin(sorted));

        std::partial_sort(std::begin(sorted),
                          std::begin(sorted) + 16,
                          std::begin(sorted) + 16,
                          std::less{});
        std::partial_sort(std::begin(sorted) + 16,
                          std::end(sorted),
                          std::end(sorted),
                          std::greater{});

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
        // 12. triangle down up
        float sorted[32];
        std::copy(
          std::begin(state.pitchAmountsBackup), std::end(state.pitchAmountsBackup), std::begin(sorted));

        std::partial_sort(std::begin(sorted),
                          std::begin(sorted) + 16,
                          std::begin(sorted) + 16,
                          std::greater{});
        std::partial_sort(std::begin(sorted) + 16,
                          std::end(sorted),
                          std::end(sorted),
                          std::less{});

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
            printf("clockTicks: %d, divider: %.2f\n", externalClockTicks, divider);

            if (externalClockTicks % (int)divider == 0) {
              if (clock.getPhase() > 0.5f) {
                // Make the clock tick now because the external clock is faster than
                // our calculations and we'd miss a tick if we don't. If the phase
                // is < 0.5 then we assume the click is slower than our calculations
                // and we don't tick again because we'd tick twice in quick succession.

                // TODO: you can only do this trick if the clock tick counts are a multiple of each other
                tick = true;
              }
              // TODO: you can only do this trick if the clock tick counts are a multiple of each other
              clock.reset();
            }
          } else {
            if (clock.getPhase() > 0.5f) {
              // Make the clock tick now because the external clock is faster than
              // our calculations and we'd miss a tick if we don't. If the phase
              // is < 0.5 then we assume the click is slower than our calculations
              // and we don't tick again because we'd tick twice in quick succession.

              // TODO: you can only do this trick if the clock tick counts are a multiple of each other
              tick = true;
            }
            // TODO: you can only do this trick if the clock tick counts are a multiple of each other
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
    // TODO: 0, 2, 4, 8, 10, 16, 32?
    uint stepCount = state.stepCount.getScaled();

    // 0 to 1
    float volumeEnv = state.volumeEnvelope.getScaled();
    float cutoffEnv = state.cutoffEnvelope.getScaled();

    // doing this before we might trigger the envelopes to make sure that the initial direction is set properly
    volumeEnvelope.setTimeAndDirection(volumeEnv);
    cutoffEnvelope.setTimeAndDirection(cutoffEnv);

    if (isClockTick()) {
      clockTicks++;
      printf("minSample: %.2f, maxSample: %.2f\n", minSample, maxSample);
      minSample = 0;
      maxSample = 0;

      // TODO: just change this to the number of ticks. we can mod 2.
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

        float evolve = state.evolve.getScaled();
        float evolveAbs = fabs(state.evolve.getScaled());
        // add a dead zone around the middle of the knob,
        // only evolve if the random probability is greater than the current
        // absolute evolve value
        bool evolved = false;
        if (evolveAbs > 0.1f && evolveAbs/4.f > randomProb()) {
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
    float frequency = getOscillatorFrequency();// * maybeAttackDecay(pitchEnv, pitchEnvelope.process());
    if (frequency > 28.f) {
      oscillator.setFreq(frequency);
      sample = oscillator.process(); // -0.5 to 0.5
    }

    // noise
    float noise = (randomProb() * 2.f - 1.f) * state.noise.getScaled();
    sample += noise;

    // filter
    float cutoff = getFilterCutoff() * maybeAttackDecay(cutoffEnv, cutoffEnvelope.process());
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
  float cachedRawPitch;
  // TODO: also cache the filter, pitch and volume?
  float lastPlayedPitchAmount;
  float lastPlayedFilterAmount;
  int previousAlgorithm;
  bool previousClockState;
  int previousScale;
  ButtonInput& bootButton;
  SDSState state;
  SDSController controller;
  Metro clock;
  AttackDecayEnvelope volumeEnvelope;
  AttackDecayEnvelope cutoffEnvelope;
  Oscillator oscillator;
  LadderFilter filter;

  float minSample;
  float maxSample;
};

}  // namespace platform

#endif  // PLATFORM_STEP_INSTRUMENT_H