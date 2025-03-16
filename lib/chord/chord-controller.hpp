#ifndef PLATFORM_CHORD_CONTROLLER_H
#define PLATFORM_CHORD_CONTROLLER_H

#include "chord-state.hpp"
#include "../pots.hpp"

namespace platform {

struct ChordController {
  ChordController(Pots &pots) : pots{pots} {};

  void update(ChordState& state) {

    state.lfoShape.setValue(pots.getInterpolatedValue(0));
    state.lfoRate.setValue(pots.getInterpolatedValue(1));
    state.lfoLevel.setValue(pots.getInterpolatedValue(2));
    state.volume.setValue(pots.getInterpolatedValue(3));

    state.oscillatorShape.setValue(pots.getInterpolatedValue(4));
    state.filterCutoff.setValue(pots.getInterpolatedValue(5));
    state.filterResonance.setValue(pots.getInterpolatedValue(6));
    // TODO: filter type (7)

    state.volumes[0].setValue(pots.getInterpolatedValue(8));
    state.volumes[1].setValue(pots.getInterpolatedValue(9));
    state.volumes[2].setValue(pots.getInterpolatedValue(10));
    state.volumes[3].setValue(pots.getInterpolatedValue(11));

    state.frequencies[0].setValue(pots.getInterpolatedValue(12));
    state.frequencies[1].setValue(pots.getInterpolatedValue(13));
    state.frequencies[2].setValue(pots.getInterpolatedValue(14));
    state.frequencies[3].setValue(pots.getInterpolatedValue(15));
  }

  private:
  Pots &pots;
};

} // namespace platform

#endif  // PLATFORM_CHORD_CONTROLLER_H