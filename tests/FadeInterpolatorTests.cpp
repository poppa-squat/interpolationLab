#include "dsp/methods/FadeInterpolator.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <vector>

using interpolation_lab::AudioView;
using interpolation_lab::ConstAudioView;
using interpolation_lab::FadeInterpolator;
using interpolation_lab::ProcessSpec;

namespace {

void processSources(FadeInterpolator& fade, const std::vector<std::vector<float>>& sources,
                    const std::vector<float>& weights, std::vector<float>& output) {
    std::vector<const float*> sourcePtrs(sources.size(), nullptr);
    std::vector<ConstAudioView> views(sources.size());
    for (std::size_t i = 0; i < sources.size(); ++i) {
        sourcePtrs[i] = sources[i].data();
        views[i] = ConstAudioView{&sourcePtrs[i], 1, static_cast<int>(sources[i].size())};
    }
    float* outputPtr = output.data();
    fade.process(views, weights, AudioView{&outputPtr, 1, static_cast<int>(output.size())});
}

} // namespace

TEST_CASE("fade is identity at mix extremes") {
    FadeInterpolator fade;
    fade.prepare(ProcessSpec{44100.0, 8, 1, 2});
    const std::vector<float> source{0.1F, -0.2F, 0.3F, -0.4F};
    const std::vector<float> target{0.9F, 0.8F, 0.7F, 0.6F};
    std::vector<float> output(4, 0.0F);

    processSources(fade, {source, target}, {1.0F, 0.0F}, output);
    REQUIRE(output == source);

    processSources(fade, {source, target}, {0.0F, 1.0F}, output);
    REQUIRE(output == target);
}

TEST_CASE("fade is equal-power at mix 0.5") {
    FadeInterpolator fade;
    fade.prepare(ProcessSpec{44100.0, 1, 1, 2});
    const std::vector<float> source{1.0F};
    const std::vector<float> target{1.0F};
    std::vector<float> output(1, 0.0F);
    processSources(fade, {source, target}, {0.5F, 0.5F}, output);
    const auto expected = std::sqrt(2.0F) / 2.0F * 2.0F;
    REQUIRE(output[0] == Catch::Approx(expected).margin(1.0e-5F));
}

TEST_CASE("fade duplicates a mono target across stereo") {
    FadeInterpolator fade;
    fade.prepare(ProcessSpec{44100.0, 1, 2, 2});
    std::array<float, 1> sourceL{0.0F};
    std::array<float, 1> sourceR{0.0F};
    std::array<float, 1> target{1.0F};
    std::array<float, 1> outL{0.0F};
    std::array<float, 1> outR{0.0F};
    const float* sourceChannels[] = {sourceL.data(), sourceR.data()};
    const float* targetChannels[] = {target.data()};
    float* outputChannels[] = {outL.data(), outR.data()};
    const ConstAudioView sources[] = {ConstAudioView{sourceChannels, 2, 1},
                                      ConstAudioView{targetChannels, 1, 1}};
    const float weights[] = {0.0F, 1.0F};
    fade.process(sources, weights, AudioView{outputChannels, 2, 1});
    REQUIRE(outL[0] == Catch::Approx(1.0F));
    REQUIRE(outR[0] == Catch::Approx(1.0F));
}

TEST_CASE("fade reports zero latency") {
    FadeInterpolator fade;
    REQUIRE(fade.latencySamples() == 0);
}

TEST_CASE("fade mixes three sources by weight") {
    FadeInterpolator fade;
    fade.prepare(ProcessSpec{44100.0, 1, 1, 4});
    const std::vector<float> a{1.0F};
    const std::vector<float> b{2.0F};
    const std::vector<float> c{4.0F};
    std::vector<float> output(1, 0.0F);

    processSources(fade, {a, b, c}, {1.0F, 0.0F, 0.0F}, output);
    REQUIRE(output[0] == Catch::Approx(1.0F).margin(1.0e-5F));

    processSources(fade, {a, b, c}, {0.0F, 0.0F, 1.0F}, output);
    REQUIRE(output[0] == Catch::Approx(4.0F).margin(1.0e-5F));

    processSources(fade, {a, b, c}, {1.0F / 3.0F, 1.0F / 3.0F, 1.0F / 3.0F}, output);
    const auto gain = std::sqrt(1.0F / 3.0F);
    REQUIRE(output[0] == Catch::Approx(gain * (1.0F + 2.0F + 4.0F)).margin(1.0e-5F));
}
