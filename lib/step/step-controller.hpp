#ifndef PLATFORM_STEP_CONTROLLER_H
#define PLATFORM_STEP_CONTROLLER_H

#include "step-state.hpp"
#include "../pots.hpp"

namespace platform {

struct StepController {
  StepController(Pots &pots) : pots{pots} {};

  void update(StepState& state) {

    state.bpm.setValue(pots.getInterpolatedValue(0));
    state.stepCount.setValue(pots.getInterpolatedValue(1));
    state.skips.setValue(pots.getInterpolatedValue(2));
    state.mods.setValue(pots.getInterpolatedValue(3));

    state.shape.setValue(pots.getInterpolatedValue(4));
    state.filterResonance.setValue(pots.getInterpolatedValue(5));
    state.drive.setValue(pots.getInterpolatedValue(6));
    state.volume.setValue(pots.getInterpolatedValue(7));

    state.frequency.setValue(pots.getInterpolatedValue(8));
    state.filterCutoff.setValue(pots.getInterpolatedValue(9));
    state.noise.setValue(pots.getInterpolatedValue(10));
    state.modDestination.setValue(pots.getInterpolatedValue(11));

    state.pitchDecay.setValue(pots.getInterpolatedValue(12));
    state.cutoffDecay.setValue(pots.getInterpolatedValue(13));
    state.noiseDecay.setValue(pots.getInterpolatedValue(14));
    state.modAmount.setValue(pots.getInterpolatedValue(15));
  }

  private:
  Pots &pots;
};

} // namespace platform

#endif  // PLATFORM_STEP_CONTROLLER_H