#ifndef SEQUENCER_HPP
#define SEQUENCER_HPP

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>
#include <vector>


#ifndef M_PI
#define M_PI 3.1415927410125732421875f
#endif

namespace platform {

class Sequencer {
public:
  struct Step {
    bool gate;
    float cv;
  };

  Sequencer()
    : sequence(kMaxSteps),
      currentStep(0),
      sequenceLength(32),
      complexity(16),
      density(0.5f),
      spread(0.5f),
      bias(0.4f),
      cvSeed(0),
      cvPaletteSeed(0) {
    // Initialize with default parameters
    regenerateCVPalette();
    regenerateControlVoltages();
    regenerateGates();
  }

  // Main interface - returns gate and CV for current step and advances
  std::pair<bool, float> process() {
    Step& step = sequence[currentStep];
    std::pair<bool, float> result = {step.gate, step.cv};

    // Advance to next step
    currentStep = (currentStep + 1) % sequenceLength;

    return result;
  }

  // Parameter setters

  void setSequenceLength(int length) {
    if (length < 1 || length > kMaxSteps) {
      return;
    }
    sequenceLength = length;
    // Reset position if we're now beyond the sequence length
    if (currentStep >= sequenceLength) {
      currentStep = 0;
    }
  }

  void setComplexity(int newComplexity) {
    if (newComplexity < 1 || newComplexity > kMaxSteps) {
      return;
    }
    complexity = newComplexity;
    regenerateCVPalette();
    regenerateControlVoltages();
  }

  void setDensity(float newDensity) {
    if (newDensity < 0.0f || newDensity > 1.0f) {
      return;
    }
    density = newDensity;
    regenerateGates();
  }

  void setSpread(float newSpread) {
    if (newSpread < 0.0f || newSpread > 1.0f) {
      return;
    }
    spread = newSpread;
    regenerateControlVoltages();
  }

  void setBias(float newBias) {
    if (newBias < 0.0f || newBias > 1.0f) {
      return;
    }
    // Quantize to nearest 0.04 for reproducibility
    bias = std::round(newBias / 0.04f) * 0.04f;
    // Clamp to ensure we stay in [0, 1] after quantization
    bias = std::max(0.0f, std::min(1.0f, bias));
    regenerateCVPalette();
    regenerateControlVoltages();
  }

  // Parameter getters
  int getCurrentStep() const { return currentStep; }
  int getSequenceLength() const { return sequenceLength; }
  int getComplexity() const { return complexity; }
  float getDensity() const { return density; }
  float getSpread() const { return spread; }
  float getBias() const { return bias; }
  unsigned int getCVSeed() const { return cvSeed; }
  unsigned int getCVPaletteSeed() const { return cvPaletteSeed; }

  // Set seed for CV assignment reproducibility
  void setCVSeed(unsigned int seed) {
    cvSeed = seed;
    regenerateControlVoltages();
  }

  // Set seed for CV palette reproducibility
  void setCVPaletteSeed(unsigned int seed) {
    cvPaletteSeed = seed;
    regenerateCVPalette();
    regenerateControlVoltages();
  }

  // Reset to first step
  void reset() {
    currentStep = 0;
  }

private:
  static constexpr int kMaxSteps = 32;

  // Sequence data
  std::vector<Step> sequence;
  int currentStep;

  // Parameters
  int sequenceLength;  // 1-32
  int complexity;      // 1-32
  float bias;          // 0-1
  float spread;        // 0-1
  float density;       // 0-1

  // Control voltage palette
  std::vector<float> cvPalette;

  // Random number generation seeds
  unsigned int cvSeed;
  unsigned int cvPaletteSeed;

