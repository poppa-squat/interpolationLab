#pragma once

#include <cmath>
#include <numbers>

namespace interpolation_lab {

inline void equalPowerGains(const float mix, float& sourceGain, float& targetGain) {
    const auto clamped = mix < 0.0F ? 0.0F : (mix > 1.0F ? 1.0F : mix);
    const auto angle = clamped * (0.5F * std::numbers::pi_v<float>);
    sourceGain = std::cos(angle);
    targetGain = std::sin(angle);
}

} // namespace interpolation_lab
