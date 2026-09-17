#pragma once

#include "dsp/AudioAutoencoder.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>

namespace interpolation_lab {

// Invertible diagonal projection injected by tests so Catch2 can cover the
// encode → mix → decode path without LibTorch.
// encode(x) = scale * x, decode(z) = z / scale, so a barycentric latent mix equals
// a linear mix of the audio hops.
class DummyAudioAutoencoder final : public AudioAutoencoder {
  public:
    static constexpr int kDefaultHopSize = 16;
    static constexpr float kDefaultScale = 2.0F;

    DummyAudioAutoencoder() = default;
    explicit DummyAudioAutoencoder(const int hopSize, const float scale = kDefaultScale)
        : hopSize_(std::max(1, hopSize)), scale_(scale == 0.0F ? kDefaultScale : scale) {}

    void matchHostSampleRate(const double hostSampleRate) override {
        if (lockSampleRate_) {
            return;
        }
        sampleRate_ = hostSampleRate > 0.0 ? hostSampleRate : 44100.0;
    }

    void lockSampleRate(const double sampleRate) {
        lockSampleRate_ = true;
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
    }

    [[nodiscard]] auto sampleRate() const -> double override { return sampleRate_; }
    [[nodiscard]] auto hopSize() const -> int override { return hopSize_; }
    [[nodiscard]] auto latentSize() const -> int override { return hopSize_; }
    void reset() override {}

    void encode(const std::span<const float> audio, const std::span<float> latent) override {
        const auto n = std::min(audio.size(), latent.size());
        for (std::size_t i = 0; i < n; ++i) {
            latent[i] = scale_ * audio[i];
        }
        if (n < latent.size()) {
            std::fill(latent.begin() + static_cast<std::ptrdiff_t>(n), latent.end(), 0.0F);
        }
    }

    void decode(const std::span<const float> latent, const std::span<float> audio) override {
        const auto inv = 1.0F / scale_;
        const auto n = std::min(audio.size(), latent.size());
        for (std::size_t i = 0; i < n; ++i) {
            audio[i] = inv * latent[i];
        }
        if (n < audio.size()) {
            std::fill(audio.begin() + static_cast<std::ptrdiff_t>(n), audio.end(), 0.0F);
        }
    }

    [[nodiscard]] auto clone() const -> std::unique_ptr<AudioAutoencoder> override {
        return std::make_unique<DummyAudioAutoencoder>(*this);
    }

  private:
    double sampleRate_{44100.0};
    int hopSize_{kDefaultHopSize};
    float scale_{kDefaultScale};
    bool lockSampleRate_{false};
};

} // namespace interpolation_lab
