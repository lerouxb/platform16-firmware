#ifndef PLATFORM_VARIABLESAWOSC_H
#define PLATFORM_VARIABLESAWOSC_H

#include <cmath>
#include "utils.hpp"


namespace platform {

/**
Saw with variable slope or notch.
Ported from DaisySP's port by Ben Sergentanis
Which was in turn Ported from pichenettes/eurorack/plaits/dsp/oscillator/variable_saw_oscillator.h
Original code written by Emilie Gillet in 2016. \n
*/
class VariableSawOscillator {
  public:
  VariableSawOscillator() {}
  ~VariableSawOscillator() {}

  void init(float sampleRateIn) {
    sampleRate = sampleRateIn;

    phase = 0.0f;
    nextSample = 0.0f;
    previousPW = 0.5f;
    high = false;

    setFreq(220.f);
    setPW(0.f);
    setWaveshape(1.f);
  }

  /** Get the next sample */
  float process() {
    float thisSample = nextSample;
    nextSample = 0.0f;

    const float triangleAmount = waveshape;
    const float notchAmount = 1.0f - waveshape;
    const float slopeUp = 1.0f / (pw);
    const float slopeDown = 1.0f / (1.0f - pw);

    phase += frequency;

    if (!high && phase >= pw) {
      const float triangleStep = (slopeUp + slopeDown) * frequency * triangleAmount;
      const float notch = (variableSawNotchDepth + 1.0f - pw) * notchAmount;
      const float t = (phase - pw) / (previousPW - pw + frequency);
      thisSample += notch * thisBlepSample(t);
      nextSample += notch * nextBlepSample(t);
      thisSample -= triangleStep * thisIntegratedBlepSample(t);
      nextSample -= triangleStep * nextIntegratedBlepSample(t);
      high = true;
    } else if (phase >= 1.0f) {
      phase -= 1.0f;
      const float triangleStep = (slopeUp + slopeDown) * frequency * triangleAmount;
      const float notch = (variableSawNotchDepth + 1.0f) * notchAmount;
      const float t = phase / frequency;
      thisSample -= notch * thisBlepSample(t);
      nextSample -= notch * nextBlepSample(t);
      thisSample += triangleStep * thisIntegratedBlepSample(t);
      nextSample += triangleStep * nextIntegratedBlepSample(t);
      high = false;
    }

    nextSample += computeNaiveSample(phase, pw, slopeUp, slopeDown, triangleAmount, notchAmount);
    previousPW = pw;

    return (2.0f * thisSample - 1.0f) / (1.0f + variableSawNotchDepth);
  }

  /** Set master freq.
      \param frequency Freq in Hz.
  */
  void setFreq(float frequencyIn) {
    // operate on a temporary var because process() uses frequency and could be
    // called at any moment
    float newFrequency = frequencyIn / sampleRate;
    newFrequency = newFrequency >= .25f ? .25f : newFrequency;
    pw = newFrequency >= .25f ? .5f : pw;
    frequency = newFrequency;
  }

  /** Adjust the wave depending on the shape
      \param pw Notch or slope. Works best -1 to 1.
  */
  void setPW(float pwIn) {
    if (frequency >= .25f) {
      pw = .5f;
    } else {
      pw = fclamp(pwIn, frequency * 2.0f, 1.0f - 2.0f * frequency);
    }
  }

  /** Slope or notch
      \param waveshape 0 = notch, 1 = slope
  */
  void setWaveshape(float waveshapeIn) {
    waveshape = waveshapeIn;
  };


  private:
  float computeNaiveSample(float phaseIn,
                           float pwIn,
                           float slopeUp,
                           float slopeDown,
                           float triangleAmount,
                           float notchAmount) {
    float notchSaw = phaseIn < pwIn ? phaseIn : 1.0f + variableSawNotchDepth;
    float triangle = phaseIn < pwIn ? phaseIn * slopeUp : 1.0f - (phaseIn - pwIn) * slopeDown;
    return notchSaw * notchAmount + triangle * triangleAmount;
  }

  float sampleRate;

  // Oscillator state.
  float phase;
  float nextSample;
  float previousPW;
  bool high;

  const float variableSawNotchDepth = 0.2f;

  // For interpolation of parameters.
  float frequency;
  float pw;
  float waveshape;
};

}  // namespace platform

#endif  // PLATFORM_VARIABLESAWOSC_H