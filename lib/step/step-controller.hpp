#ifndef PLATFORM_STEP_CONTROLLER_H
#define PLATFORM_STEP_CONTROLLER_H

#include "step-state.hpp"
#include "../pots.hpp"

namespace platform {

struct StepController {
  StepController(Pots &pots) : pots{pots} {};

  void update(StepState& state) {

    state.bpm.setValue(pots.getInterpolatedValue(0));
    state.volume.setValue(pots.getInterpolatedValue(1));
    state.pitch.setValue(pots.getInterpolatedValue(2));
    state.cutoff.setValue(pots.getInterpolatedValue(3));

    state.stepCount.setValue(pots.getInterpolatedValue(4));
    state.volumeDecay.setValue(pots.getInterpolatedValue(5));
    state.pitchDecay.setValue(pots.getInterpolatedValue(6));
    state.cutoffDecay.setValue(pots.getInterpolatedValue(7));

    state.skips.setValue(pots.getInterpolatedValue(8));
    state.volumeAmount.setValue(pots.getInterpolatedValue(9));
    state.pitchAmount.setValue(pots.getInterpolatedValue(10));
    state.cutoffAmount.setValue(pots.getInterpolatedValue(11));

    state.algorithm.setValue(pots.getInterpolatedValue(12));
    state.drive.setValue(pots.getInterpolatedValue(13));
    state.scale.setValue(pots.getInterpolatedValue(14));
    state.resonance.setValue(pots.getInterpolatedValue(15));
  }

  private:
  Pots &pots;
};

} // namespace platform

#endif  // PLATFORM_STEP_CONTROLLER_H