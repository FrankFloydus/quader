/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include "crimson/overlays/overlay_command.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace crimson {

/// Overlay command after color conversion for renderer submission.
struct PreparedOverlayCommand {
	/// Original overlay command.
	OverlayCommand command;
	/// Linear SDR color after opacity multiplication.
	ColorLinear color_linear_sdr;
};

/// Prepared grid overlay command and shader-facing colors.
struct PreparedGridOverlayCommand {
	/// Original overlay command.
	OverlayCommand command;
	/// Grid payload referenced by the command.
	GridOverlayCommand grid;
	/// Minor grid color in linear SDR.
	std::array<float, 4> minor_color_linear_sdr{};
	/// Major grid color in linear SDR.
	std::array<float, 4> major_color_linear_sdr{};
	/// U-axis color in linear SDR.
	std::array<float, 4> axis_u_color_linear_sdr{};
	/// V-axis color in linear SDR.
	std::array<float, 4> axis_v_color_linear_sdr{};
};

/// Prepared line overlay command and copied line payloads.
struct PreparedLineOverlayCommand {
	/// Original overlay command.
	OverlayCommand command;
	/// Line segments referenced by the command.
	std::vector<LineOverlaySegment> segments;
	/// Command color in linear SDR.
	std::array<float, 4> color_linear_sdr{};
};

/// Prepared overlay commands for one depth bucket.
struct OverlayDrawBucket {
	/// Generic prepared commands.
	std::vector<PreparedOverlayCommand> commands;
	/// Prepared grid commands.
	std::vector<PreparedGridOverlayCommand> grid_commands;
	/// Prepared line commands.
	std::vector<PreparedLineOverlayCommand> line_commands;
};

/// Prepared overlay commands split by depth policy.
struct OverlayDrawLists {
	/// Overlays hidden by scene depth.
	OverlayDrawBucket depth_tested;
	/// Overlays blended through scene depth.
	OverlayDrawBucket xray;
	/// Overlays drawn on top.
	OverlayDrawBucket always_on_top;

	/// Return the total number of prepared command records across all buckets.
	[[nodiscard]] std::size_t command_count() const noexcept;
};

/// Converts overlay snapshot commands into depth buckets and linear colors.
class OverlaySystem final {
public:
	/**
	 * Prepare overlay commands for submission.
	 *
	 * @param commands Overlay commands to bucket.
	 * @param grid_payloads Grid payload storage referenced by commands.
	 * @param line_payloads Line payload storage referenced by commands.
	 * @return Prepared draw lists with copied payload ranges.
	 */
	[[nodiscard]] OverlayDrawLists prepare(
			std::span<const OverlayCommand> commands,
			std::span<const GridOverlayCommand> grid_payloads,
			std::span<const LineOverlaySegment> line_payloads = {}) const;
};

/// Convert an sRGB overlay color to a linear SDR RGBA array.
[[nodiscard]] std::array<float, 4> to_linear_sdr_array(ColorSrgb color, float opacity = 1.0F) noexcept;

} // namespace crimson
