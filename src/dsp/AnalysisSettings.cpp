#include "dsp/AnalysisSettings.h"

#include <algorithm>
#include <cmath>

namespace interpolation_lab {

auto AnalysisSettings::fromSampleRate(const double sampleRate) -> AnalysisSettings {
    AnalysisSettings settings;
    settings.sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    auto window = static_cast<int>(std::llround(kWindowSeconds * settings.sampleRate));
    if (window < 2) {
        window = 2;
    }
    if ((window % 2) != 0) {
        ++window;
    }
    settings.windowSize = window;
    settings.hopSize = window / 2;

    const auto minFft = std::max(
        window, static_cast<int>(std::ceil(settings.sampleRate / kTargetBinHz)));
    auto fft = 1;
    while (fft < minFft) {
        fft <<= 1;
    }
    settings.fftSize = fft;
    return settings;
}

} // namespace interpolation_lab
