#include "dsp/Weights.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("two-source mix maps onto barycentric weights") {
    float source = 0.0F;
    float target = 0.0F;

    interpolation_lab::twoSourceWeights(0.0F, source, target);
    REQUIRE(source == Catch::Approx(1.0F));
    REQUIRE(target == Catch::Approx(0.0F));

    interpolation_lab::twoSourceWeights(1.0F, source, target);
    REQUIRE(source == Catch::Approx(0.0F));
    REQUIRE(target == Catch::Approx(1.0F));

    interpolation_lab::twoSourceWeights(0.25F, source, target);
    REQUIRE(source == Catch::Approx(0.75F));
    REQUIRE(target == Catch::Approx(0.25F));
}

TEST_CASE("sampleOrZero repeats the last channel and rejects empty views") {
    float left = 0.5F;
    const float* channels[] = {&left};
    const interpolation_lab::ConstAudioView view{channels, 1, 1};
    REQUIRE(interpolation_lab::sampleOrZero(view, 0, 0) == Catch::Approx(0.5F));
    REQUIRE(interpolation_lab::sampleOrZero(view, 1, 0) == Catch::Approx(0.5F));
    REQUIRE(interpolation_lab::sampleOrZero(view, 0, 2) == Catch::Approx(0.0F));
    REQUIRE(interpolation_lab::sampleOrZero({}, 0, 0) == Catch::Approx(0.0F));
}
