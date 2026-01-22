/*
Ported from daisysp, Copyright (c) 2020 Electrosmith, Corp

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/

#ifndef PLATFORM_FM2_H
#define PLATFORM_FM2_H

#include "oscillator.hpp"

namespace platform {

struct PM2 {

  PM2() {}
  ~PM2() {}

  void init(float sampleRate) {
    //init oscillators
    car.init(sampleRate);
    mod.init(sampleRate);

    //set some reasonable values
    lfreq = 440.f;
    lratio = 2.f;
    setFrequency(lfreq);
    setRatio(lratio);

    car.setAmp(1.f);
    ldepth = 1.f;
    mod.setAmp(ldepth);

    car.setWaveform(Oscillator::WAVE_SIN);
    mod.setWaveform(Oscillator::WAVE_SIN);
  }
  
  float process() {
    if (lratio != ratio || lfreq != freq || ldepth != depth) {
        lratio = ratio;
        lfreq = freq;
        ldepth = depth;
        car.setFreq(lfreq);
        mod.setFreq(lfreq * lratio);
        mod.setAmp(ldepth);
    }

    float modval = mod.process();
    car.phaseAdd(modval);
    return car.process();
  }

  void setFrequency(float freqIn) {
    freq = fabsf(freqIn);
  }

  void setRatio(float ratioIn) {
    ratio = fabsf(ratioIn);
  }

  void setDepth(float depthIn) {
    depth = fabsf(depthIn);
  }

  float getDepth() {
    return depth;
  }

  void reset() {
    car.reset();
    mod.reset();
  }

  private:
    Oscillator mod, car;
    float      freq, lfreq, ratio, lratio, ldepth, depth;
};
} // namespace platform

#endif // PLATFORM_FM2_H