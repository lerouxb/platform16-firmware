#ifndef PLATFORM_TEP_CONTROLLER_H
#define PLATFORM_TEP_CONTROLLER_H

#include "../../lib/pots.hpp"
#include "tep-state.hpp"

namespace platform {

struct TEPController {
  TEPController(Pots& pots) : pots{pots} {};

  void update(TEPState& state) {
    state.distortionRhythm.setValue(pots.getInterpolatedValue(K1));
    state.glide.setValue(pots.getInterpolatedValue(K2));
    
    state.distortion.setValue(pots.getInterpolatedValue(K3));
    state.bpm.setValue(pots.getInterpolatedValue(K4));
    state.volume.setValue(pots.getInterpolatedValue(K5));

    state.distortionAccent.setValue(pots.getInterpolatedValue(K6));
    state.detune.setValue(pots.getInterpolatedValue(K7));

    state.resonance.setValue(pots.getInterpolatedValue(K8));
    state.octave.setValue(pots.getInterpolatedValue(K9));

    state.cutoffAccent.setValue(pots.getInterpolatedValue(K10));
    state.arpeggioMode.setValue(pots.getInterpolatedValue(K11));

    state.cutoff.setValue(pots.getInterpolatedValue(K12));
    state.rotate.setValue(pots.getInterpolatedValue(K13));
    state.degree.setValue(pots.getInterpolatedValue(K14));

    state.cutoffRhythm.setValue(pots.getInterpolatedValue(K15));
    state.degreeRhythm.setValue(pots.getInterpolatedValue(K16));
  }

  private:
  Pots& pots;
};

}  // namespace platform

#endif  // PLATFORM_TEP_CONTROLLER_H
