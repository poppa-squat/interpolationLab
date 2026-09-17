#include "dsp/methods/RaveInterpolator.h"

#include "dsp/AnalysisSettings.h"
#include "dsp/EqualPower.h"
#include "dsp/LatentMath.h"
#include "dsp/Weights.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace interpolation_lab {
namespace {

auto mixFromWeights(const std::span<const float> weights) -> float {
    return weights.size() > 1 ? weights[1] : 0.0F;
}

void endpointGains(const float mix, float& drySourceGain, float& wetGain, float& dryTargetGain) {
    drySourceGain = 0.0F;
    wetGain = 1.0F;
    dryTargetGain = 0.0F;
    if (mix <= kEndpointWidth) {
        const auto t = kEndpointWidth > 0.0F ? mix / kEndpointWidth : 1.0F;
        equalPowerGains(t, drySourceGain, wetGain);
        return;
    }
    if (mix >= 1.0F - kEndpointWidth) {
        const auto t = kEndpointWidth > 0.0F ? (mix - (1.0F - kEndpointWidth)) / kEndpointWidth : 1.0F;
        equalPowerGains(t, wetGain, dryTargetGain);
    }
}

void resampleLinear(const std::span<const float> input, const std::span<float> output) {
    if (output.empty()) {
        return;
    }
    if (input.empty()) {
        std::fill(output.begin(), output.end(), 0.0F);
        return;
    }
    if (input.size() == output.size()) {
        std::copy(input.begin(), input.end(), output.begin());
        return;
    }

    const auto last = static_cast<double>(input.size() - 1);
    const auto denom =
        output.size() == 1 ? 1.0 : static_cast<double>(output.size() - 1);
    for (std::size_t i = 0; i < output.size(); ++i) {
        const auto pos = last * static_cast<double>(i) / denom;
        const auto index = static_cast<std::size_t>(pos);
        const auto next = std::min(index + 1, input.size() - 1);
        const auto t = static_cast<float>(pos - static_cast<double>(index));
        output[i] = input[index] * (1.0F - t) + input[next] * t;
    }
}

auto ratesMatch(const double hostRate, const double modelRate) -> bool {
    return std::abs(hostRate - modelRate) < 0.5;
}

} // namespace

auto tryLoadRaveModelFromEnv() -> std::unique_ptr<AudioAutoencoder> {
    const auto* path = std::getenv(kRaveModelEnvVar);
    if (path == nullptr || path[0] == '\0') {
        return nullptr;
    }
    return loadRaveTorchScript(path);
}

#ifndef INTERPOLATION_LAB_USE_TORCH
auto loadRaveTorchScript(const std::string&) -> std::unique_ptr<AudioAutoencoder> {
    return nullptr;
}
#endif

RaveInterpolator::RaveInterpolator() : RaveInterpolator(tryLoadRaveModelFromEnv()) {}

RaveInterpolator::RaveInterpolator(std::unique_ptr<AudioAutoencoder> encoder)
    : prototype_(std::move(encoder)) {}

void RaveInterpolator::prepare(const ProcessSpec& spec) {
    numChannels_ = std::max(1, spec.numChannels);
    maxBlockSize_ = std::max(1, spec.maxBlockSize);
    maxSources_ = std::max(1, spec.maxSources);
    hostSampleRate_ = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
    hopSize_ = 0;
    hostHop_ = 0;
    latentSize_ = 0;
    latency_ = 0;
    modelSampleRate_ = hostSampleRate_;
    channels_.clear();
    latentViews_.clear();

    if (prototype_ == nullptr) {
        return;
    }

    prototype_->matchHostSampleRate(hostSampleRate_);
    prototype_->reset();
    hopSize_ = std::max(1, prototype_->hopSize());
    latentSize_ = std::max(1, prototype_->latentSize());
    modelSampleRate_ = prototype_->sampleRate() > 0.0 ? prototype_->sampleRate() : hostSampleRate_;
    if (ratesMatch(hostSampleRate_, modelSampleRate_)) {
        hostHop_ = hopSize_;
        modelSampleRate_ = hostSampleRate_;
    } else {
        hostHop_ = std::max(
            1, static_cast<int>(std::lround(static_cast<double>(hopSize_) * hostSampleRate_ /
                                            modelSampleRate_)));
    }
    latency_ = hostHop_;
    latentViews_.assign(static_cast<std::size_t>(maxSources_), {});
    channels_.assign(static_cast<std::size_t>(numChannels_), {});
    reset();
}

void RaveInterpolator::reset() {
    for (auto& channel : channels_) {
        resetChannel(channel);
    }
}

auto RaveInterpolator::latencySamples() const -> int { return latency_; }

auto RaveInterpolator::hasAutoencoder() const -> bool { return prototype_ != nullptr; }

auto RaveInterpolator::hopSize() const -> int { return hopSize_; }

auto RaveInterpolator::hostHopSize() const -> int { return hostHop_; }

