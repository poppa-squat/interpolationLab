#pragma once

#include "dsp/Interpolator.h"

namespace interpolation_lab {

inline void twoSourceWeights(const float mix, float& source0, float& source1) {
    const auto clamped = mix < 0.0F ? 0.0F : (mix > 1.0F ? 1.0F : mix);
    source0 = 1.0F - clamped;
    source1 = clamped;
}

inline auto sampleOrZero(const ConstAudioView& view, const int channel, const int sample) -> float {
    if (view.numChannels <= 0 || view.channels == nullptr) {
        return 0.0F;
    }
    const auto sourceChannel = channel < view.numChannels ? channel : view.numChannels - 1;
    if (sourceChannel < 0 || view.channels[sourceChannel] == nullptr) {
        return 0.0F;
    }
    if (sample < 0 || sample >= view.numSamples) {
        return 0.0F;
    }
    return view.channels[sourceChannel][sample];
}

} // namespace interpolation_lab
