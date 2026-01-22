#ifndef PLATFORM_PMD_CONTROLLER_H
#define PLATFORM_PMD_CONTROLLER_H

#include "pmd-state.hpp"
#include "../../lib/pots.hpp"

namespace platform {

struct PMDController {
  PMDController(Pots &pots) : pots{pots} {};

  void update(PMDState& state) {
    state.bpm.setValue(pots.getInterpolatedValue(K1));
    state.volume.setValue(pots.getInterpolatedValue(K2));

    state.length.setValue(pots.getInterpolatedValue(K3));
    state.complexity.setValue(pots.getInterpolatedValue(K4));
    state.bias.setValue(pots.getInterpolatedValue(K5));

    state.density.setValue(pots.getInterpolatedValue(K6));
    state.spread.setValue(pots.getInterpolatedValue(K7));

    state.baseFreq.setValue(pots.getInterpolatedValue(K8));
    state.decay.setValue(pots.getInterpolatedValue(K9));

    state.range.setValue(pots.getInterpolatedValue(K10));
    state.scramble.setValue(pots.getInterpolatedValue(K11));

    state.tembreLFODepth.setValue(pots.getInterpolatedValue(K12));
    state.modulatorDepth.setValue(pots.getInterpolatedValue(K13));
    state.envelopeLFODepth.setValue(pots.getInterpolatedValue(K14));

    state.tembreLFORate.setValue(pots.getInterpolatedValue(K15));
    state.envelopeLFORate.setValue(pots.getInterpolatedValue(K16));
  }

  private:
  Pots &pots;
};

} // namespace platform

#endif  // PLATFORM_PMD_CONTROLLER_H