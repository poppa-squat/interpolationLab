#pragma once

#include "dsp/Interpolator.h"

#include <memory>
#include <string_view>

namespace interpolation_lab {

struct InterpolatorInfo {
    std::string_view id;
    std::string_view name;
    std::unique_ptr<Interpolator> (*create)();
};

class InterpolatorCatalog {
  public:
    [[nodiscard]] static auto size() -> int;
    [[nodiscard]] static auto info(int index) -> const InterpolatorInfo&;
    [[nodiscard]] static auto create(int index) -> std::unique_ptr<Interpolator>;
};

} // namespace interpolation_lab
