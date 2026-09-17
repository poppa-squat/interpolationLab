#include "dsp/DummyAudioAutoencoder.h"
#include "dsp/methods/RaveInterpolator.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <memory>
#include <vector>

using interpolation_lab::AudioView;
using interpolation_lab::ConstAudioView;
using interpolation_lab::DummyAudioAutoencoder;
using interpolation_lab::ProcessSpec;
using interpolation_lab::RaveInterpolator;

namespace {

void processSources(RaveInterpolator& interpolator, const std::vector<std::vector<float>>& sources,
                    const std::vector<float>& weights, std::vector<float>& output) {
    std::vector<const float*> sourcePtrs(sources.size(), nullptr);
    std::vector<ConstAudioView> views(sources.size());
    for (std::size_t i = 0; i < sources.size(); ++i) {
        sourcePtrs[i] = sources[i].data();
        views[i] = ConstAudioView{&sourcePtrs[i], 1, static_cast<int>(sources[i].size())};
    }
    float* outputPtr = output.data();
    interpolator.process(views, weights, AudioView{&outputPtr, 1, static_cast<int>(output.size())});
}

auto meanAbsError(const std::vector<float>& output, const std::vector<float>& expected,
                  const int latency) -> float {
    auto error = 0.0F;
    auto count = 0;
    for (int i = latency; i < static_cast<int>(output.size()); ++i) {
        error += std::abs(output[static_cast<std::size_t>(i)] -
                          expected[static_cast<std::size_t>(i - latency)]);
        ++count;
    }
    REQUIRE(count > 0);
    return error / static_cast<float>(count);
}

auto makeDummy() -> std::unique_ptr<DummyAudioAutoencoder> {
    return std::make_unique<DummyAudioAutoencoder>();
}

} // namespace

TEST_CASE("rave dummy mix 0 matches delayed source") {
    RaveInterpolator interpolator(makeDummy());
    interpolator.prepare(ProcessSpec{44100.0, 64, 1, 2});
    const auto hop = interpolator.hostHopSize();
    const auto samples = hop * 6;
    std::vector<float> source(static_cast<std::size_t>(samples));
    std::vector<float> target(static_cast<std::size_t>(samples));
    for (int i = 0; i < samples; ++i) {
        source[static_cast<std::size_t>(i)] = 0.01F * static_cast<float>(i);
        target[static_cast<std::size_t>(i)] = 0.9F;
    }
    std::vector<float> output(static_cast<std::size_t>(samples), 0.0F);
    processSources(interpolator, {source, target}, {1.0F, 0.0F}, output);

    REQUIRE(interpolator.latencySamples() == hop);
    REQUIRE(interpolator.hasAutoencoder());
    REQUIRE(meanAbsError(output, source, hop) < 1.0e-5F);
}

TEST_CASE("rave dummy mix 1 matches delayed target") {
    RaveInterpolator interpolator(makeDummy());
    interpolator.prepare(ProcessSpec{44100.0, 64, 1, 2});
    const auto hop = interpolator.hostHopSize();
    const auto samples = hop * 6;
    std::vector<float> source(static_cast<std::size_t>(samples), 0.2F);
    std::vector<float> target(static_cast<std::size_t>(samples));
    for (int i = 0; i < samples; ++i) {
        target[static_cast<std::size_t>(i)] = -0.02F * static_cast<float>(i);
    }
    std::vector<float> output(static_cast<std::size_t>(samples), 0.0F);
    processSources(interpolator, {source, target}, {0.0F, 1.0F}, output);
    REQUIRE(meanAbsError(output, target, hop) < 1.0e-5F);
}

TEST_CASE("rave dummy mix 0.5 is a barycentric hop mix") {
    RaveInterpolator interpolator(makeDummy());
    interpolator.prepare(ProcessSpec{44100.0, 64, 1, 2});
    const auto hop = interpolator.hostHopSize();
    const auto samples = hop * 8;
    const std::vector<float> source(static_cast<std::size_t>(samples), 0.25F);
    const std::vector<float> target(static_cast<std::size_t>(samples), 0.75F);
    std::vector<float> output(static_cast<std::size_t>(samples), 0.0F);
    processSources(interpolator, {source, target}, {0.5F, 0.5F}, output);

    const auto latency = interpolator.latencySamples();
    auto error = 0.0F;
    auto count = 0;
    for (int i = latency; i < samples; ++i) {
        error += std::abs(output[static_cast<std::size_t>(i)] - 0.5F);
        ++count;
    }
    REQUIRE(count > 0);
    REQUIRE(error / static_cast<float>(count) < 1.0e-5F);
}

