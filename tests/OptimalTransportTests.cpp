#include "dsp/OptimalTransport.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <complex>
#include <tuple>
#include <vector>

using interpolation_lab::SpectralMass;
using interpolation_lab::SpectralPoint;
using interpolation_lab::groupSpectrum;
using interpolation_lab::transportMatrix;

TEST_CASE("group spectrum splits on reassignment zero crossings") {
    std::vector<SpectralPoint> spectrum(6);
    for (std::size_t i = 0; i < spectrum.size(); ++i) {
        spectrum[i].value = 1.0;
        spectrum[i].freq = static_cast<double>(i);
    }
    // Rising, falling (center), rising (boundary), falling (center), ...
    spectrum[0].freqReassigned = -0.5;
    spectrum[1].freqReassigned = 1.4;  // rise: new region continues
    spectrum[2].freqReassigned = 1.6;  // fall: center
    spectrum[3].freqReassigned = 4.0;  // rise: boundary
    spectrum[4].freqReassigned = 3.6;  // fall: center
    spectrum[5].freqReassigned = 6.0;  // rise: would be next boundary at end

    const auto masses = groupSpectrum(spectrum);
    REQUIRE(masses.size() >= 2);
    auto total = 0.0;
    for (const auto& mass : masses) {
        total += mass.mass;
        REQUIRE(mass.leftBin < mass.rightBin);
        REQUIRE(mass.centerBin >= mass.leftBin);
        REQUIRE(mass.centerBin < mass.rightBin);
    }
    REQUIRE(total == Catch::Approx(1.0).margin(1.0e-9));
}

TEST_CASE("transport matrix conserves unit mass") {
    const std::vector<SpectralMass> source{
        {0, 2, 1, 0.25},
        {2, 5, 3, 0.75},
    };
    const std::vector<SpectralMass> target{
        {0, 1, 0, 0.5},
        {1, 4, 2, 0.5},
    };
    const auto assignments = transportMatrix(source, target);
    auto total = 0.0;
    for (const auto& assignment : assignments) {
        total += std::get<2>(assignment);
        REQUIRE(std::get<2>(assignment) >= 0.0);
    }
    REQUIRE(total == Catch::Approx(1.0).margin(1.0e-12));
}

TEST_CASE("silent spectrum yields a single zero mass") {
    std::vector<SpectralPoint> spectrum(4);
    for (std::size_t i = 0; i < spectrum.size(); ++i) {
        spectrum[i].freq = static_cast<double>(i);
        spectrum[i].freqReassigned = static_cast<double>(i);
        spectrum[i].value = 0.0;
    }
    const auto masses = groupSpectrum(spectrum);
    REQUIRE(masses.size() == 1);
    REQUIRE(masses.front().mass == Catch::Approx(0.0));
}
