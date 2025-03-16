#ifndef PLATFORM_LADDER_H
#define PLATFORM_LADDER_H

#include <array>
#include <cmath>

#include "utils.hpp"

namespace platform {

static inline float fast_tanh(float x) {
  if (x > 3.0f)
    return 1.0f;
  if (x < -3.0f)
    return -1.0f;
  float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
//-----------------------------------------------------------
// Ported from daisysp:
// Huovilainen New Moog (HNM) model as per CMJ jun 2006
// Richard van Hoesel, v. 1.03, Feb. 14 2021
// v1.7 (Infrasonic/Daisy) add configurable filter mode
// v1.6 (Infrasonic/Daisy) removes polyphase FIR, uses 4x linear
//      oversampling for performance reasons
// v1.5 adds polyphase FIR or Linear interpolation
// v1.4 FC extended to 18.7kHz, max res to 1.8, 4x oversampling,
//      and a minor Q-tuning adjustment
// v.1.03 adds oversampling, extended resonance,
// and exposes parameters input_drive and passband_gain
// v.1.02 now includes both cutoff and resonance "CV" modulation inputs
// please retain this header if you use this code.
//-----------------------------------------------------------

/**
 * 4-pole ladder filter model with selectable filter type (LP/BP/HP 12 or 24 dB/oct),
 * drive, passband gain compensation, and stable self-oscillation.
 *
 *
 */
class LadderFilter {
  public:
  enum class FilterMode { LP24, LP12, BP24, BP12, HP24, HP12 };

  LadderFilter() = default;
  ~LadderFilter() = default;

  /** Initializes the ladder filter module.
   */
  void init(float sampleRateIn) {
    sampleRate = sampleRateIn;
    srIntRecip = 1.0f / (sampleRate * interpolation);
    alpha = 1.0f;
    K = 1.0f;
    Fbase = 1000.0f;
    Qadjust = 1.0f;
    oldinput = 0.f;
    mode = FilterMode::LP24;

    setPassbandGain(0.5f);
    setInputDrive(0.5f);
    setFreq(5000.f);
    setRes(0.2f);
  };

  /** Process single sample */
  float process(float in) {
    float input = in * driveScaled;
    float total = 0.0f;
    float interp = 0.0f;
    for (size_t os = 0; os < interpolation; os++) {
      float in_interp = (interp * oldinput + (1.0f - interp) * input);
      float u = in_interp - (z1[3] - pbg * in_interp) * K * Qadjust;
      u = fast_tanh(u);
      float stage1 = LPF(u, 0);
      float stage2 = LPF(stage1, 1);
      float stage3 = LPF(stage2, 2);
      float stage4 = LPF(stage3, 3);
      total += weightedSumForCurrentMode({u, stage1, stage2, stage3, stage4}) * interpolationRecip;
      interp += interpolationRecip;
    }
    oldinput = input;
    return total;
  }

  /** Process mono buffer/block of samples in place */
  __attribute__((optimize("unroll-loops"))) void processBlock(float* buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
      buf[i] = process(buf[i]);
    }
  }

  /**
      Sets the cutoff frequency of the filter.
      Units of hz, valid in range 5 - ~nyquist (samp_rate / 2)
      Internally clamped to this range.
  */
  void setFreq(float freq) {
    Fbase = freq;
    computeCoeffs(freq);
  }

  /**
      Sets the resonance of the filter.
      Filter will stably self oscillate at higher values.
      Valid in range 0 - 1.8
      Internally clamped to this range.
  */
  void setRes(float res) {
    // maps resonance = 0->1 to K = 0 -> 4
    res = fclamp(res, 0.0f, maxResonance);
    K = 4.0f * res;
  }

  /**
      Set "passband gain" compensation factor to mitigate
      loss of energy in passband at higher resonance values.
      Drive and passband gain have a dependent relationship.
      Valid in range 0 - 0.5
      Internally clamped to this range.
   */
  void setPassbandGain(float pbg) {
    pbg = fclamp(pbg, 0.0f, 0.5f);
    setInputDrive(drive);
  }

  /**
      Sets drive of the input stage into the tanh clipper
      Valid in range 0 - 4.0
   */
  void setInputDrive(float odrv) {
    drive = fmax(odrv, 0.0f);
    if (drive > 1.0f) {
      drive = fmin(drive, 4.0f);
      // max is 4 when pbg = 0, and 2.5 when pbg is 0.5
      driveScaled = 1.0f + (drive - 1.0f) * (1.0f - pbg);
    } else {
      driveScaled = drive;
    }
  }
  float LPF(float s, int i) {
    //             (1.0 / 1.3)   (0.3 / 1.3)
    float ft = s * 0.76923077f + 0.23076923f * z0[i] - z1[i];
    ft = ft * alpha + z1[i];
    z1[i] = ft;
    z0[i] = s;
    return ft;
  }
  void computeCoeffs(float freq) {
    freq = fclamp(freq, 5.0f, sampleRate * 0.425f);
    float wc = freq * 2.0f * M_PI * srIntRecip;
    float wc2 = wc * wc;
    alpha = 0.9892f * wc - 0.4324f * wc2 + 0.1381f * wc * wc2 - 0.0202f * wc2 * wc2;
    // Qadjust = 1.0029f + 0.0526f * wc - 0.0926 * wc2 + 0.0218* wc * wc2;
    Qadjust = 1.006f + 0.0536f * wc - 0.095f * wc2 - 0.05f * wc2 * wc2;
    // revised hfQ (rvh - feb 14 2021)
  }

  float weightedSumForCurrentMode(const std::array<float, 5>& stage_outs) {
    // Weighted filter stage mixing to achieve selected response
    // as described in "Oscillator and Filter Algorithms for Virtual Analog Synthesis"
    // Välimäki and Huovilainen, Computer Music Journal, vol 60, 2006
    switch (mode) {
      case FilterMode::LP24:
        return stage_outs[4];
      case FilterMode::LP12:
        return stage_outs[2];
      case FilterMode::BP24:
        return (stage_outs[2] + stage_outs[4]) * 4.0f - stage_outs[3] * 8.0f;
      case FilterMode::BP12:
        return (stage_outs[1] - stage_outs[2]) * 2.0f;
      case FilterMode::HP24:
        return stage_outs[0] + stage_outs[4] - ((stage_outs[1] + stage_outs[3]) * 4.0f) +
          stage_outs[2] * 6.0f;
      case FilterMode::HP12:
        return stage_outs[0] + stage_outs[2] - stage_outs[1] * 2.0f;
      default:
        return 0.0f;
    }
  }

  /**
      Sets the filter mode/response
      Defaults to classic lowpass 24dB/oct
   */
  inline void setFilterMode(FilterMode modeIn) {
    mode = modeIn;
  }

  private:
  static constexpr uint8_t interpolation = 4;
  static constexpr float interpolationRecip = 1.0f / interpolation;
  static constexpr float maxResonance = 1.8f;

  float sampleRate, srIntRecip;
  float alpha;
  float beta[4] = {0.0, 0.0, 0.0, 0.0};
  float z0[4] = {0.0, 0.0, 0.0, 0.0};
  float z1[4] = {0.0, 0.0, 0.0, 0.0};
  float K;
  float Fbase;
  float Qadjust;
  float pbg;
  float drive, driveScaled;
  float oldinput;
  FilterMode mode;
};

}  // namespace platform

#endif  // PLATFORM_LADDER_H