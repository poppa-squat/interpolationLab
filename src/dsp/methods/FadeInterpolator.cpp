#include "dsp/methods/FadeInterpolator.h"

#include "dsp/EqualPower.h"
#include "dsp/Weights.h"

#include <algorithm>
#include <cmath>

namespace interpolation_lab {

void FadeInterpolator::prepare(const ProcessSpec& spec) {
    const auto sources = std::max(1, spec.maxSources);
    gains_.assign(static_cast<std::size_t>(sources), 0.0F);
}

void FadeInterpolator::reset() {}

auto FadeInterpolator::latencySamples() const -> int { return 0; }

void FadeInterpolator::fillGains(const std::span<const float> weights, const std::size_t sourceCount) {
    const auto mixCount = std::min(sourceCount, gains_.size());
    if (mixCount == 0) {
        return;
    }

    if (sourceCount == 2 && gains_.size() >= 2) {
        const auto mix = weights.size() > 1 ? weights[1] : 0.0F;
        equalPowerGains(mix, gains_[0], gains_[1]);
        return;
    }

    for (std::size_t i = 0; i < mixCount; ++i) {
        const auto weight = i < weights.size() ? weights[i] : 0.0F;
        const auto clamped = weight < 0.0F ? 0.0F : weight;
        gains_[i] = std::sqrt(clamped);
    }
}

void FadeInterpolator::process(const std::span<const ConstAudioView> sources,
                               const std::span<const float> weights, const AudioView output) {
    if (output.channels == nullptr || output.numChannels <= 0 || output.numSamples <= 0) {
        return;
    }

    const auto sourceCount = std::min(sources.size(), gains_.size());
    fillGains(weights, sources.size());

    const auto samples = output.numSamples;
    for (int channel = 0; channel < output.numChannels; ++channel) {
        auto* out = output.channels[channel];
        if (out == nullptr) {
            continue;
        }
        for (int sample = 0; sample < samples; ++sample) {
            auto mixed = 0.0F;
            for (std::size_t source = 0; source < sourceCount; ++source) {
                mixed += gains_[source] * sampleOrZero(sources[source], channel, sample);
            }
            out[sample] = mixed;
        }
    }
}

} // namespace interpolation_lab
