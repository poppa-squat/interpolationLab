#pragma once

#include <complex>
#include <cstddef>
#include <span>
#include <tuple>
#include <vector>

namespace interpolation_lab {

struct SpectralPoint {
    std::complex<double> value{};
    double freq{0.0};
    double freqReassigned{0.0};
};

struct SpectralMass {
    std::size_t leftBin{0};
    std::size_t rightBin{0};
    std::size_t centerBin{0};
    double mass{0.0};
};

[[nodiscard]] auto groupSpectrum(std::span<const SpectralPoint> spectrum)
    -> std::vector<SpectralMass>;

[[nodiscard]] auto transportMatrix(const std::vector<SpectralMass>& source,
                                   const std::vector<SpectralMass>& target)
    -> std::vector<std::tuple<std::size_t, std::size_t, double>>;

[[nodiscard]] auto interpolateSpectrum(const std::vector<SpectralPoint>& source,
                                       const std::vector<SpectralPoint>& target,
                                       std::vector<double>& phases, double windowSeconds,
                                       double mix) -> std::vector<SpectralPoint>;

} // namespace interpolation_lab
