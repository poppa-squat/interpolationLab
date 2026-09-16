#include "dsp/InterpolatorCatalog.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("catalog lists fade first and spectral transport second") {
    REQUIRE(interpolation_lab::InterpolatorCatalog::size() >= 2);
    const auto& fade = interpolation_lab::InterpolatorCatalog::info(0);
    REQUIRE(fade.id == "fade");
    REQUIRE(fade.name == "Fade");
    const auto& spectral = interpolation_lab::InterpolatorCatalog::info(1);
    REQUIRE(spectral.id == "spectral_transport");
    REQUIRE(spectral.name == "Spectral Transport");
}

TEST_CASE("catalog create returns a prepared interpolator") {
    auto fade = interpolation_lab::InterpolatorCatalog::create(0);
    REQUIRE(fade != nullptr);
    fade->prepare({44100.0, 64, 2, 2});
    REQUIRE(fade->latencySamples() == 0);

    auto spectral = interpolation_lab::InterpolatorCatalog::create(1);
    REQUIRE(spectral != nullptr);
    spectral->prepare({44100.0, 64, 2, 2});
    REQUIRE(spectral->latencySamples() > 0);
}

TEST_CASE("catalog clamps out-of-range indices") {
    const auto lastIndex = interpolation_lab::InterpolatorCatalog::size() - 1;
    const auto& last = interpolation_lab::InterpolatorCatalog::info(lastIndex);
    const auto& clamped = interpolation_lab::InterpolatorCatalog::info(99);
    REQUIRE(clamped.id == last.id);
}
