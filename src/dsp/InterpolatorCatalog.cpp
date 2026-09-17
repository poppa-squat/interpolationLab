#include "dsp/InterpolatorCatalog.h"

#include "dsp/methods/FadeInterpolator.h"
#include "dsp/methods/RaveInterpolator.h"
#include "dsp/methods/SpectralTransportInterpolator.h"

#include <algorithm>
#include <array>

namespace interpolation_lab {
namespace {

auto makeFade() -> std::unique_ptr<Interpolator> { return std::make_unique<FadeInterpolator>(); }

auto makeSpectralTransport() -> std::unique_ptr<Interpolator> {
    return std::make_unique<SpectralTransportInterpolator>();
}

auto makeRave() -> std::unique_ptr<Interpolator> { return std::make_unique<RaveInterpolator>(); }

const std::array<InterpolatorInfo, 3> kEntries{{
    {"fade", "Fade", &makeFade},
    {"spectral_transport", "Spectral Transport", &makeSpectralTransport},
    {"rave", "RAVE Latent", &makeRave},
}};

} // namespace

auto InterpolatorCatalog::size() -> int { return static_cast<int>(kEntries.size()); }

auto InterpolatorCatalog::info(const int index) -> const InterpolatorInfo& {
    const auto clamped = std::clamp(index, 0, size() - 1);
    return kEntries[static_cast<std::size_t>(clamped)];
}

auto InterpolatorCatalog::create(const int index) -> std::unique_ptr<Interpolator> {
    return info(index).create();
}

} // namespace interpolation_lab
