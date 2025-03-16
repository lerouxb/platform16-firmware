/*
Ported from daisysp, Copyright (c) 2020 Electrosmith, Corp

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/

#ifndef PLATFORM_OSCILLATOR_H
#define PLATFORM_OSCILLATOR_H
#include "utils.hpp"
#include <stdint.h>

namespace platform {

static float polyblep(float phaseInc, float t) {
  float dt = phaseInc;
  if (t < dt) {
    t /= dt;
    return t + t - t * t - 1.0f;
  } else if (t > 1.0f - dt) {
    t = (t - 1.0f) / dt;
    return t * t + t + t + 1.0f;
  } else {
    return 0.0f;
  }
}

/** Synthesis of several waveforms, including polyBLEP bandlimited waveforms.
 */
class Oscillator {
  public:
  Oscillator() {}
  ~Oscillator() {}
  /** Choices for output waveforms, POLYBLEP are appropriately labeled. Others are naive forms.
   */
  enum {
    WAVE_SIN,
    WAVE_TRI,
    WAVE_SAW,
    WAVE_RAMP,
    WAVE_SQUARE,
    WAVE_POLYBLEP_TRI,
    WAVE_POLYBLEP_SAW,
    WAVE_POLYBLEP_SQUARE,
    WAVE_LAST,
  };


  /** Initializes the Oscillator

      \param sample_rate - sample rate of the audio engine being run, and the frequency that the
     Process function will be called.

      Defaults:
      - freq_ = 100 Hz
      - amp_ = 0.5
      - waveform_ = sine wave.
  */
  void init(float sampleRate) {
    sr = sampleRate;
    srRecip = 1.0f / sampleRate;
    freq = 100.0f;
    amp = 0.5f;
    pw = 0.5f;
    phase = 0.0f;
    phaseInc = calcPhaseInc(freq);
    waveform = WAVE_SIN;
    eoc = true;
    eor = true;
  }

  /** Changes the frequency of the Oscillator, and recalculates phase increment.
   */
  inline void setFreq(const float f) {
    freq = f;
    phaseInc = calcPhaseInc(f);
  }

  /** Sets the amplitude of the waveform.
   */
  inline void setAmp(const float a) {
    amp = a;
  }

  /** Sets the waveform to be synthesized by the Process() function.
   */
  inline void setWaveform(const uint8_t wf) {
    waveform = wf < WAVE_LAST ? wf : WAVE_SIN;
  }

  /** Sets the pulse width for WAVE_SQUARE and WAVE_POLYBLEP_SQUARE (range 0 - 1)
   */
  inline void setPw(const float pwIn) {
    pw = fclamp(pwIn, 0.0f, 1.0f);
  }

  /** Returns true if cycle is at end of rise. Set during call to Process.
   */
  inline bool isEOR() {
    return eor;
  }

  /** Returns true if cycle is at end of cycle. Set during call to Process.
   */
  inline bool isEOC() {
    return eoc;
  }

  /** Returns true if cycle rising.
   */
  inline bool isRising() {
    return phase < 0.5f;
  }

  /** Returns true if cycle falling.
   */
  inline bool isFalling() {
    return phase >= 0.5f;
  }

  /** Processes the waveform to be generated, returning one sample. This should be called once per
   * sample period.
   */
  float process() {
    float out, t;
    switch (waveform) {
      case WAVE_SIN:
        out = sinf(phase * TWOPI_F);
        break;
      case WAVE_TRI:
        t = -1.0f + (2.0f * phase);
        out = 2.0f * (fabsf(t) - 0.5f);
        break;
      case WAVE_SAW:
        out = -1.0f * (((phase * 2.0f)) - 1.0f);
        break;
      case WAVE_RAMP:
        out = ((phase * 2.0f)) - 1.0f;
        break;
      case WAVE_SQUARE:
        out = phase < pw ? (1.0f) : -1.0f;
        break;
      case WAVE_POLYBLEP_TRI:
        t = phase;
        out = phase < 0.5f ? 1.0f : -1.0f;
        out += polyblep(phaseInc, t);
        out -= polyblep(phaseInc, fastmod1f(t + 0.5f));
        // Leaky Integrator:
        // y[n] = A + x[n] + (1 - A) * y[n-1]
        out = phaseInc * out + (1.0f - phaseInc) * lastOut;
        lastOut = out;
        out *= 4.f;  // normalize amplitude after leaky integration
        break;
      case WAVE_POLYBLEP_SAW:
        t = phase;
        out = (2.0f * t) - 1.0f;
        out -= polyblep(phaseInc, t);
        out *= -1.0f;
        break;
      case WAVE_POLYBLEP_SQUARE:
        t = phase;
        out = phase < pw ? 1.0f : -1.0f;
        out += polyblep(phaseInc, t);
        out -= polyblep(phaseInc, fastmod1f(t + (1.0f - pw)));
        out *= 0.707f;  // ?
        break;
      default:
        out = 0.0f;
        break;
    }
    phase += phaseInc;
    if (phase > 1.0f) {
      phase -= 1.0f;
      eoc = true;
    } else {
      eoc = false;
    }
    eor = (phase - phaseInc < 0.5f && phase >= 0.5f);

    return out * amp;
  }


  /** Adds a value 0.0-1.0 (equivalent to 0.0-TWO_PI) to the current phase. Useful for PM and "FM"
   * synthesis.
   */
  void phaseAdd(float phaseIn) {
    phase += phaseIn;
  }
  /** Resets the phase to the input argument. If no argumeNt is present, it will reset phase to 0.0;
   */
  void reset(float phaseIn = 0.0f) {
    phase = phaseIn;
  }

  inline float calcPhaseInc(float f) {
    return f * srRecip;
  }

  private:
  uint8_t waveform;
  float amp, freq, pw;
  float sr, srRecip, phase, phaseInc;
  float lastOut, lastFreq;
  bool eor, eoc;
};
}  // namespace platform
#endif