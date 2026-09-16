#include "dsp/methods/SpectralTransportInterpolator.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <vector>

using interpolation_lab::AudioView;
using interpolation_lab::ConstAudioView;
using interpolation_lab::ProcessSpec;
using interpolation_lab::SpectralTransportInterpolator;

namespace {

auto makeSine(const double frequency, const double sampleRate, const int samples) -> std::vector<float> {
    std::vector<float> signal(static_cast<std::size_t>(samples));
    for (int i = 0; i < samples; ++i) {
        signal[static_cast<std::size_t>(i)] = static_cast<float>(
            std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / sampleRate));
    }
    return signal;
}

auto peakHz(const std::vector<float>& signal, const double sampleRate, const int skip) -> double {
    const auto start = std::min(skip, static_cast<int>(signal.size()));
    const auto count = static_cast<int>(signal.size()) - start;
    REQUIRE(count > 64);

    auto n = 1;
    while (n < count) {
        n <<= 1;
    }
    n >>= 1;
    std::vector<std::complex<double>> bins(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        bins[static_cast<std::size_t>(i)] = signal[static_cast<std::size_t>(start + i)];
    }

    const auto logN = static_cast<int>(std::log2(n));
    for (int i = 1, reversed = 0; i < n; ++i) {
        auto bit = n >> 1;
        for (; (reversed & bit) != 0; bit >>= 1) {
            reversed ^= bit;
        }
        reversed ^= bit;
        if (i < reversed) {
            std::swap(bins[static_cast<std::size_t>(i)], bins[static_cast<std::size_t>(reversed)]);
        }
    }
    for (int length = 2; length <= n; length <<= 1) {
        const auto angle = -2.0 * std::numbers::pi / static_cast<double>(length);
        const std::complex<double> step{std::cos(angle), std::sin(angle)};
        for (int startBin = 0; startBin < n; startBin += length) {
            std::complex<double> twiddle{1.0, 0.0};
            const auto half = length >> 1;
            for (int offset = 0; offset < half; ++offset) {
                const auto even = bins[static_cast<std::size_t>(startBin + offset)];
                const auto odd = bins[static_cast<std::size_t>(startBin + offset + half)] * twiddle;
                bins[static_cast<std::size_t>(startBin + offset)] = even + odd;
                bins[static_cast<std::size_t>(startBin + offset + half)] = even - odd;
                twiddle *= step;
            }
        }
        static_cast<void>(logN);
    }

    auto best = 1;
    auto bestMag = 0.0;
    for (int bin = 1; bin < n / 2; ++bin) {
        const auto mag = std::abs(bins[static_cast<std::size_t>(bin)]);
        if (mag > bestMag) {
            bestMag = mag;
            best = bin;
        }
    }
    return static_cast<double>(best) * sampleRate / static_cast<double>(n);
}

void run(SpectralTransportInterpolator& interpolator, const std::vector<float>& source,
         const std::vector<float>& target, const float mix, std::vector<float>& output) {
    const float* sourcePtr = source.data();
    const float* targetPtr = target.data();
    float* outputPtr = output.data();
    const ConstAudioView sources[] = {ConstAudioView{&sourcePtr, 1, static_cast<int>(source.size())},
                                      ConstAudioView{&targetPtr, 1, static_cast<int>(target.size())}};
    const float weights[] = {1.0F - mix, mix};
    interpolator.process(sources, weights, AudioView{&outputPtr, 1, static_cast<int>(output.size())});
}

} // namespace

TEST_CASE("spectral transport mix 0 matches delayed source") {
    constexpr auto sampleRate = 44100.0;
    SpectralTransportInterpolator interpolator;
    interpolator.prepare(ProcessSpec{sampleRate, 512, 1, 2});
    const auto settings = interpolator.settings();
    const auto samples = settings.windowSize * 6;
    const auto source = makeSine(440.0, sampleRate, samples);
    const auto target = makeSine(880.0, sampleRate, samples);
    std::vector<float> output(static_cast<std::size_t>(samples), 0.0F);
    run(interpolator, source, target, 0.0F, output);

    const auto latency = interpolator.latencySamples();
    REQUIRE(latency == settings.windowSize);
    auto error = 0.0F;
    auto count = 0;
    for (int i = latency + settings.windowSize; i < samples; ++i) {
        error += std::abs(output[static_cast<std::size_t>(i)] -
                          source[static_cast<std::size_t>(i - latency)]);
        ++count;
    }
    REQUIRE(count > 0);
    REQUIRE(error / static_cast<float>(count) < 1.0e-5F);
}

TEST_CASE("spectral transport mix 0.5 glides sines toward the midpoint") {
    constexpr auto sampleRate = 44100.0;
    SpectralTransportInterpolator interpolator;
    interpolator.prepare(ProcessSpec{sampleRate, 512, 1, 2});
    const auto settings = interpolator.settings();
    const auto samples = settings.windowSize * 8;
    const auto source = makeSine(440.0, sampleRate, samples);
    const auto target = makeSine(880.0, sampleRate, samples);
    std::vector<float> output(static_cast<std::size_t>(samples), 0.0F);
    run(interpolator, source, target, 0.5F, output);

    const auto peak = peakHz(output, sampleRate, interpolator.latencySamples() + settings.windowSize * 2);
    REQUIRE(peak == Catch::Approx(660.0).margin(40.0));
}

TEST_CASE("spectral transport mix 0.5 keeps audible level") {
    constexpr auto sampleRate = 44100.0;
    SpectralTransportInterpolator interpolator;
    interpolator.prepare(ProcessSpec{sampleRate, 512, 1, 2});
    const auto settings = interpolator.settings();
    const auto samples = settings.windowSize * 8;
    const auto source = makeSine(440.0, sampleRate, samples);
    const auto target = makeSine(880.0, sampleRate, samples);
    std::vector<float> output(static_cast<std::size_t>(samples), 0.0F);
    run(interpolator, source, target, 0.5F, output);

    const auto start = interpolator.latencySamples() + settings.windowSize * 2;
    auto sumSq = 0.0;
    auto count = 0;
    for (int i = start; i < samples; ++i) {
        const auto value = static_cast<double>(output[static_cast<std::size_t>(i)]);
        sumSq += value * value;
        ++count;
    }
    REQUIRE(count > 0);
    const auto rms = std::sqrt(sumSq / static_cast<double>(count));
    REQUIRE(rms > 0.2);
    REQUIRE(rms < 1.5);
}

TEST_CASE("spectral transport reads mix from the second weight") {
    constexpr auto sampleRate = 44100.0;
    SpectralTransportInterpolator interpolator;
    interpolator.prepare(ProcessSpec{sampleRate, 512, 1, 2});
    const auto settings = interpolator.settings();
    const auto samples = settings.windowSize * 6;
    const auto source = makeSine(440.0, sampleRate, samples);
    const auto target = makeSine(880.0, sampleRate, samples);
    std::vector<float> output(static_cast<std::size_t>(samples), 0.0F);
    run(interpolator, source, target, 1.0F, output);

    const auto latency = interpolator.latencySamples();
    auto error = 0.0F;
    auto count = 0;
    for (int i = latency + settings.windowSize; i < samples; ++i) {
        error += std::abs(output[static_cast<std::size_t>(i)] -
                          target[static_cast<std::size_t>(i - latency)]);
        ++count;
    }
    REQUIRE(count > 0);
    REQUIRE(error / static_cast<float>(count) < 1.0e-5F);
}
