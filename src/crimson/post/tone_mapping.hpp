#pragma once

#include "crimson/color/color_space.hpp"
#include "crimson/renderer_config.hpp"

#include <optional>

namespace crimson {

enum class DisplayConversionPath {
    SrgbBackbuffer,
    ManualFinalShader,
};

[[nodiscard]] ColorLinear apply_tone_mapper(ToneMapper mapper, ColorLinear hdr) noexcept;
[[nodiscard]] std::optional<DisplayConversionPath> choose_display_conversion_path(
    bool srgb_backbuffer_supported,
    DisplayConversionSettings settings) noexcept;

} // namespace crimson
