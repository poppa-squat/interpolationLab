#include "dsp/InterpolatorCatalog.h"

#include "dsp/methods/FadeInterpolator.h"

#include <algorithm>
#include <array>

namespace interpolation_lab {
namespace {

auto makeFade() -> std::unique_ptr<Interpolator> { return std::make_unique<FadeInterpolator>(); }

const std::array<InterpolatorInfo, 1> kEntries{{
    {"fade", "Fade", &makeFade},
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
