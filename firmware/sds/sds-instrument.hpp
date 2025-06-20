#ifndef PLATFORM_STEP_INSTRUMENT_H
#define PLATFORM_STEP_INSTRUMENT_H

#include "../../lib/buttons.hpp"
#include "../../lib/decay.hpp"
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
// #define ALGORITHM_RANDOM_SQUARE_LOW 7
// #define ALGORITHM_RANDOM_SQUARE_HIGH 8
#define ALGORITHM_FOUR_TRIANGLES_UP 7
#define ALGORITHM_FOUR_TRIANGLES_DOWN 8
#define ALGORITHM_RAMP_UP_DOWN 9
#define ALGORITHM_RAMP_DOWN_UP 10
#define ALGORITHM_TRIANGLE_UP_DOWN 11
#define ALGORITHM_TRIANGLE_DOWN_UP 12

namespace platform {

float safeDecayTime(float decayTime) {
  // 1/48000 (the sample rate) = 0.000020833333333333333
  // 1/24000 = 0.000041666666666666665
  return decayTime == 0 ? 0.000041666666666666665 : decayTime;
}

float maybeDecay(float decay, float value) {
  return decay > 9.8 ? 1.f : value;
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
      playedPitchChanged{true},
      cachedRawPitch{0},
      lastPlayedAmount{0},
      previousAlgorithm{0},
      previousClockState{false},
      previousScale{0},
      clockTickOffset{0},
      minSample{0},
      maxSample{0}
      {};

  void init(float sampleRateIn) {
    sampleRate = sampleRateIn;
    oscillator.init(sampleRate);
    oscillator.setAmp(1.f);
    //oscillator.setWaveform(Oscillator::WAVE_SAW);
    //oscillator.setWaveform(Oscillator::WAVE_POLYBLEP_SQUARE);
    oscillator.setWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    clock.init(getTickFrequency(), sampleRate);

    volumeEnvelope.init(sampleRate);
    pitchEnvelope.init(sampleRate);
    cutoffEnvelope.init(sampleRate);

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
    float value = state.bpm.getScaled();
    if (value < 0.005) {
      // if it is close to zero, then just stop the clock
      return 0.f;
    }
    return value / 60.f * 4.f;  // 16th notes, not quarter notes
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

    value += state.volumeAmount.getScaled() * lastPlayedAmount;

    //value = fclamp(value, 0.f, 1.f);
    
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
    float rawAmount = state.pitchAmount.getScaled() * lastPlayedAmount;
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
    float value = state.cutoff.value;

    value += state.cutoffAmount.value * lastPlayedAmount;
    value = fclamp(value, 0.f, 1.f);

    float max = HALF_SAMPLE_RATE;
    float min = 5.f;
    value = powf(value, 3.0f) * (max - min) + min;

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
      state.amounts[i] = state.amountsBackup[i];
    }

    switch (state.algorithm.getScaled()) {
      case ALGORITHM_NONE:
        break;

      case ALGORITHM_RAMP_UP: {
        // 1. ramp up
        std::sort(std::begin(state.amounts), std::end(state.amounts));
        break;
      }

      case ALGORITHM_RAMP_DOWN: {
        // 2. ramp down
        std::sort(std::begin(state.amounts), std::end(state.amounts), std::greater{});
        break;
      }

      case ALGORITHM_TRANGLE_UP: {
        // 3. triangle up
        std::partial_sort(std::begin(state.amounts),
                          std::begin(state.amounts) + 16,
                          std::begin(state.amounts) + 16,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 16,
                          std::end(state.amounts),
                          std::end(state.amounts),
                          std::greater{});
        break;
      }

      case ALGORITHM_TRIANGLE_DOWN: {
        // 4. triangle down
        std::partial_sort(std::begin(state.amounts),
                          std::begin(state.amounts) + 16,
                          std::begin(state.amounts) + 16,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 16,
                          std::end(state.amounts),
                          std::end(state.amounts),
                          std::less{});
        break;
      }

      case ALGORITHM_TWO_TRIANGLES_UP: {
        // 5. two triangles up
        std::partial_sort(std::begin(state.amounts),
                          std::begin(state.amounts) + 8,
                          std::begin(state.amounts) + 8,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 8,
                          std::begin(state.amounts) + 16,
                          std::begin(state.amounts) + 16,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 16,
                          std::begin(state.amounts) + 24,
                          std::begin(state.amounts) + 24,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 24,
                          std::end(state.amounts),
                          std::end(state.amounts),
                          std::greater{});
        break;
      }

      case ALGORITHM_TWO_TRIANGLES_DOWN: {
        // 6. two triangles down
        std::partial_sort(std::begin(state.amounts),
                          std::begin(state.amounts) + 8,
                          std::begin(state.amounts) + 8,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 8,
                          std::begin(state.amounts) + 16,
                          std::begin(state.amounts) + 16,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 16,
                          std::begin(state.amounts) + 24,
                          std::begin(state.amounts) + 24,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 24,
                          std::end(state.amounts),
                          std::end(state.amounts),
                          std::less{});
        break;
      }

      /*
      case ALGORITHM_RANDOM_SQUARE_LOW:
      {
        // 7. random square low high
        std::sort(std::begin(state.amounts), std::end(state.amounts));
        std::shuffle(std::begin(state.amounts), std::begin(state.amounts)+16, g);
        std::shuffle(std::begin(state.amounts)+16, std::end(state.amounts), g);
        break;
      }

      case ALGORITHM_RANDOM_SQUARE_HIGH:
      {
        // 8. random square high low
        std::sort(std::begin(state.amounts), std::end(state.amounts), std::greater{});
        std::shuffle(std::begin(state.amounts), std::begin(state.amounts)+16, g);
        std::shuffle(std::begin(state.amounts)+16, std::end(state.amounts), g);
        break;
      }
      */
      case ALGORITHM_FOUR_TRIANGLES_UP: {
        // 7. four triangles up
        std::partial_sort(std::begin(state.amounts),
                          std::begin(state.amounts) + 4,
                          std::begin(state.amounts) + 4,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 4,
                          std::begin(state.amounts) + 8,
                          std::begin(state.amounts) + 8,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 8,
                          std::begin(state.amounts) + 12,
                          std::begin(state.amounts) + 12,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 12,
                          std::begin(state.amounts) + 16,
                          std::begin(state.amounts) + 16,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 16,
                          std::begin(state.amounts) + 20,
                          std::begin(state.amounts) + 20,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 20,
                          std::begin(state.amounts) + 24,
                          std::begin(state.amounts) + 24,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 24,
                          std::begin(state.amounts) + 28,
                          std::begin(state.amounts) + 28,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 28,
                          std::end(state.amounts),
                          std::end(state.amounts),
                          std::greater{});
        break;
      }

      case ALGORITHM_FOUR_TRIANGLES_DOWN: {
        // 8. four triangles down
        std::partial_sort(std::begin(state.amounts),
                          std::begin(state.amounts) + 4,
                          std::begin(state.amounts) + 4,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 4,
                          std::begin(state.amounts) + 8,
                          std::begin(state.amounts) + 8,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 8,
                          std::begin(state.amounts) + 12,
                          std::begin(state.amounts) + 12,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 12,
                          std::begin(state.amounts) + 16,
                          std::begin(state.amounts) + 16,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 16,
                          std::begin(state.amounts) + 20,
                          std::begin(state.amounts) + 20,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 20,
                          std::begin(state.amounts) + 24,
                          std::begin(state.amounts) + 24,
                          std::less{});
        std::partial_sort(std::begin(state.amounts) + 24,
                          std::begin(state.amounts) + 28,
                          std::begin(state.amounts) + 28,
                          std::greater{});
        std::partial_sort(std::begin(state.amounts) + 28,
                          std::end(state.amounts),
                          std::end(state.amounts),
                          std::less{});
        break;
      }

      case ALGORITHM_RAMP_UP_DOWN: {
        // 9. ramp up down
        float sorted[32];
        std::copy(
          std::begin(state.amountsBackup), std::end(state.amountsBackup), std::begin(sorted));
        std::sort(std::begin(sorted), std::end(sorted), std::less{});

        for (int i = 0; i < 32; i++) {
          if (i % 2 == 0) {
            state.amounts[i] = sorted[i / 2];
          } else {
            state.amounts[i] = sorted[16 + (i / 2)];
          }
        }
        break;
      }


      case ALGORITHM_RAMP_DOWN_UP: {
        // 10. ramp down up
        float sorted[32];
        std::copy(
          std::begin(state.amountsBackup), std::end(state.amountsBackup), std::begin(sorted));
        std::sort(std::begin(sorted), std::end(sorted), std::greater{});

        for (int i = 0; i < 32; i++) {
          if (i % 2 == 0) {
            state.amounts[i] = sorted[i / 2];
          } else {
            state.amounts[i] = sorted[16 + (i / 2)];
          }
        }
        break;
      }

      case ALGORITHM_TRIANGLE_UP_DOWN: {
        // 11. triangle up down
        float sorted[32];
        std::copy(
          std::begin(state.amountsBackup), std::end(state.amountsBackup), std::begin(sorted));

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
            state.amounts[i] = sorted[i / 2];
          } else {
            state.amounts[i] = sorted[16 + (i / 2)];
          }
        }
        break;
      }


