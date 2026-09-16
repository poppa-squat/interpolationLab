#include "dsp/methods/SpectralTransportInterpolator.h"

#include "dsp/EqualPower.h"
#include "dsp/Weights.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>

namespace interpolation_lab {
namespace {

auto hann(const double n, const double windowSize) -> double {
    return 0.5 + 0.5 * std::cos(2.0 * std::numbers::pi * n / (windowSize - 1.0));
}

auto hannDerivative(const double n, const double windowSize, const double sampleRate) -> double {
    return -(std::numbers::pi * sampleRate) / (windowSize - 1.0) *
           std::sin(2.0 * std::numbers::pi * n / (windowSize - 1.0));
}

void endpointGains(const float mix, float& drySourceGain, float& transportGain,
                   float& dryTargetGain) {
    drySourceGain = 0.0F;
    transportGain = 1.0F;
    dryTargetGain = 0.0F;
    if (mix <= kEndpointWidth) {
        const auto t = kEndpointWidth > 0.0F ? mix / kEndpointWidth : 1.0F;
        equalPowerGains(t, drySourceGain, transportGain);
        return;
    }
    if (mix >= 1.0F - kEndpointWidth) {
        const auto t = kEndpointWidth > 0.0F ? (mix - (1.0F - kEndpointWidth)) / kEndpointWidth : 1.0F;
        equalPowerGains(t, transportGain, dryTargetGain);
    }
}

auto mixFromWeights(const std::span<const float> weights) -> float {
    return weights.size() > 1 ? weights[1] : 0.0F;
}

} // namespace

void SpectralTransportInterpolator::prepare(const ProcessSpec& spec) {
    settings_ = AnalysisSettings::fromSampleRate(spec.sampleRate);
    numChannels_ = std::max(1, spec.numChannels);
    maxBlockSize_ = std::max(1, spec.maxBlockSize);
    fft_.setSize(settings_.fftSize);

    const auto bins = static_cast<std::size_t>(settings_.binCount());
    const auto fft = static_cast<std::size_t>(settings_.fftSize);
    const auto window = static_cast<std::size_t>(settings_.windowSize);
    time_.assign(fft, 0.0F);
    timeDerivative_.assign(fft, 0.0F);
    inverseTime_.assign(fft, 0.0F);
    bins_.assign(bins, {});
    binsDerivative_.assign(bins, {});
    binsInverse_.assign(bins, {});
    sourceWindow_.assign(window, 0.0F);
    targetWindow_.assign(window, 0.0F);
    sourceSpectrum_.assign(bins, {});
    targetSpectrum_.assign(bins, {});

    channels_.assign(static_cast<std::size_t>(numChannels_), {});
    reset();
}

void SpectralTransportInterpolator::reset() {
    for (auto& channel : channels_) {
        resetChannel(channel);
    }
}

auto SpectralTransportInterpolator::latencySamples() const -> int { return settings_.windowSize; }

void SpectralTransportInterpolator::resetChannel(ChannelState& channel) {
    const auto window = static_cast<std::size_t>(std::max(1, settings_.windowSize));
    const auto bins = static_cast<std::size_t>(std::max(1, settings_.binCount()));
    const auto outputSize =
        static_cast<std::size_t>(settings_.windowSize + maxBlockSize_ + settings_.hopSize + 8);

    channel.sourceRing.assign(window, 0.0F);
    channel.targetRing.assign(window, 0.0F);
    channel.drySource.assign(window, 0.0F);
    channel.dryTarget.assign(window, 0.0F);
    channel.ola.assign(window, 0.0F);
    channel.outputRing.assign(std::max(outputSize, window), 0.0F);
    channel.phases.assign(bins, 0.0);
    channel.writePos = 0;
    channel.dryWritePos = 0;
    channel.outputWritePos = 0;
    channel.outputReadPos = 0;
    channel.outputCount = 0;
    channel.samplesSeen = 0;
    channel.samplesSinceHop = 0;

    for (int i = 0; i < settings_.windowSize; ++i) {
        pushOutput(channel, 0.0F);
    }
}

void SpectralTransportInterpolator::pushOutput(ChannelState& channel, const float sample) {
    if (channel.outputRing.empty()) {
        return;
    }
    channel.outputRing[static_cast<std::size_t>(channel.outputWritePos)] = sample;
    channel.outputWritePos =
        (channel.outputWritePos + 1) % static_cast<int>(channel.outputRing.size());
    ++channel.outputCount;
}

auto SpectralTransportInterpolator::popOutput(ChannelState& channel) -> float {
    if (channel.outputRing.empty() || channel.outputCount <= 0) {
        return 0.0F;
    }
    const auto sample = channel.outputRing[static_cast<std::size_t>(channel.outputReadPos)];
    channel.outputReadPos =
        (channel.outputReadPos + 1) % static_cast<int>(channel.outputRing.size());
    --channel.outputCount;
    return sample;
}

void SpectralTransportInterpolator::process(const std::span<const ConstAudioView> sources,
                                            const std::span<const float> weights,
                                            const AudioView output) {
    if (output.channels == nullptr || output.numChannels <= 0 || output.numSamples <= 0 ||
        channels_.empty()) {
        return;
    }

    const ConstAudioView source = sources.empty() ? ConstAudioView{} : sources[0];
    const ConstAudioView target = sources.size() > 1 ? sources[1] : ConstAudioView{};
    const auto mixClamped = std::clamp(mixFromWeights(weights), 0.0F, 1.0F);
    const auto channelsToProcess = std::min(output.numChannels, static_cast<int>(channels_.size()));

    for (int channel = 0; channel < channelsToProcess; ++channel) {
        auto& state = channels_[static_cast<std::size_t>(channel)];
        auto* out = output.channels[channel];
        if (out == nullptr) {
            continue;
        }

        for (int sample = 0; sample < output.numSamples; ++sample) {
            const auto sourceSample = sampleOrZero(source, channel, sample);
            const auto targetSample = sampleOrZero(target, channel, sample);

            state.sourceRing[static_cast<std::size_t>(state.writePos)] = sourceSample;
            state.targetRing[static_cast<std::size_t>(state.writePos)] = targetSample;
            state.writePos = (state.writePos + 1) % settings_.windowSize;

            state.drySource[static_cast<std::size_t>(state.dryWritePos)] = sourceSample;
            state.dryTarget[static_cast<std::size_t>(state.dryWritePos)] = targetSample;
            state.dryWritePos = (state.dryWritePos + 1) % settings_.windowSize;

            ++state.samplesSeen;
            if (state.samplesSeen >= settings_.windowSize) {
                ++state.samplesSinceHop;
                if (state.samplesSeen == settings_.windowSize ||
                    state.samplesSinceHop >= settings_.hopSize) {
                    state.samplesSinceHop = 0;
                    processHop(state, mixClamped);
                }
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

void SpectralTransportInterpolator::processHop(ChannelState& channel, const float mix) {
    const auto window = settings_.windowSize;
    for (int i = 0; i < window; ++i) {
        const auto index = (channel.writePos + i) % window;
        sourceWindow_[static_cast<std::size_t>(i)] =
            channel.sourceRing[static_cast<std::size_t>(index)];
        targetWindow_[static_cast<std::size_t>(i)] =
            channel.targetRing[static_cast<std::size_t>(index)];
    }

    analyze(sourceWindow_, sourceSpectrum_);
    analyze(targetWindow_, targetSpectrum_);
    const auto interpolated = interpolateSpectrum(sourceSpectrum_, targetSpectrum_, channel.phases,
                                                  settings_.windowSeconds(), static_cast<double>(mix));
    addHopToOla(channel, interpolated);

    float drySourceGain = 0.0F;
    float transportGain = 1.0F;
    float dryTargetGain = 0.0F;
    endpointGains(mix, drySourceGain, transportGain, dryTargetGain);

    for (int i = 0; i < settings_.hopSize; ++i) {
        const auto dryIndex = (channel.dryWritePos + i) % window;
        const auto drySource = channel.drySource[static_cast<std::size_t>(dryIndex)];
        const auto dryTarget = channel.dryTarget[static_cast<std::size_t>(dryIndex)];
        const auto transported = channel.ola[static_cast<std::size_t>(i)];
        pushOutput(channel, drySourceGain * drySource + transportGain * transported +
                                dryTargetGain * dryTarget);
    }

    const auto remaining = window - settings_.hopSize;
    std::copy(channel.ola.begin() + settings_.hopSize, channel.ola.begin() + window,
              channel.ola.begin());
    std::fill(channel.ola.begin() + remaining, channel.ola.end(), 0.0F);
}

void SpectralTransportInterpolator::analyze(const std::vector<float>& window,
                                            std::vector<SpectralPoint>& spectrum) {
    std::fill(time_.begin(), time_.end(), 0.0F);
    std::fill(timeDerivative_.begin(), timeDerivative_.end(), 0.0F);

    const auto nWindow = static_cast<double>(settings_.windowSize);
    const auto padding = settings_.paddingSamples();
    for (int i = 0; i < settings_.windowSize; ++i) {
        const auto n = static_cast<double>(i) - (nWindow - 1.0) / 2.0;
        const auto sample = static_cast<double>(window[static_cast<std::size_t>(i)]);
        const auto index = static_cast<std::size_t>(i + padding);
        time_[index] = static_cast<float>(sample * hann(n, nWindow));
        timeDerivative_[index] =
            static_cast<float>(sample * hannDerivative(n, nWindow, settings_.sampleRate));
    }

    fft_.forward(time_, bins_);
    fft_.forward(timeDerivative_, binsDerivative_);

    const auto binCount = settings_.binCount();
    spectrum.resize(static_cast<std::size_t>(binCount));
    for (int i = 0; i < binCount; ++i) {
        const auto x = static_cast<std::complex<double>>(bins_[static_cast<std::size_t>(i)]);
        const auto xDeriv =
            static_cast<std::complex<double>>(binsDerivative_[static_cast<std::size_t>(i)]);
        const auto freq = (2.0 * std::numbers::pi * static_cast<double>(i) * settings_.sampleRate) /
                          static_cast<double>(settings_.fftSize);

        auto& point = spectrum[static_cast<std::size_t>(i)];
        point.value = x;
        point.freq = freq;
        const auto norm = std::norm(x);
        if (norm <= 1.0e-20) {
            point.freqReassigned = freq;
            continue;
        }
        const auto conjOverNorm = std::conj(x) / norm;
        const auto dphaseDt = -std::imag(xDeriv * conjOverNorm);
        point.freqReassigned = freq + dphaseDt;
    }
}

void SpectralTransportInterpolator::addHopToOla(ChannelState& channel,
                                                const std::vector<SpectralPoint>& spectrum) {
    std::fill(binsInverse_.begin(), binsInverse_.end(), std::complex<float>{});
    const auto count = std::min(spectrum.size(), binsInverse_.size());
    for (std::size_t i = 0; i < count; ++i) {
        binsInverse_[i] = static_cast<std::complex<float>>(spectrum[i].value);
    }
    fft_.inverse(binsInverse_, inverseTime_);

    const auto padding = settings_.paddingSamples();
    for (int i = 0; i < settings_.windowSize; ++i) {
        channel.ola[static_cast<std::size_t>(i)] +=
            inverseTime_[static_cast<std::size_t>(i + padding)];
    }
}

} // namespace interpolation_lab
