#ifndef PLATFORM_POTS_H
#define PLATFORM_POTS_H

#include "hardware/adc.h"
#include "hardware/gpio.h"

namespace platform {

const float tinyAmount = 0.0003f;
//const float tinyAmount = 0.f;

#define K1 9
#define K2 4
#define K3 12
#define K4 5
#define K5 3
#define K6 10
#define K7 6
#define K8 13
#define K9 2
#define K10 11
#define K11 7
#define K12 14
#define K13 0
#define K14 1
#define K15 15
#define K16 8

struct Pots {
  Pots(uint s0PinIn, uint s1PinIn, uint s2PinIn, uint s3PinIn) {
    s0Pin = s0PinIn;
    s1Pin = s1PinIn;
    s2Pin = s2PinIn;
    s3Pin = s3PinIn;

    nextPot = 0;
    for (uint i = 0; i < 16; i++) {
      currentValues[i] = 0;
      targetValues[i] = 0;
      currentIncrements[i] = 0;
    }
  }

  ~Pots() {}

  void init() {
    gpio_init(s0Pin);
    gpio_set_dir(s0Pin, GPIO_OUT);
    gpio_init(s1Pin);
    gpio_set_dir(s1Pin, GPIO_OUT);
    gpio_init(s2Pin);
    gpio_set_dir(s2Pin, GPIO_OUT); 
    gpio_init(s3Pin);
    gpio_set_dir(s3Pin, GPIO_OUT);

    setPins();

    // read in the initial values so everything makes sense from the start
    for (uint i = 0; i < 16; i++) {
      process();
    }
  }

  float getInterpolatedValue(uint index) {
    return currentValues[index];
  }

  void setPins() {
    /*
    gpio_put(s0Pin, 1);
    gpio_put(s1Pin, 1);
    gpio_put(s2Pin, 1);
    gpio_put(s3Pin, 1);

    gpio_put(s0Pin, 0);
    gpio_put(s1Pin, 0);
    gpio_put(s2Pin, 0);
    gpio_put(s3Pin, 0);
    */

    gpio_put(s0Pin, (nextPot & 1) ? 1 : 0);
    gpio_put(s1Pin, (nextPot & 2) ? 1 : 0);
    gpio_put(s2Pin, (nextPot & 4) ? 1 : 0);
    gpio_put(s3Pin, (nextPot & 8) ? 1 : 0);
  }

  void process() {
    // read twice then average
    uint16_t resultInt = adc_read() + adc_read();
    // 8191 = 2 * 4096 (12 bits) - 1
    float result = static_cast<float>(resultInt) / 8191.f;
    if (fabs(result - targetValues[nextPot]) > tinyAmount) {
      // only change if it is a significant difference
      targetValues[nextPot] =  result;
      currentIncrements[nextPot] = (targetValues[nextPot] - currentValues[nextPot]) / 16.f;
    }

    nextPot++;
    if (nextPot == 16) {
      nextPot = 0;
    }

    // set the pins for next time, hopefully giving them time to settle
    setPins();

    for (uint i = 0; i<16; i++) {
      if (fabs(currentValues[i] - targetValues[i]) < tinyAmount) {
        // snap to the target to try and prevent drift
        currentValues[i] = targetValues[i];
      } else {
        currentValues[i] += currentIncrements[i];
      }
    }
  }

  private:
  float currentValues[16];
  float targetValues[16];
  float currentIncrements[16];
  uint s0Pin;
  uint s1Pin;
  uint s2Pin;
  uint s3Pin;
  uint nextPot;
};

}  // namespace platform

#endif // PLATFORM_POTS_H