      case ALGORITHM_TRIANGLE_DOWN_UP: {
        // 12. triangle down up
        float sorted[32];
        std::copy(
          std::begin(state.amountsBackup), std::end(state.amountsBackup), std::begin(sorted));

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
            state.amounts[i] = sorted[i / 2];
          } else {
            state.amounts[i] = sorted[16 + (i / 2)];
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
      state.amounts[i] = randomProb();
      state.amountsBackup[i] = state.amounts[i];
    }

    if (state.stepCount.getScaled() != 0) {
      // always play the down beat, otherwise when you shorten stepCount a sequence might sound off
      state.steps[0] = 1.f;
    }

    sortByAlgorithm();

    // TODO: space out the skips?


    // for (int i=0; i < 32; i++) {
    //   printf("%.2f ", state.amounts[i]);
    // }
    // printf("\n");
  }

  float process() {
    // TODO: 0, 2, 4, 8, 10, 16, 32?
    uint stepCount = state.stepCount.getScaled();


    float volumeDecay = state.volumeDecay.getScaled();
    float pitchDecay = state.pitchDecay.getScaled();
    float cutoffDecay = state.cutoffDecay.getScaled();

    // the pin is inverted because it is tied to an NPN transistor
    bool clockState = !gpio_get(CLOCK_IN_PIN);

    if (clock.process() || bootButton.isPressed ||
        (clockState && clockState != previousClockState)) {

      //printf("minSample: %.2f, maxSample: %.2f\n", minSample, maxSample);
      minSample = 0;
      maxSample = 0;

      if (clockTickOffset == 0) {
        // the pin is inverted because it is tied to an NPN transistor
        gpio_put(CLOCK_OUT_PIN, false);
      } else {
        gpio_put(CLOCK_OUT_PIN, true);
      }
      clockTickOffset++;
      if (clockTickOffset == 2) {
        clockTickOffset = 0;
      }

      if (stepCount == 0) {
        randomizeSequence();
      }

      if (isPlayedStep()) {
        // recalculate volume, frequency and cutoff, the steps..
        playedPitchChanged = true;
        lastPlayedAmount = state.amounts[state.step];

        // printf("%.2f\n", sampleRate);
        float evolve = state.evolve.getScaled();
        float evolveAbs = fabs(state.evolve.getScaled());
        // add a dead zone around the middle of the knob,
        // only evolve if the random probability is greater than the current
        // absolute evolve value
        bool evolved = false;
        if (evolveAbs > 0.1f && evolveAbs/4.f > randomProb()) {
          evolved = true;
          if (evolve > 0.f) {
            state.amountsBackup[state.step] = randomProb();
            //printf("evolve amount %d %.2f\n", state.step, state.amounts[state.step]);
          }
          else {
            state.steps[state.step] = randomProb();
            //printf("evolve step %d %.2f\n", state.step, state.steps[state.step]);
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
        pitchEnvelope.trigger();
        cutoffEnvelope.trigger();
      }

      state.step++;

      if (state.step >= stepCount) {
        state.step = 0;
      }
    }


    previousClockState = clockState;

    clock.setFreq(getTickFrequency());
    filter.setRes(state.resonance.getScaled() * 1.8f);

    volumeEnvelope.setDecayTime(safeDecayTime(volumeDecay));
    pitchEnvelope.setDecayTime(safeDecayTime(pitchDecay));
    cutoffEnvelope.setDecayTime(safeDecayTime(cutoffDecay));

    float frequency = getOscillatorFrequency() * maybeDecay(pitchDecay, pitchEnvelope.process());
    oscillator.setFreq(frequency);
    float cutoff = getFilterCutoff() * maybeDecay(cutoffDecay, cutoffEnvelope.process());
    filter.setFreq(fmax(5.f, cutoff));

    float sample = oscillator.process(); // -0.5 to 0.5

    // apply the filter
    sample = filter.process(sample);

    // overdrive
    //sample = softClip(sample * state.drive.getPreGain()) * state.drive.getPostGain();

    // volume
    // sample = sample * state.volume.getScaled();
    sample = sample * getVolume() * maybeDecay(volumeDecay, volumeEnvelope.process());

    minSample = std::min(minSample, sample);
    maxSample = std::max(maxSample, sample);

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
  float sampleRate;
  bool playedPitchChanged;
  float cachedRawPitch;
  float lastPlayedAmount;
  int previousAlgorithm;
  bool previousClockState;
  int previousScale;
  int clockTimeout;
  int clockTickOffset;
  ButtonInput& bootButton;
  SDSState state;
  SDSController controller;
  Metro clock;
  DecayEnvelope volumeEnvelope;
  DecayEnvelope pitchEnvelope;
  DecayEnvelope cutoffEnvelope;
  Oscillator oscillator;
  LadderFilter filter;

  float minSample;
  float maxSample;
};

}  // namespace platform

#endif  // PLATFORM_STEP_INSTRUMENT_H