  // Internal generation methods
  void regenerateGates() {
    // Calculate how many gates should be true
    int numGates = static_cast<int>(std::round(density * kMaxSteps));

    // Reset all gates to false
    for (auto& step : sequence) {
      step.gate = false;
    }

    if (numGates == 0) {
      return;
    }

    // Distribute gates evenly
    float spacing = static_cast<float>(kMaxSteps) / numGates;

    for (int i = 0; i < numGates; ++i) {
      int position = static_cast<int>(std::round(i * spacing)) % kMaxSteps;
      sequence[position].gate = true;
    }

    /*
    // Rotate gates so first step always has a gate (downbeat)
    if (numGates > 0 && !sequence[0].gate) {
      // Find the first gate
      int firstGatePos = 0;
      for (int i = 0; i < kMaxSteps; ++i) {
        if (sequence[i].gate) {
          firstGatePos = i;
          break;
        }
      }

      // Rotate gates to the left by firstGatePos positions
      std::vector<bool> gatePattern(kMaxSteps);
      for (int i = 0; i < kMaxSteps; ++i) {
        gatePattern[i] = sequence[i].gate;
      }
      for (int i = 0; i < kMaxSteps; ++i) {
        sequence[i].gate = gatePattern[(i + firstGatePos) % kMaxSteps];
      }
    }
    */
  }

  void regenerateCVPalette() {
    cvPalette.clear();
    cvPalette.reserve(complexity);

    // Use seeded RNG for reproducible palette generation
    std::mt19937 paletteRng(cvPaletteSeed);

    for (int i = 0; i < complexity; ++i) {
      cvPalette.push_back(generateBiasedCV(paletteRng));
    }

    // Sort palette for consistent ordering
    std::sort(cvPalette.begin(), cvPalette.end());
  }

  void regenerateControlVoltages() {
    if (cvPalette.empty()) {
      return;
    }

    int totalRuns = calculateTotalRuns();

    // Create a list of CV indices to use
    std::vector<int> cvIndices;
    cvIndices.reserve(totalRuns);

    // Fill with palette indices, cycling if necessary
    for (int i = 0; i < totalRuns; ++i) {
      cvIndices.push_back(i % complexity);
    }

    // Shuffle the CV indices for variety using seeded RNG for reproducibility
    std::mt19937 cvRng(cvSeed);
    std::shuffle(cvIndices.begin(), cvIndices.end(), cvRng);

    // TODO: there could be two identical cv values next to each other which
    // would mean that two adjacent runs could be identical and therefore merge
    // into just the same single run. For low values of complexity plus
    // totalRuns higher than the complexity value it would be very noticeable.
    // We don't want that.

    // Calculate run length
    float avgRunLength = static_cast<float>(kMaxSteps) / totalRuns;

    // Assign CVs in runs
    int currentPosition = 0;
    int runIndex = 0;

    while (currentPosition < kMaxSteps && runIndex < totalRuns) {
      // Determine this run's length
      int remainingSteps = kMaxSteps - currentPosition;
      int remainingRuns = totalRuns - runIndex;

      float targetLength = static_cast<float>(remainingSteps) / remainingRuns;
      int runLength = std::max(1, static_cast<int>(std::round(targetLength)));

      // Don't exceed the sequence
      runLength = std::min(runLength, remainingSteps);

      // Assign CV to this run
      float cv = cvPalette[cvIndices[runIndex]];
      for (int i = 0; i < runLength; ++i) {
        sequence[currentPosition + i].cv = cv;
      }

      currentPosition += runLength;
      runIndex++;
    }
  }

  // Helper methods

  float generateBiasedCV(std::mt19937& rngToUse) {
    // Use Box-Muller transform to generate Gaussian distribution
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    float u1 = dist(rngToUse);
    float u2 = dist(rngToUse);

    // Box-Muller transform
    float z0 = std::sqrt(-2.0f * std::log(u1)) * std::cos(2.0f * M_PI * u2);

    // Scale and shift based on bias (mean at bias, stddev of 0.2)
    // We'll use stddev of 0.2 to keep values mostly in range
    float value = bias + z0 * 0.2f;

    // Clamp to [0, 1]
    return std::max(0.0f, std::min(1.0f, value));
  }

  int calculateTotalRuns() const {
    // At spread = 0: number of runs = complexity (one run per CV value)
    // At spread = 1: number of runs = kMaxSteps (CV changes every step)

    int minRuns = complexity;
    int maxRuns = kMaxSteps;

    // Linear interpolation based on spread
    int totalRuns =
      static_cast<int>(std::round(minRuns + spread * (maxRuns - minRuns)));

    // Ensure we have at least as many runs as complexity values
    return std::max(minRuns, std::min(maxRuns, totalRuns));
  }
};

}  // namespace platform

#endif  // SEQUENCER_HPP
