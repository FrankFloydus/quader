#pragma once

#include "crimson/overlays/overlay_command.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace crimson {

struct PreparedOverlayCommand {
	OverlayCommand command;
	ColorLinear color_linear_sdr;
};

struct PreparedGridOverlayCommand {
	OverlayCommand command;
	GridOverlayCommand grid;
	std::array<float, 4> minor_color_linear_sdr{};
	std::array<float, 4> major_color_linear_sdr{};
	std::array<float, 4> axis_u_color_linear_sdr{};
	std::array<float, 4> axis_v_color_linear_sdr{};
};

struct OverlayDrawBucket {
	std::vector<PreparedOverlayCommand> commands;
	std::vector<PreparedGridOverlayCommand> grid_commands;
};

struct OverlayDrawLists {
	OverlayDrawBucket depth_tested;
	OverlayDrawBucket xray;
	OverlayDrawBucket always_on_top;

	[[nodiscard]] std::size_t command_count() const noexcept;
};

class OverlaySystem final {
public:
	[[nodiscard]] OverlayDrawLists prepare(
			std::span<const OverlayCommand> commands,
			std::span<const GridOverlayCommand> grid_payloads) const;
};

[[nodiscard]] std::array<float, 4> to_linear_sdr_array(ColorSrgb color, float opacity = 1.0F) noexcept;

} // namespace crimson
