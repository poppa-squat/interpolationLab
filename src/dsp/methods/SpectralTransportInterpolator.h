#pragma once

#include "dsp/AnalysisSettings.h"
#include "dsp/Fft.h"
#include "dsp/Interpolator.h"
#include "dsp/OptimalTransport.h"

#include <complex>
#include <vector>

namespace interpolation_lab {

class SpectralTransportInterpolator final : public Interpolator {
  public:
    void prepare(const ProcessSpec& spec) override;
    void reset() override;
    [[nodiscard]] auto latencySamples() const -> int override;
    void process(std::span<const ConstAudioView> sources, std::span<const float> weights,
                 AudioView output) override;

    [[nodiscard]] auto settings() const -> const AnalysisSettings& { return settings_; }

  private:
    struct ChannelState {
        std::vector<float> sourceRing;
        std::vector<float> targetRing;
        std::vector<float> drySource;
        std::vector<float> dryTarget;
        std::vector<float> ola;
        std::vector<float> outputRing;
        std::vector<double> phases;
        int writePos{0};
        int dryWritePos{0};
        int outputWritePos{0};
        int outputReadPos{0};
        int outputCount{0};
        int samplesSeen{0};
        int samplesSinceHop{0};
    };

    void resetChannel(ChannelState& channel);
    void pushOutput(ChannelState& channel, float sample);
    auto popOutput(ChannelState& channel) -> float;
    void processHop(ChannelState& channel, float mix);
    void analyze(const std::vector<float>& window, std::vector<SpectralPoint>& spectrum);
    void addHopToOla(ChannelState& channel, const std::vector<SpectralPoint>& spectrum);

    AnalysisSettings settings_{};
    int numChannels_{0};
    int maxBlockSize_{0};
    RealFft fft_;
    std::vector<ChannelState> channels_;
    std::vector<float> time_;
    std::vector<float> timeDerivative_;
    std::vector<float> inverseTime_;
    std::vector<std::complex<float>> bins_;
    std::vector<std::complex<float>> binsDerivative_;
    std::vector<std::complex<float>> binsInverse_;
    std::vector<float> sourceWindow_;
    std::vector<float> targetWindow_;
    std::vector<SpectralPoint> sourceSpectrum_;
    std::vector<SpectralPoint> targetSpectrum_;
};

} // namespace interpolation_lab
