#pragma once

#include <memory>
#include <span>
#include <string>

namespace interpolation_lab {

inline constexpr auto kRaveModelEnvVar = "INTERPOLATION_LAB_RAVE_MODEL";

// Invertible hop codec: one latent frame <-> one hop of mono audio at `sampleRate()`.
class AudioAutoencoder {
  public:
    virtual ~AudioAutoencoder() = default;

    virtual void matchHostSampleRate(double hostSampleRate) = 0;
    [[nodiscard]] virtual auto sampleRate() const -> double = 0;
    [[nodiscard]] virtual auto hopSize() const -> int = 0;
    [[nodiscard]] virtual auto latentSize() const -> int = 0;
    virtual void reset() = 0;
    virtual void encode(std::span<const float> audio, std::span<float> latent) = 0;
    virtual void decode(std::span<const float> latent, std::span<float> audio) = 0;
    [[nodiscard]] virtual auto clone() const -> std::unique_ptr<AudioAutoencoder> = 0;
};

auto tryLoadRaveModelFromEnv() -> std::unique_ptr<AudioAutoencoder>;
auto loadRaveTorchScript(const std::string& path) -> std::unique_ptr<AudioAutoencoder>;

} // namespace interpolation_lab
