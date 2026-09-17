#include "dsp/AudioAutoencoder.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <utility>

#ifdef INTERPOLATION_LAB_USE_TORCH

#include <torch/script.h>
#include <torch/torch.h>

namespace interpolation_lab {
namespace {

auto readNumber(const torch::jit::Module& model, const char* name, double fallback) -> double {
    try {
        const auto value = model.attr(name);
        if (value.isInt()) {
            return static_cast<double>(value.toInt());
        }
        if (value.isDouble()) {
            return value.toDouble();
        }
    } catch (const std::exception&) {
    }
    return fallback;
}

class RaveTorchScriptAutoencoder final : public AudioAutoencoder {
  public:
    RaveTorchScriptAutoencoder(std::string path, torch::jit::Module model, const double sampleRate,
                               const int hopSize, const int latentSize)
        : path_(std::move(path)),
          model_(std::move(model)),
          sampleRate_(sampleRate > 0.0 ? sampleRate : 48000.0),
          hopSize_(std::max(1, hopSize)),
          latentSize_(std::max(1, latentSize)) {
        model_.eval();
    }

    void matchHostSampleRate(double) override {}
    [[nodiscard]] auto sampleRate() const -> double override { return sampleRate_; }
    [[nodiscard]] auto hopSize() const -> int override { return hopSize_; }
    [[nodiscard]] auto latentSize() const -> int override { return latentSize_; }

    void reset() override {
        try {
            model_.get_method("reset")(std::vector<torch::jit::IValue>{});
        } catch (const std::exception&) {
        }
    }

    void encode(const std::span<const float> audio, const std::span<float> latent) override {
        std::fill(latent.begin(), latent.end(), 0.0F);
        if (audio.empty() || latent.empty()) {
            return;
        }
        try {
            const torch::InferenceMode guard;
            auto input = torch::from_blob(const_cast<float*>(audio.data()),
                                          {1, 1, static_cast<long>(audio.size())}, torch::kFloat)
                             .clone();
            auto encoded = model_.get_method("encode")({input}).toTensor().contiguous().to(torch::kFloat);
            copyTensor(encoded, latent);
        } catch (const std::exception&) {
            std::fill(latent.begin(), latent.end(), 0.0F);
        }
    }

    void decode(const std::span<const float> latent, const std::span<float> audio) override {
        std::fill(audio.begin(), audio.end(), 0.0F);
        if (audio.empty() || latent.empty()) {
            return;
        }
        try {
            const torch::InferenceMode guard;
            auto input = torch::from_blob(const_cast<float*>(latent.data()),
                                          {1, static_cast<long>(latent.size()), 1}, torch::kFloat)
                             .clone();
            auto decoded = model_.get_method("decode")({input}).toTensor().contiguous().to(torch::kFloat);
            copyTensor(decoded, audio);
        } catch (const std::exception&) {
            try {
                const torch::InferenceMode guard;
                auto input = torch::from_blob(const_cast<float*>(latent.data()),
                                              {1, 1, static_cast<long>(latent.size())}, torch::kFloat)
                                 .clone();
                auto decoded =
                    model_.get_method("decode")({input}).toTensor().contiguous().to(torch::kFloat);
                copyTensor(decoded, audio);
            } catch (const std::exception&) {
                std::fill(audio.begin(), audio.end(), 0.0F);
            }
        }
    }

    [[nodiscard]] auto clone() const -> std::unique_ptr<AudioAutoencoder> override {
        return loadRaveTorchScript(path_);
    }

  private:
    static void copyTensor(const torch::Tensor& tensor, const std::span<float> dest) {
        const auto flat = tensor.reshape({-1}).contiguous();
        const auto count =
            std::min(dest.size(), static_cast<std::size_t>(std::max<int64_t>(0, flat.numel())));
        if (count == 0 || !flat.data_ptr<float>()) {
            return;
        }
        std::copy_n(flat.data_ptr<float>(), count, dest.begin());
    }

    std::string path_;
    torch::jit::Module model_;
    double sampleRate_{48000.0};
    int hopSize_{2048};
    int latentSize_{16};
};

auto inferHopAndLatent(torch::jit::Module& model, const int fallbackHop)
    -> std::pair<int, int> {
    auto hop = fallbackHop;
    auto latent = 16;
    try {
        const torch::InferenceMode guard;
        auto probe = torch::zeros({1, 1, hop}, torch::kFloat);
        auto encoded = model.get_method("encode")({probe}).toTensor().contiguous();
        if (encoded.dim() >= 2) {
            latent = static_cast<int>(encoded.size(1));
        } else {
            latent = static_cast<int>(encoded.numel());
        }
        if (encoded.dim() >= 3 && encoded.size(2) > 1 && encoded.size(2) < hop) {
            hop = hop / static_cast<int>(encoded.size(2));
            hop = std::max(1, hop);
        }
    } catch (const std::exception&) {
    }
    return {std::max(1, hop), std::max(1, latent)};
}

} // namespace

auto loadRaveTorchScript(const std::string& path) -> std::unique_ptr<AudioAutoencoder> {
    if (path.empty()) {
        return nullptr;
    }
    try {
        torch::set_num_threads(1);
        auto model = torch::jit::load(path);
        model.eval();
        const auto sampleRate = readNumber(model, "sampling_rate", readNumber(model, "sr", 48000.0));
        auto hop = static_cast<int>(
            std::lround(readNumber(model, "hop_length", readNumber(model, "ratio", 2048.0))));
        hop = std::max(1, hop);
        const auto inferred = inferHopAndLatent(model, hop);
        return std::make_unique<RaveTorchScriptAutoencoder>(path, std::move(model), sampleRate,
                                                            inferred.first, inferred.second);
    } catch (const std::exception& error) {
        std::cerr << "RAVE model load failed: " << error.what() << '\n';
        return nullptr;
    }
}

} // namespace interpolation_lab

#endif
