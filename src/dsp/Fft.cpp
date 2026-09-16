#include "dsp/Fft.h"

#include <algorithm>
#include <cmath>

namespace interpolation_lab {

namespace {

auto fftOrderFor(const int fftSize) -> int {
    if (fftSize < 2) {
        return 0;
    }
    return juce::roundToInt(std::log2(static_cast<double>(fftSize)));
}

} // namespace

RealFft::RealFft(const int fftSize) { setSize(fftSize); }

void RealFft::setSize(const int fftSize) {
    fftSize_ = fftSize;
    if (fftSize_ < 2) {
        fft_.reset();
        work_.clear();
        return;
    }
    fft_ = std::make_unique<juce::dsp::FFT>(fftOrderFor(fftSize_));
    work_.assign(static_cast<std::size_t>(fftSize_) * 2U, 0.0F);
}

void RealFft::forward(const std::span<const float> time, const std::span<std::complex<float>> bins) {
    if (fft_ == nullptr || fftSize_ < 2) {
        return;
    }
    std::fill(work_.begin(), work_.end(), 0.0F);
    const auto copyCount = std::min(time.size(), static_cast<std::size_t>(fftSize_));
    if (copyCount > 0) {
        std::copy_n(time.begin(), copyCount, work_.begin());
    }
    fft_->performRealOnlyForwardTransform(work_.data(), true);

    const auto binCount = static_cast<std::size_t>(fftSize_ / 2 + 1);
    const auto outCount = std::min(bins.size(), binCount);
    for (std::size_t bin = 0; bin < outCount; ++bin) {
        bins[bin] = {work_[2 * bin], work_[2 * bin + 1]};
    }
}

void RealFft::inverse(const std::span<const std::complex<float>> bins, const std::span<float> time) {
    if (fft_ == nullptr || fftSize_ < 2) {
        return;
    }
    std::fill(work_.begin(), work_.end(), 0.0F);
    const auto binCount = static_cast<std::size_t>(fftSize_ / 2 + 1);
    const auto inCount = std::min(bins.size(), binCount);
    for (std::size_t bin = 0; bin < inCount; ++bin) {
        work_[2 * bin] = bins[bin].real();
        work_[2 * bin + 1] = bins[bin].imag();
    }
    fft_->performRealOnlyInverseTransform(work_.data());

    const auto copyCount = std::min(time.size(), static_cast<std::size_t>(fftSize_));
    if (copyCount > 0) {
        std::copy_n(work_.begin(), copyCount, time.begin());
    }
}

} // namespace interpolation_lab
