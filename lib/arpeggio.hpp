
#ifndef PLATFORM_ARPEGGIO_H
#define PLATFORM_ARPEGGIO_H

#include <vector>
#include <cstdlib>

namespace platform {

enum class ArpeggioMode {
  NO_ARPEGGIO,      // always return the first note
  UP,               // up
  DOWN,             // down
  UP_DOWN,          // up/down
  DOWN_UP,          // down/up
  CONVERGE,         // converge
  DIVERGE,          // diverge
  CONVERGE_DIVERGE, // converge/diverge
  DIVERGE_CONVERGE, // diverge/converge
  RANDOM            // random
};

struct Arpeggio {
  Arpeggio() : nextStep(0), direction(1), mode(ArpeggioMode::NO_ARPEGGIO), phase(0), lastValue(1.0f) {}

  void setMode(ArpeggioMode newMode) {
    if (newMode == mode) {
      return;
    }
    mode = newMode;
    reset();
  }

  void setValues(const std::vector<float>& newValues) {
    // TODO: this is not the most efficient. We can probably just compare the noteIndex on the outside
    if (newValues == values) {
      return;
    }
    values = newValues;
    reset();
  }

  void reset() {
    if (mode == ArpeggioMode::DOWN || mode == ArpeggioMode::DOWN_UP) {
      nextStep = values.size() - 1;
      direction = -1;
    } else {
      nextStep = 0; 
      direction = 1;
    }
    phase = 0;
  }

  float getLastValue() const {
    return lastValue;
  }

  float process() {
    // If the vector is empty, always return 1
    if (values.empty()) {
      lastValue = 1.0f;
      return lastValue;
    }

    float result = 1.0f;

    switch (mode) {
      case ArpeggioMode::NO_ARPEGGIO:
        result = values[0];
        break;

      case ArpeggioMode::UP:
        result = values[nextStep];
        nextStep = (nextStep + 1) % values.size();
        break;

      case ArpeggioMode::DOWN:
        result = values[nextStep];
        if (nextStep == 0) {
          nextStep = values.size() - 1;
        } else {
          nextStep--;
        }
        break;

      case ArpeggioMode::UP_DOWN:
        result = values[nextStep];
        nextStep += direction;
        
        if (nextStep >= values.size()) {
          nextStep = values.size() - 2;
          if (nextStep < 0) nextStep = 0;
          direction = -1;
        } else if (nextStep < 0) {
          nextStep = 1;
          if (nextStep >= values.size()) nextStep = 0;
          direction = 1;
        }
        break;

      case ArpeggioMode::DOWN_UP:
        result = values[nextStep];
        nextStep += direction;
        
        if (nextStep < 0) {
          nextStep = 1;
          if (nextStep >= values.size()) nextStep = 0;
          direction = 1;
        } else if (nextStep >= values.size()) {
          nextStep = values.size() - 2;
          if (nextStep < 0) nextStep = 0;
          direction = -1;
        }
        break;

      case ArpeggioMode::CONVERGE: {
        // Alternate between beginning and end, moving toward the middle
        int leftIndex = phase / 2;
        int rightIndex = values.size() - 1 - (phase / 2);
        
        if (phase % 2 == 0) {
          result = values[leftIndex];
        } else {
          result = values[rightIndex];
        }
        
        phase++;
        if (leftIndex >= rightIndex || phase >= values.size() * 2) {
          phase = 0;
        }
        break;
      }

      case ArpeggioMode::DIVERGE: {
        // Start from middle, alternate outward
        int mid = values.size() / 2;
        int offset = phase / 2;
        
        if (phase % 2 == 0) {
          nextStep = mid + offset;
          if (nextStep >= values.size()) nextStep = values.size() - 1;
        } else {
          nextStep = mid - offset - 1;
          if (nextStep < 0) nextStep = 0;
        }
        
        result = values[nextStep];
        
        phase++;
        if (phase >= values.size() * 2) {
          phase = 0;
        }
        break;
      }

      case ArpeggioMode::CONVERGE_DIVERGE: {
        // converge then diverge
        int halfCycle = values.size();
        
        if (phase < halfCycle) {
          // converge phase
          int leftIndex = phase / 2;
          int rightIndex = values.size() - 1 - (phase / 2);
          
          if (phase % 2 == 0) {
            result = values[leftIndex];
          } else {
            result = values[rightIndex];
          }
        } else {
          // diverge phase
          int divergePhase = phase - halfCycle;
          int mid = values.size() / 2;
          int offset = divergePhase / 2;
          
          if (divergePhase % 2 == 0) {
            nextStep = mid + offset;
            if (nextStep >= values.size()) nextStep = values.size() - 1;
          } else {
            nextStep = mid - offset - 1;
            if (nextStep < 0) nextStep = 0;
          }
          
          result = values[nextStep];
        }
        
        phase++;
        if (phase >= values.size() * 2) {
          phase = 0;
        }
        break;
      }

      case ArpeggioMode::DIVERGE_CONVERGE: {
        // diverge then converge
        int halfCycle = values.size();
        
        if (phase < halfCycle) {
          // diverge phase
          int mid = values.size() / 2;
          int offset = phase / 2;
          
          if (phase % 2 == 0) {
            nextStep = mid + offset;
            if (nextStep >= values.size()) nextStep = values.size() - 1;
          } else {
            nextStep = mid - offset - 1;
            if (nextStep < 0) nextStep = 0;
          }
          
          result = values[nextStep];
        } else {
          // converge phase
          int convergePhase = phase - halfCycle;
          int leftIndex = convergePhase / 2;
          int rightIndex = values.size() - 1 - (convergePhase / 2);
          
          if (convergePhase % 2 == 0) {
            result = values[leftIndex];
          } else {
            result = values[rightIndex];
          }
        }
        
        phase++;
        if (phase >= values.size() * 2) {
          phase = 0;
        }
        break;
      }

      case ArpeggioMode::RANDOM:
        nextStep = rand() % values.size();
        result = values[nextStep];
        break;
    }

    lastValue = result;
    return result;
  }

private:
  std::vector<float> values;
  int nextStep;
  int direction;
  ArpeggioMode mode;
  int phase;
  float lastValue;
};

}  // namespace platform

#endif  // PLATFORM_ARPEGGIO_H
