#pragma once

#include "dsp/AudioAutoencoder.h"
#include "dsp/Interpolator.h"

#include <memory>
#include <span>
#include <vector>

namespace interpolation_lab {

class RaveInterpolator final : public Interpolator {
  public:
    RaveInterpolator();
    explicit RaveInterpolator(std::unique_ptr<AudioAutoencoder> encoder);

    void prepare(const ProcessSpec& spec) override;
    void reset() override;
    [[nodiscard]] auto latencySamples() const -> int override;
    void process(std::span<const ConstAudioView> sources, std::span<const float> weights,
                 AudioView output) override;

    [[nodiscard]] auto hasAutoencoder() const -> bool;
    [[nodiscard]] auto hopSize() const -> int;
    [[nodiscard]] auto hostHopSize() const -> int;

  private:
    struct SourceState {
        std::unique_ptr<AudioAutoencoder> encoder;
        std::vector<float> hostHop;
        std::vector<float> modelHop;
        std::vector<float> latent;
    };

    struct ChannelState {
        std::vector<SourceState> sources;
        std::unique_ptr<AudioAutoencoder> decoder;
        std::vector<float> mixedLatent;
        std::vector<float> decodedModelHop;
        std::vector<float> decodedHostHop;
        std::vector<float> outputRing;
        int hopFill{0};
        int outputWritePos{0};
        int outputReadPos{0};
        int outputCount{0};
    };

    void resetChannel(ChannelState& channel);
    void pushOutput(ChannelState& channel, float sample);
    auto popOutput(ChannelState& channel) -> float;
    void processHop(ChannelState& channel, std::span<const float> weights, float mix);

    std::unique_ptr<AudioAutoencoder> prototype_;
    std::vector<ChannelState> channels_;
    std::vector<std::span<const float>> latentViews_;
    int numChannels_{0};
    int maxBlockSize_{0};
    int maxSources_{2};
    int hopSize_{0};
    int hostHop_{0};
    int latentSize_{0};
    int latency_{0};
    double hostSampleRate_{44100.0};
    double modelSampleRate_{44100.0};
};

} // namespace interpolation_lab