void RaveInterpolator::resetChannel(ChannelState& channel) {
    channel.sources.clear();
    channel.decoder = prototype_ != nullptr ? prototype_->clone() : nullptr;
    if (channel.decoder != nullptr) {
        channel.decoder->reset();
    }

    channel.sources.resize(static_cast<std::size_t>(maxSources_));
    for (auto& source : channel.sources) {
        source.encoder = prototype_ != nullptr ? prototype_->clone() : nullptr;
        if (source.encoder != nullptr) {
            source.encoder->reset();
        }
        source.hostHop.assign(static_cast<std::size_t>(std::max(1, hostHop_)), 0.0F);
        source.modelHop.assign(static_cast<std::size_t>(std::max(1, hopSize_)), 0.0F);
        source.latent.assign(static_cast<std::size_t>(std::max(1, latentSize_)), 0.0F);
    }

    channel.mixedLatent.assign(static_cast<std::size_t>(std::max(1, latentSize_)), 0.0F);
    channel.decodedModelHop.assign(static_cast<std::size_t>(std::max(1, hopSize_)), 0.0F);
    channel.decodedHostHop.assign(static_cast<std::size_t>(std::max(1, hostHop_)), 0.0F);

    const auto outputSize = static_cast<std::size_t>(
        std::max(1, latency_ + maxBlockSize_ + hostHop_ + 8));
    channel.outputRing.assign(outputSize, 0.0F);
    channel.hopFill = 0;
    channel.outputWritePos = 0;
    channel.outputReadPos = 0;
    channel.outputCount = 0;

    for (int i = 0; i < latency_; ++i) {
        pushOutput(channel, 0.0F);
    }
}

void RaveInterpolator::pushOutput(ChannelState& channel, const float sample) {
    if (channel.outputRing.empty()) {
        return;
    }
    channel.outputRing[static_cast<std::size_t>(channel.outputWritePos)] = sample;
    channel.outputWritePos =
        (channel.outputWritePos + 1) % static_cast<int>(channel.outputRing.size());
    ++channel.outputCount;
}

auto RaveInterpolator::popOutput(ChannelState& channel) -> float {
    if (channel.outputRing.empty() || channel.outputCount <= 0) {
        return 0.0F;
    }
    const auto sample = channel.outputRing[static_cast<std::size_t>(channel.outputReadPos)];
    channel.outputReadPos =
        (channel.outputReadPos + 1) % static_cast<int>(channel.outputRing.size());
    --channel.outputCount;
    return sample;
}

void RaveInterpolator::processHop(ChannelState& channel, const std::span<const float> weights,
                                  const float mix) {
    const auto sourceCount = channel.sources.size();
    for (std::size_t source = 0; source < sourceCount; ++source) {
        auto& state = channel.sources[source];
        resampleLinear(state.hostHop, state.modelHop);
        const auto weight = source < weights.size() ? weights[source] : 0.0F;
        if (state.encoder == nullptr || (weight <= 0.0F && source > 1)) {
            std::fill(state.latent.begin(), state.latent.end(), 0.0F);
            continue;
        }
        state.encoder->encode(state.modelHop, state.latent);
    }

    latentViews_.resize(sourceCount);
    for (std::size_t source = 0; source < sourceCount; ++source) {
        latentViews_[source] = channel.sources[source].latent;
    }
    mixLatents(latentViews_, weights, channel.mixedLatent);

    if (channel.decoder != nullptr) {
        channel.decoder->decode(channel.mixedLatent, channel.decodedModelHop);
    } else {
        std::fill(channel.decodedModelHop.begin(), channel.decodedModelHop.end(), 0.0F);
    }
    resampleLinear(channel.decodedModelHop, channel.decodedHostHop);

    float drySourceGain = 0.0F;
    float wetGain = 1.0F;
    float dryTargetGain = 0.0F;
    endpointGains(mix, drySourceGain, wetGain, dryTargetGain);

    const auto hop = static_cast<int>(channel.decodedHostHop.size());
    for (int i = 0; i < hop; ++i) {
        const auto drySource =
            channel.sources.empty() ? 0.0F : channel.sources[0].hostHop[static_cast<std::size_t>(i)];
        const auto dryTarget = channel.sources.size() > 1
                                   ? channel.sources[1].hostHop[static_cast<std::size_t>(i)]
                                   : 0.0F;
        const auto wet = channel.decodedHostHop[static_cast<std::size_t>(i)];
        pushOutput(channel, drySourceGain * drySource + wetGain * wet + dryTargetGain * dryTarget);
    }
}

void RaveInterpolator::process(const std::span<const ConstAudioView> sources,
                               const std::span<const float> weights, const AudioView output) {
    if (output.channels == nullptr || output.numChannels <= 0 || output.numSamples <= 0) {
        return;
    }

    if (prototype_ == nullptr || channels_.empty() || hostHop_ <= 0) {
        for (int channel = 0; channel < output.numChannels; ++channel) {
            if (output.channels[channel] != nullptr) {
                std::fill_n(output.channels[channel], output.numSamples, 0.0F);
            }
        }
        return;
    }

    const auto mix = std::clamp(mixFromWeights(weights), 0.0F, 1.0F);
    const auto channelsToProcess = std::min(output.numChannels, static_cast<int>(channels_.size()));

    for (int channel = 0; channel < channelsToProcess; ++channel) {
        auto& state = channels_[static_cast<std::size_t>(channel)];
        auto* out = output.channels[channel];
        if (out == nullptr) {
            continue;
        }

        for (int sample = 0; sample < output.numSamples; ++sample) {
            const auto sourceCount = state.sources.size();
            for (std::size_t source = 0; source < sourceCount; ++source) {
                const auto view = source < sources.size() ? sources[source] : ConstAudioView{};
                state.sources[source].hostHop[static_cast<std::size_t>(state.hopFill)] =
                    sampleOrZero(view, channel, sample);
            }

            ++state.hopFill;
            if (state.hopFill >= hostHop_) {
                processHop(state, weights, mix);
                state.hopFill = 0;
            }

            out[sample] = popOutput(state);
        }
    }

    for (int channel = channelsToProcess; channel < output.numChannels; ++channel) {
        if (output.channels[channel] != nullptr) {
            std::fill_n(output.channels[channel], output.numSamples, 0.0F);
        }
    }
}

} // namespace interpolation_lab
