#ifndef PLATFORM_PARAMETERS_H
#define PLATFORM_PARAMETERS_H

#include "utils.hpp"

#include <cmath>
#include <algorithm>

namespace platform {

// manages a value between 0 and 1 as specified
struct RawParameter {
  float value;

  RawParameter() {
    setScaled(0);
  };
  RawParameter(float value) : value(0) {
    setScaled(value);
  };

  void setValue(float valueIn) {
    value = fclamp(valueIn, 0.f, 1.f);
  }

  auto getScaled() {
    return value;
  }

  void setScaled(float input) {
    setValue(input);
  }
};

// stores a value between 0 and 1, but scales it to a value between min and max,
// linearly.
// so 0 becomes min and 1 becomes max. rounds to the nearest integer
template<int min, int max>
struct IntegerRangeParameter {
  float value;
  int scaled;

  IntegerRangeParameter() {
    setScaled(0);
  };

  IntegerRangeParameter(float value) : value(0) {
    setScaled(value);
  };

  void setValue(auto valueIn) {
    value = fclamp(valueIn, 0.f, 1.f);
    scaled = _getScaled();
  }

  int _getScaled() {
    return (int) round(value * (max-min) + min);
  }

  int getScaled() {
    return scaled;
  }

  void setScaled(int input) {
    int cappedInput = fclamp(input, min, max);
    value = ((float) (cappedInput - min)) / (float) (max-min);
    scaled = input;
  }
};

// stores a value between 0 and 1, but scales it between min and max,
// exponentially.
// so 0 becomes min and 1 becomes max, scaled exponentially
template<float min, float max, float exponent>
struct ExponentialParameter {
  float value;
  float scaled;

  ExponentialParameter(): value{0} {
    setScaled(0.f);
  };

  ExponentialParameter(float valueIn): value{0} {
    setScaled(valueIn);
  };

  void setValue(float valueIn) {
    value = fclamp(valueIn, 0.f, 1.f);
    scaled = _getScaled();
  }

  auto _getScaled() {
    return powf(value, exponent) * (max - min) + min;
  }

  auto getScaled() {
    return scaled;
  }

  void setScaled(float input) {
    float cappedInput = fclamp(input, min, max);
    value = powf(cappedInput - min, 1.f / exponent);
    scaled = input;
  }
};

struct OverdriveParameter {
  float value;
  float pre_gain;
  float post_gain;

  OverdriveParameter() {
    setScaled(0);
  };
  OverdriveParameter(float value) : value(0) {
    setScaled(value);
  };

  void setValue(float valueIn) {
    value = fclamp(valueIn, 0.f, 1.f);

    float drive = 2.f * value;

    const float drive_2 = drive * drive;
    const float pre_gain_a = drive * 0.5f;
    const float pre_gain_b = drive_2 * drive_2 * drive * 24.0f;
    pre_gain = pre_gain_a + (pre_gain_b - pre_gain_a) * drive_2;

    const float drive_squashed = drive * (2.0f - drive);
    post_gain = 1.0f / softClip(0.33f + drive_squashed * (pre_gain - 0.33f));
  }

  float getPreGain() {
    return pre_gain;
  }

  float getPostGain() {
    return post_gain;
  }

  auto getScaled() {
    return value;
  }

  void setScaled(float input) {
    setValue(input);
  }
};


} // namespace platform

#endif // PLATFORM_PARAMETERS_H