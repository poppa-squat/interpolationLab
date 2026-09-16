#include "dsp/InterpolatorCatalog.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("catalog lists fade first") {
    REQUIRE(interpolation_lab::InterpolatorCatalog::size() >= 1);
    const auto& fade = interpolation_lab::InterpolatorCatalog::info(0);
    REQUIRE(fade.id == "fade");
    REQUIRE(fade.name == "Fade");
}

TEST_CASE("catalog create returns a prepared interpolator") {
    auto interpolator = interpolation_lab::InterpolatorCatalog::create(0);
    REQUIRE(interpolator != nullptr);
    interpolator->prepare({44100.0, 64, 2, 2});
    REQUIRE(interpolator->latencySamples() == 0);
}

TEST_CASE("catalog clamps out-of-range indices") {
    const auto& first = interpolation_lab::InterpolatorCatalog::info(0);
    const auto& clamped = interpolation_lab::InterpolatorCatalog::info(99);
    REQUIRE(clamped.id == first.id);
}
