#pragma once

#include <juce_dsp/juce_dsp.h>

#include <complex>
#include <memory>
#include <span>
#include <vector>

namespace interpolation_lab {

class RealFft {
  public:
    RealFft() = default;
    explicit RealFft(int fftSize);

    void setSize(int fftSize);
    [[nodiscard]] auto size() const -> int { return fftSize_; }
    void forward(std::span<const float> time, std::span<std::complex<float>> bins);
    void inverse(std::span<const std::complex<float>> bins, std::span<float> time);

  private:
    int fftSize_{0};
    std::vector<float> work_;
    std::unique_ptr<juce::dsp::FFT> fft_;
};

} // namespace interpolation_lab