TEST_CASE("rave dummy ignores extra sources at weight 0") {
    RaveInterpolator interpolator(makeDummy());
    interpolator.prepare(ProcessSpec{44100.0, 64, 1, 3});
    const auto hop = interpolator.hostHopSize();
    const auto samples = hop * 6;
    const std::vector<float> source(static_cast<std::size_t>(samples), 0.25F);
    const std::vector<float> target(static_cast<std::size_t>(samples), 0.75F);
    const std::vector<float> extra(static_cast<std::size_t>(samples), 10.0F);
    std::vector<float> output(static_cast<std::size_t>(samples), 0.0F);
    processSources(interpolator, {source, target, extra}, {0.5F, 0.5F, 0.0F}, output);

    const auto latency = interpolator.latencySamples();
    auto error = 0.0F;
    auto count = 0;
    for (int i = latency; i < samples; ++i) {
        error += std::abs(output[static_cast<std::size_t>(i)] - 0.5F);
        ++count;
    }
    REQUIRE(count > 0);
    REQUIRE(error / static_cast<float>(count) < 1.0e-5F);
}

TEST_CASE("rave dummy copies a mono target across stereo") {
    RaveInterpolator interpolator(makeDummy());
    interpolator.prepare(ProcessSpec{44100.0, 64, 2, 2});
    const auto hop = interpolator.hostHopSize();
    const auto samples = hop * 4;
    std::vector<float> sourceL(static_cast<std::size_t>(samples), 0.0F);
    std::vector<float> sourceR(static_cast<std::size_t>(samples), 0.0F);
    std::vector<float> target(static_cast<std::size_t>(samples), 0.4F);
    std::vector<float> outL(static_cast<std::size_t>(samples), 0.0F);
    std::vector<float> outR(static_cast<std::size_t>(samples), 0.0F);

    const float* sourceChannels[] = {sourceL.data(), sourceR.data()};
    const float* targetChannels[] = {target.data()};
    float* outputChannels[] = {outL.data(), outR.data()};
    const ConstAudioView sources[] = {ConstAudioView{sourceChannels, 2, samples},
                                      ConstAudioView{targetChannels, 1, samples}};
    const float weights[] = {0.0F, 1.0F};
    interpolator.process(sources, weights, AudioView{outputChannels, 2, samples});

    REQUIRE(meanAbsError(outL, target, hop) < 1.0e-5F);
    REQUIRE(meanAbsError(outR, target, hop) < 1.0e-5F);
}

TEST_CASE("rave dummy mix 0 still matches after host/model resample") {
    auto dummy = std::make_unique<DummyAudioAutoencoder>();
    dummy->lockSampleRate(48000.0);
    RaveInterpolator interpolator(std::move(dummy));
    interpolator.prepare(ProcessSpec{44100.0, 64, 1, 2});
    REQUIRE(interpolator.hostHopSize() != interpolator.hopSize());

    const auto hop = interpolator.hostHopSize();
    const auto samples = hop * 6;
    std::vector<float> source(static_cast<std::size_t>(samples));
    const std::vector<float> target(static_cast<std::size_t>(samples), -1.0F);
    for (int i = 0; i < samples; ++i) {
        source[static_cast<std::size_t>(i)] = 0.03F * static_cast<float>(i % 7);
    }
    std::vector<float> output(static_cast<std::size_t>(samples), 0.0F);
    processSources(interpolator, {source, target}, {1.0F, 0.0F}, output);
    REQUIRE(meanAbsError(output, source, hop) < 1.0e-5F);
}

TEST_CASE("rave without a model is silent and reports zero latency") {
    RaveInterpolator interpolator{std::unique_ptr<interpolation_lab::AudioAutoencoder>{}};
    interpolator.prepare(ProcessSpec{44100.0, 64, 2, 2});
    REQUIRE_FALSE(interpolator.hasAutoencoder());
    REQUIRE(interpolator.latencySamples() == 0);

    const std::vector<float> source{0.5F, 0.5F, 0.5F, 0.5F};
    const std::vector<float> target{1.0F, 1.0F, 1.0F, 1.0F};
    std::vector<float> output(4, 99.0F);
    processSources(interpolator, {source, target}, {1.0F, 0.0F}, output);
    REQUIRE(output == std::vector<float>{0.0F, 0.0F, 0.0F, 0.0F});
}
