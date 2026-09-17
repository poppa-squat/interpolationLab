#pragma once

#include <algorithm>
#include <span>
#include <vector>

namespace interpolation_lab {

// Mix aligned latent frames with barycentric weights: out = Σ w_i frames[i].
// Extra frames or weights are ignored. Negative weights are treated as 0.
// `out` is filled with zeros when there is nothing to mix.
inline void mixLatents(const std::span<const std::span<const float>> frames,
                       const std::span<const float> weights, const std::span<float> out) {
    std::fill(out.begin(), out.end(), 0.0F);
    if (out.empty()) {
        return;
    }

    const auto count = std::min(frames.size(), weights.size());
    for (std::size_t source = 0; source < count; ++source) {
        const auto weight = weights[source] < 0.0F ? 0.0F : weights[source];
        if (weight == 0.0F) {
            continue;
        }
        const auto& frame = frames[source];
        const auto dim = std::min(out.size(), frame.size());
        for (std::size_t i = 0; i < dim; ++i) {
            out[i] += weight * frame[i];
        }
    }
}

inline void mixLatents(const std::vector<std::span<const float>>& frames,
                       const std::span<const float> weights, const std::span<float> out) {
    mixLatents(std::span<const std::span<const float>>{frames.data(), frames.size()}, weights, out);
}

inline void mixLatentFrames(const std::vector<std::vector<float>>& frames,
                            const std::span<const float> weights, std::vector<float>& out) {
    std::vector<std::span<const float>> views(frames.size());
    for (std::size_t i = 0; i < frames.size(); ++i) {
        views[i] = frames[i];
    }
    mixLatents(views, weights, out);
}

} // namespace interpolation_lab
