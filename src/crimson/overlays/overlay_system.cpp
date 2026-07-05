/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/overlays/overlay_system.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace crimson {
namespace {

[[nodiscard]] float finite_or_zero(float value) noexcept {
	return std::isfinite(value) ? value : 0.0F;
}

[[nodiscard]] float clamp01(float value) noexcept {
	return std::clamp(finite_or_zero(value), 0.0F, 1.0F);
}

[[nodiscard]] OverlayDrawBucket &bucket_for_depth_mode(
		OverlayDrawLists &lists,
		OverlayDepthMode depth_mode) noexcept {
	switch (depth_mode) {
		case OverlayDepthMode::DepthTested:
			return lists.depth_tested;
		case OverlayDepthMode::XRay:
			return lists.xray;
		case OverlayDepthMode::AlwaysOnTop:
			return lists.always_on_top;
	}

	return lists.depth_tested;
}

void append_grid_payloads(
		OverlayDrawBucket &bucket,
		const OverlayCommand &command,
		std::span<const GridOverlayCommand> grid_payloads) {
	const std::size_t kBegin = static_cast<std::size_t>(command.payload_offset);
	const std::size_t kCount = static_cast<std::size_t>(command.payload_count);
	if (kBegin >= grid_payloads.size() || kCount == 0 || kBegin + kCount > grid_payloads.size()) {
		return;
	}

	for (std::size_t index = kBegin; index < kBegin + kCount; ++index) {
		const GridOverlayCommand &grid = grid_payloads[index];
		if (grid.view_index != command.view_index) {
			continue;
		}

		bucket.grid_commands.push_back(PreparedGridOverlayCommand{
				.command = command,
				.grid = grid,
				.minor_color_linear_sdr = to_linear_sdr_array(grid.minor_color, command.opacity),
				.major_color_linear_sdr = to_linear_sdr_array(grid.major_color, command.opacity),
				.axis_u_color_linear_sdr = to_linear_sdr_array(grid.axis_u_color, command.opacity),
				.axis_v_color_linear_sdr = to_linear_sdr_array(grid.axis_v_color, command.opacity),
		});
	}
}

void append_line_payloads(
		OverlayDrawBucket &bucket,
		const OverlayCommand &command,
		std::span<const LineOverlaySegment> line_payloads) {
	const std::size_t kBegin = static_cast<std::size_t>(command.payload_offset);
	const std::size_t kCount = static_cast<std::size_t>(command.payload_count);
	if (kBegin >= line_payloads.size() || kCount == 0 || kBegin + kCount > line_payloads.size()) {
		return;
	}

	PreparedLineOverlayCommand prepared;
	prepared.command = command;
	prepared.color_linear_sdr = to_linear_sdr_array(command.color_srgb, command.opacity);
	prepared.segments.reserve(kCount);
	for (std::size_t index = kBegin; index < kBegin + kCount; ++index) {
		prepared.segments.push_back(line_payloads[index]);
	}
	bucket.line_commands.push_back(std::move(prepared));
}

} // namespace

RenderQueue render_queue_for_overlay_depth_mode(OverlayDepthMode depth_mode) noexcept {
	switch (depth_mode) {
		case OverlayDepthMode::DepthTested:
			return RenderQueue::OverlayDepthTested;
		case OverlayDepthMode::XRay:
			return RenderQueue::OverlayXRay;
		case OverlayDepthMode::AlwaysOnTop:
			return RenderQueue::OverlayAlwaysOnTop;
	}

	return RenderQueue::OverlayDepthTested;
}

std::array<float, 4> to_linear_sdr_array(ColorSrgb color, float opacity) noexcept {
	const ColorLinear kLinear = srgb_to_linear(color);
	return {
		kLinear.r,
		kLinear.g,
		kLinear.b,
		clamp01(kLinear.a * opacity),
	};
}

std::size_t OverlayDrawLists::command_count() const noexcept {
	return depth_tested.commands.size() + depth_tested.grid_commands.size() + depth_tested.line_commands.size() + xray.commands.size() + xray.grid_commands.size() + xray.line_commands.size() + always_on_top.commands.size() + always_on_top.grid_commands.size() + always_on_top.line_commands.size();
}

OverlayDrawLists OverlaySystem::prepare(
		std::span<const OverlayCommand> commands,
		std::span<const GridOverlayCommand> grid_payloads,
		std::span<const LineOverlaySegment> line_payloads) const {
	OverlayDrawLists lists;
	for (const OverlayCommand &command : commands) {
		if (command.base_shader != BaseShaderId::OverlayUnlit) {
			continue;
		}

		OverlayDrawBucket &bucket = bucket_for_depth_mode(lists, command.depth_mode);
		const std::array<float, 4> kColor = to_linear_sdr_array(command.color_srgb, command.opacity);
		bucket.commands.push_back(PreparedOverlayCommand{
				.command = command,
				.color_linear_sdr = ColorLinear{ kColor[0], kColor[1], kColor[2], kColor[3] },
		});

		if (command.primitive == OverlayPrimitive::Grid) {
			append_grid_payloads(bucket, command, grid_payloads);
		} else if (command.primitive == OverlayPrimitive::LineList) {
			append_line_payloads(bucket, command, line_payloads);
		}
	}

	return lists;
}

} // namespace crimson
