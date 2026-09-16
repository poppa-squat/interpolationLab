#include "dsp/OptimalTransport.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>

namespace interpolation_lab {
namespace {

void placeMass(const SpectralMass& mass, const int centerBin, const double scale,
               const double interpolatedFreq, const double centerPhase,
               const std::vector<SpectralPoint>& input, std::vector<SpectralPoint>& output,
               const double nextPhase, std::vector<double>& phases,
               std::vector<double>& amplitudes) {
    const auto phaseShift = centerPhase - std::arg(input[mass.centerBin].value);

    for (auto i = mass.leftBin; i < mass.rightBin; ++i) {
        const auto newIndex = static_cast<int>(i) + centerBin - static_cast<int>(mass.centerBin);
        if (newIndex < 0 || newIndex >= static_cast<int>(output.size())) {
            continue;
        }

        const auto phase = phaseShift + std::arg(input[i].value);
        const auto mag = scale * std::abs(input[i].value);
        output[static_cast<std::size_t>(newIndex)].value += std::polar(mag, phase);

        auto& amp = amplitudes[static_cast<std::size_t>(newIndex)];
        if (mag > amp) {
            amp = mag;
            phases[static_cast<std::size_t>(newIndex)] = nextPhase;
            output[static_cast<std::size_t>(newIndex)].freqReassigned = interpolatedFreq;
        }
    }
}

} // namespace

auto groupSpectrum(const std::span<const SpectralPoint> spectrum) -> std::vector<SpectralMass> {
    if (spectrum.empty()) {
        return {};
    }

    auto massSum = 0.0;
    for (const auto& point : spectrum) {
        massSum += std::abs(point.value);
    }

    std::vector<SpectralMass> masses;
    masses.push_back(SpectralMass{0, 0, 0, 0.0});

    auto first = true;
    auto sign = false;
    for (std::size_t i = 0; i < spectrum.size(); ++i) {
        const auto currentSign = spectrum[i].freqReassigned > spectrum[i].freq;
        if (first) {
            first = false;
            sign = currentSign;
            continue;
        }
        if (currentSign == sign) {
            continue;
        }

        if (sign) {
            const auto leftDist = spectrum[i - 1].freqReassigned - spectrum[i - 1].freq;
            const auto rightDist = spectrum[i].freq - spectrum[i].freqReassigned;
            masses.back().centerBin = leftDist < rightDist ? i - 1 : i;
        } else {
            masses.back().mass = 0.0;
            for (auto j = masses.back().leftBin; j < i; ++j) {
                masses.back().mass += std::abs(spectrum[j].value);
            }
            if (masses.back().mass > 0.0) {
                if (massSum > 0.0) {
                    masses.back().mass /= massSum;
                }
                masses.back().rightBin = i;
                masses.push_back(SpectralMass{i, 0, i, 0.0});
            }
        }
        sign = currentSign;
    }

    masses.back().rightBin = spectrum.size();
    masses.back().mass = 0.0;
    for (auto j = masses.back().leftBin; j < spectrum.size(); ++j) {
        masses.back().mass += std::abs(spectrum[j].value);
    }
    if (massSum > 0.0) {
        masses.back().mass /= massSum;
    }

    masses.erase(std::remove_if(masses.begin(), masses.end(),
                                [](const SpectralMass& mass) { return mass.mass <= 0.0; }),
                 masses.end());
    if (masses.empty()) {
        masses.push_back(SpectralMass{0, spectrum.size(), 0, 0.0});
    }
    return masses;
}

auto transportMatrix(const std::vector<SpectralMass>& source,
                     const std::vector<SpectralMass>& target)
    -> std::vector<std::tuple<std::size_t, std::size_t, double>> {
    std::vector<std::tuple<std::size_t, std::size_t, double>> assignments;
    if (source.empty() || target.empty()) {
        return assignments;
    }

    auto sourceIndex = std::size_t{0};
    auto targetIndex = std::size_t{0};
    auto sourceMass = source[0].mass;
    auto targetMass = target[0].mass;

    while (true) {
        if (sourceMass < targetMass) {
            assignments.emplace_back(sourceIndex, targetIndex, sourceMass);
            targetMass -= sourceMass;
            ++sourceIndex;
            if (sourceIndex >= source.size()) {
                break;
            }
            sourceMass = source[sourceIndex].mass;
        } else {
            assignments.emplace_back(sourceIndex, targetIndex, targetMass);
            sourceMass -= targetMass;
            ++targetIndex;
            if (targetIndex >= target.size()) {
                break;
            }
            targetMass = target[targetIndex].mass;
        }
    }
    return assignments;
}

auto interpolateSpectrum(const std::vector<SpectralPoint>& source,
                         const std::vector<SpectralPoint>& target, std::vector<double>& phases,
                         const double windowSeconds, const double mix)
    -> std::vector<SpectralPoint> {
    std::vector<SpectralPoint> interpolated(source.size());
    if (source.empty() || target.empty()) {
        return interpolated;
    }

    const auto sourceMasses = groupSpectrum(source);
    const auto targetMasses = groupSpectrum(target);
    const auto assignments = transportMatrix(sourceMasses, targetMasses);

    for (std::size_t i = 0; i < source.size(); ++i) {
        interpolated[i].freq = source[i].freq;
    }

    if (phases.size() != source.size()) {
        phases.assign(source.size(), 0.0);
    }

    std::vector<double> newAmplitudes(phases.size(), 0.0);
    std::vector<double> newPhases(phases.size(), 0.0);
    const auto clampedMix = std::clamp(mix, 0.0, 1.0);

    for (const auto& assignment : assignments) {
        const auto& sourceMass = sourceMasses[std::get<0>(assignment)];
        const auto& targetMass = targetMasses[std::get<1>(assignment)];
        const auto transported = std::get<2>(assignment);

        auto interpolatedBin = static_cast<int>(std::llround(
            (1.0 - clampedMix) * static_cast<double>(sourceMass.centerBin) +
            clampedMix * static_cast<double>(targetMass.centerBin)));
        interpolatedBin = std::clamp(interpolatedBin, 0, static_cast<int>(source.size()) - 1);

        auto roundedMix = clampedMix;
        if (sourceMass.centerBin != targetMass.centerBin) {
            roundedMix = (static_cast<double>(interpolatedBin) -
                          static_cast<double>(sourceMass.centerBin)) /
                         (static_cast<double>(targetMass.centerBin) -
                          static_cast<double>(sourceMass.centerBin));
        }

        const auto interpolatedFreq =
            (1.0 - roundedMix) * source[sourceMass.centerBin].freqReassigned +
            roundedMix * target[targetMass.centerBin].freqReassigned;

        const auto binPhase = std::numbers::pi * static_cast<double>(interpolatedBin);
        const auto halfAdvance = (interpolatedFreq * windowSeconds / 2.0) / 2.0;
        const auto centerPhase = phases[static_cast<std::size_t>(interpolatedBin)] + halfAdvance - binPhase;
        const auto nextPhase = centerPhase + halfAdvance + binPhase;

        const auto sourceScale =
            sourceMass.mass > 0.0 ? (1.0 - clampedMix) * transported / sourceMass.mass : 0.0;
        const auto targetScale =
            targetMass.mass > 0.0 ? clampedMix * transported / targetMass.mass : 0.0;

        placeMass(sourceMass, interpolatedBin, sourceScale, interpolatedFreq, centerPhase, source,
                  interpolated, nextPhase, newPhases, newAmplitudes);
        placeMass(targetMass, interpolatedBin, targetScale, interpolatedFreq, centerPhase, target,
                  interpolated, nextPhase, newPhases, newAmplitudes);
    }

    phases = std::move(newPhases);
    return interpolated;
}

} // namespace interpolation_lab
