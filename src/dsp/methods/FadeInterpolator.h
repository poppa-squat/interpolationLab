#pragma once

#include "dsp/Interpolator.h"

#include <vector>

namespace interpolation_lab {

class FadeInterpolator final : public Interpolator {
  public:
    void prepare(const ProcessSpec& spec) override;
    void reset() override;
    [[nodiscard]] auto latencySamples() const -> int override;
    void process(std::span<const ConstAudioView> sources, std::span<const float> weights,
                 AudioView output) override;

  private:
    void fillGains(std::span<const float> weights, std::size_t sourceCount);

    std::vector<float> gains_{};
};

} // namespace interpolation_lab
