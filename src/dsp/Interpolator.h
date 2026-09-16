#pragma once

#include <cstddef>
#include <span>

namespace interpolation_lab {

struct ProcessSpec {
    double sampleRate{44100.0};
    int maxBlockSize{512};
    int numChannels{2};
    int maxSources{2};
};

struct ConstAudioView {
    const float* const* channels{};
    int numChannels{0};
    int numSamples{0};
};

struct AudioView {
    float* const* channels{};
    int numChannels{0};
    int numSamples{0};
};

// Interpolates an arbitrary number of aligned source views.
// `weights[i]` is the barycentric mix amount for `sources[i]` (typically in [0, 1]
// and summing to 1). The plugin currently wires two buses and maps Mix to
// `{1 - mix, mix}`; extra sources can be added without changing this seam.
class Interpolator {
  public:
    virtual ~Interpolator() = default;

    virtual void prepare(const ProcessSpec& spec) = 0;
    virtual void reset() = 0;
    [[nodiscard]] virtual auto latencySamples() const -> int = 0;
    virtual void process(std::span<const ConstAudioView> sources, std::span<const float> weights,
                         AudioView output) = 0;
};

} // namespace interpolation_lab
