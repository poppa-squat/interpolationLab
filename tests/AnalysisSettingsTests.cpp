#include "dsp/AnalysisSettings.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("analysis settings match the paper at 44.1 kHz") {
    const auto settings = interpolation_lab::AnalysisSettings::fromSampleRate(44100.0);
    REQUIRE(settings.windowSize == 2206);
    REQUIRE(settings.hopSize == 1103);
    REQUIRE(settings.fftSize == 16384);
    REQUIRE(44100.0 / static_cast<double>(settings.fftSize) <= 5.0);
}

TEST_CASE("analysis window is even and hop is half") {
    const auto settings = interpolation_lab::AnalysisSettings::fromSampleRate(48000.0);
    REQUIRE((settings.windowSize % 2) == 0);
    REQUIRE(settings.hopSize == settings.windowSize / 2);
    REQUIRE(settings.fftSize >= settings.windowSize);
}
