#ifndef PLATFORM_BUTTONS_H
#define PLATFORM_BUTTONS_H

#include <stdio.h>
#include "./gpio.hpp"

namespace platform {

// NOTE: These ticks are at the audio callback rate which should be 48000/4 per
// second.

// How many ticks after the button was pressed or released we'll skip checking
// the button. This is for some added contact bounce immunity. Should be quite
// quick.
//const auto debounceTimeoutTicks = 100;
//const auto debounceTimeoutTicks = 256; // the number of samples in a buffer
const auto debounceTimeoutTicks = 32;

// How long after a button is pressed down it has for it to be released in order
// to count as a single press. Should be shorter than longTimeoutTicks.
const auto singleTimeoutTicks = 12000;

// How long after a button is single-pressed before it has to be pressed again
// for it to count as a double press.
const auto doubleTimeoutTicks = 12000;

// How long a button has to be held down before it counts as a long press.
const auto longTimeoutTicks = 24000;

struct ButtonInput {
  bool isDown;         // whether the button is currently being held down
  bool isPressed;      // true once when the button is first pressed down
  bool isReleased;     // true once when the button is first released
  bool isSingle;       // true once when the button was quickly pressed and released
  bool isDouble;       // true once when the button was quickly pressed and released twice
  bool isLong;         // true once when the button has been held down a while

  int debounceTimeout; // a positive integer if we're debouncing (ie. ignoring changes)
  int singleTimeout;   // a positive integer to timeout a single-press
  int doubleTimeout;   // a posititive integer to timeout a double-press
  int longTimeout;     // a posititive integer to timeout a long press

  int lastSingleTimeout;
  int lastDoubleTimeout;

  ButtonInput(): isDown(false),
                  isPressed(false),
                  isReleased(false),
                  isSingle(false),
                  isDouble(false),
                  isLong(false),
                  debounceTimeout(0),
                  singleTimeout(0),
                  doubleTimeout(0),
                  longTimeout(0),
                  lastSingleTimeout(0),
                  lastDoubleTimeout(0)
                 {}

  void update(bool state) {
    // clear all these booleans because they can only be true for one update
    isPressed = false;
    isReleased = false;
    isSingle = false;
    isDouble = false;
    isLong = false;

    // timeout the debounce
    if (debounceTimeout) {
      debounceTimeout--;
      // ignore all events until the debounce times out
      return;
    }

    // timeout a single-press
    if (singleTimeout) {
      singleTimeout--;
    }

    // timeout a double-press
    if (doubleTimeout) {
      doubleTimeout--;
    }

    if (state == true) {
      if (isDown) {
        // was already down, is still down

        // timeout a long press
        if (longTimeout) {
          longTimeout--;
          if (longTimeout == 0) {
            // if you reach the timeout (ie. you held the button long enough),
            // then it counts as a long press
            isLong = true;
          }
        }
      }
      else {
        // was up, is now down
        isPressed = true;

        // reset the debounce timeout so that switch bounce won't affect as as much
        debounceTimeout = debounceTimeoutTicks;
        // reset the longTimeout so we start counting for a long press
        longTimeout = longTimeoutTicks;
        // reset the singleTimeout so we start counting for a quick press&release
        singleTimeout = singleTimeoutTicks;
      }
    } else {
      if (isDown) {
        // was down, is now up

        // reset the debounce timeout so that switch bounce won't affect as as much
        debounceTimeout = debounceTimeoutTicks;

        isReleased = true;

        // can't get a long press anymore
        longTimeout = 0;

        if (doubleTimeout) {
          // checking the doubleTimeout before we set it below otherwise every
          // press is a double press
          lastDoubleTimeout = doubleTimeout;
          doubleTimeout = 0;
          isDouble = true;
        }

        if (singleTimeout) {
          // a single press&release happens every time you quickly perform the
          // action, even on double-press
          lastSingleTimeout = singleTimeout;
          singleTimeout = 0;
          isSingle = true;
          // start counting for a double press
          doubleTimeout = doubleTimeoutTicks;
        }
      }
    }

    isDown = state;
  }
};
}  // namespace platform

#endif // PLATFORM_BUTTONS_H