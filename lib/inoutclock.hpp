#ifndef PLATFORM_INOUTCLOCK_H
#define PLATFORM_INOUTCLOCK_H

#include "./gpio.hpp"
#include "./metro.hpp"

namespace platform {

/*
This class detects whether an external clock is connected on
CLOCK_IN_CONNECTED_PIN and switches between using that (via CLOCK_IN_PIN) and an
internal clock. If an external clock is connected it also handles clock division
and multiplication based on a position parameter (0-14) where 0-6 is division
(1/8 to 1/2), 7 is normal speed, and 8-14 is multiplication (2x to 8x).

It then outputs a clock signal on the CLOCK_OUT_PIN at every second 16th note
like teenage engineering devices do.
*/
struct InOutClock {
  InOutClock(Metro& clockIn): clock(clockIn), sampleRate(0), isExternalClock(false), externalClockTicks(0), clockTicks(0), samplesSinceLastClockTick(0), externalClockFrequency(0), previousClockState(false) {}
  ~InOutClock() {}

  void init(float sampleRateIn) {
    sampleRate = sampleRateIn;
  } 

  float getTickFrequency(float bpm) {
    if (isExternalClock) {
      if (externalClockTicks < 2) {
        // stop the clock until ticks arrive
        return 0.f;
      }
      float position = round(bpm * 14.f);

      // 0-6 is divider with 0 being 1/8, 7 is * 1, 8-14 is multiplier with 8 being 2 and 14 being 8
      float multiplier = (position < 7.f) ? 1.f / (8.f - position) : position - 6.f;

      return externalClockFrequency * multiplier;
    } else {
      if (bpm < 0.005) {
        // if it is close to zero, then just stop the clock
        return 0.f;
      }
      return bpm / 60.f * 4.f;  // 16th notes, not quarter notes
    }
  }

  bool process(float bpm) {
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

          float position = round(bpm * 14.f);
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
    bool isTick = clock.process() || tick;
    clockTicks++;

    // Going with the teenage engineering approach of clock ticks on every
    // second 16th note. Same as for interpreting clock inputs.
    if (clockTicks % 2 == 0) {
      // the pin is inverted because it is tied to an NPN transistor
      gpio_put(CLOCK_OUT_PIN, false);
    } else {
      gpio_put(CLOCK_OUT_PIN, true);
    }

    return isTick;
  }

  int getClockTicks() {
    return clockTicks;
  }

  private:
  // isInternalClock, externalClockTicks, clockTicks, samplesSinceLastClockTick,
  // externalClockFrequency, previousClockState, previousClockState, clock, state
  Metro& clock;
  float sampleRate;
  bool isExternalClock;
  int externalClockTicks;
  int clockTicks;
  int samplesSinceLastClockTick;
  float externalClockFrequency;
  bool previousClockState;

};

} // namespace platform

#endif // PLATFORM_INOUTCLOCK_H
