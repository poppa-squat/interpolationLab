#include "dsp/LatentMath.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <span>
#include <vector>

TEST_CASE("mixLatents is identity for a single weight of one") {
    const std::vector<float> source{1.0F, -2.0F, 3.5F};
    const std::vector<float> target{9.0F, 8.0F, 7.0F};
    std::array<std::span<const float>, 2> frames{source, target};
    const float weights[] = {1.0F, 0.0F};
    std::vector<float> out(3, 0.0F);
    interpolation_lab::mixLatents(frames, weights, out);
    REQUIRE(out[0] == Catch::Approx(1.0F));
    REQUIRE(out[1] == Catch::Approx(-2.0F));
    REQUIRE(out[2] == Catch::Approx(3.5F));
}

TEST_CASE("mixLatents is barycentric for two frames") {
    const std::vector<float> source{0.0F, 2.0F};
    const std::vector<float> target{4.0F, 6.0F};
    std::array<std::span<const float>, 2> frames{source, target};
    const float weights[] = {0.25F, 0.75F};
    std::vector<float> out(2, 0.0F);
    interpolation_lab::mixLatents(frames, weights, out);
    REQUIRE(out[0] == Catch::Approx(3.0F));
    REQUIRE(out[1] == Catch::Approx(5.0F));
}

TEST_CASE("mixLatents ignores extra sources at weight 0 and negative weights") {
    const std::vector<float> a{1.0F};
    const std::vector<float> b{3.0F};
    const std::vector<float> extra{100.0F};
    const std::vector<std::vector<float>> frames{a, b, extra};
    const float weights[] = {0.5F, 0.5F, 0.0F};
    std::vector<float> out{0.0F};
    interpolation_lab::mixLatentFrames(frames, weights, out);
    REQUIRE(out[0] == Catch::Approx(2.0F));

    const float negative[] = {-1.0F, 1.0F, 1.0F};
    interpolation_lab::mixLatentFrames(frames, negative, out);
    REQUIRE(out[0] == Catch::Approx(103.0F));
}

TEST_CASE("mixLatents zeros an empty or unmatched output") {
    std::vector<float> out{};
    interpolation_lab::mixLatents(std::span<const std::span<const float>>{},
                                  std::span<const float>{}, out);
    REQUIRE(out.empty());

    const std::vector<float> source{1.0F, 2.0F};
    std::array<std::span<const float>, 1> frames{source};
    const float weights[] = {1.0F};
    std::vector<float> shortOut(1, 99.0F);
    interpolation_lab::mixLatents(frames, weights, shortOut);
    REQUIRE(shortOut[0] == Catch::Approx(1.0F));
}
