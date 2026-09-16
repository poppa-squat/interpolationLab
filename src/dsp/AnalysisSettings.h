#pragma once

namespace interpolation_lab {

struct AnalysisSettings {
    double sampleRate{44100.0};
    int windowSize{0};
    int hopSize{0};
    int fftSize{0};

    [[nodiscard]] static auto fromSampleRate(double sampleRate) -> AnalysisSettings;
    [[nodiscard]] auto binCount() const -> int { return fftSize / 2 + 1; }
    [[nodiscard]] auto paddingSamples() const -> int { return (fftSize - windowSize) / 2; }
    [[nodiscard]] auto windowSeconds() const -> double {
        return sampleRate > 0.0 ? static_cast<double>(windowSize) / sampleRate : 0.0;
    }
};

inline constexpr double kWindowSeconds = 0.05;
inline constexpr double kTargetBinHz = 5.0;
inline constexpr float kEndpointWidth = 0.02F;

} // namespace interpolation_lab
