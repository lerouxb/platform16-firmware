#ifndef PLATFORM_SDS_CONTROLLER_H
#define PLATFORM_SDS_CONTROLLER_H

#include "sds-state.hpp"
#include "../../lib/pots.hpp"

namespace platform {

struct SDSController {
  SDSController(Pots &pots) : pots{pots} {};

  void update(SDSState& state) {
    state.volume.setValue(pots.getInterpolatedValue(K1));
    state.volumeEnvelope.setValue(pots.getInterpolatedValue(K2));
    state.stepCount.setValue(pots.getInterpolatedValue(K3));
    state.drive.setValue(pots.getInterpolatedValue(K4)); // X

    state.evolve.setValue(pots.getInterpolatedValue(K5));
    state.skips.setValue(pots.getInterpolatedValue(K6));
    state.bpm.setValue(pots.getInterpolatedValue(K7));
    state.algorithm.setValue(pots.getInterpolatedValue(K8));

    state.cutoffEnvelope.setValue(pots.getInterpolatedValue(K9));
    state.scale.setValue(pots.getInterpolatedValue(K10));
    state.resonance.setValue(pots.getInterpolatedValue(K11));
    state.basePitch.setValue(pots.getInterpolatedValue(K12));

    state.noise.setValue(pots.getInterpolatedValue(K13)); // Y
    state.cutoff.setValue(pots.getInterpolatedValue(K14));
    state.pitchAmount.setValue(pots.getInterpolatedValue(K15));
    state.cutoffAmount.setValue(pots.getInterpolatedValue(K16));
  }

  private:
  Pots &pots;
};

} // namespace platform

#endif  // PLATFORM_SDS_CONTROLLER_H