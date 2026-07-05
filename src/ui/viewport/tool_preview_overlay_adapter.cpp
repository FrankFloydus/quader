/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/viewport/tool_preview_overlay_adapter.hpp"

#include <array>
#include <utility>

namespace quader::ui {
namespace {

constexpr std::array<std::pair<std::size_t, std::size_t>, 12> kBoxEdges{
	std::pair<std::size_t, std::size_t>{ 0U, 1U },
	std::pair<std::size_t, std::size_t>{ 1U, 2U },
	std::pair<std::size_t, std::size_t>{ 2U, 3U },
	std::pair<std::size_t, std::size_t>{ 3U, 0U },
	std::pair<std::size_t, std::size_t>{ 4U, 5U },
	std::pair<std::size_t, std::size_t>{ 5U, 6U },
	std::pair<std::size_t, std::size_t>{ 6U, 7U },
	std::pair<std::size_t, std::size_t>{ 7U, 4U },
	std::pair<std::size_t, std::size_t>{ 0U, 4U },
	std::pair<std::size_t, std::size_t>{ 1U, 5U },
	std::pair<std::size_t, std::size_t>{ 2U, 6U },
	std::pair<std::size_t, std::size_t>{ 3U, 7U },
};

} // namespace

void append_tool_preview_line_overlays(
		const quader::tools::ToolPreview &preview,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads) {
	if (!preview.active || (preview.world_segments.empty() && preview.boxes.empty())) {
		return;
	}

	const std::uint32_t kPayloadOffset = static_cast<std::uint32_t>(line_payloads.size());
	for (const quader::tools::ToolPreviewWorldSegment &segment : preview.world_segments) {
		line_payloads.push_back(crimson::LineOverlaySegment{
				.start = segment.start,
				.end = segment.end,
		});
	}
	for (const quader::tools::ToolPreviewBox &box : preview.boxes) {
		if (!box.active) {
			continue;
		}
		const std::size_t kFirstBoxEdge = preview.world_segments.empty() ? 0U : 4U;
		for (std::size_t edge_index = kFirstBoxEdge; edge_index < kBoxEdges.size(); ++edge_index) {
			const auto &[start_index, end_index] = kBoxEdges[edge_index];
			line_payloads.push_back(crimson::LineOverlaySegment{
					.start = box.corners[start_index],
					.end = box.corners[end_index],
			});
		}
	}

	const std::uint32_t kPayloadCount = static_cast<std::uint32_t>(line_payloads.size()) - kPayloadOffset;
	if (kPayloadCount == 0U) {
		return;
	}

	auto append_overlay = [&](std::uint32_t view_index) {
		overlays.push_back(crimson::OverlayCommand{
				.view_index = view_index,
				.primitive = crimson::OverlayPrimitive::LineList,
				.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
				.base_shader = crimson::BaseShaderId::OverlayUnlit,
				.color_srgb = kBoxToolPreviewLineColorSrgb,
				.opacity = kBoxToolPreviewLineOpacity,
				.thickness_px = kBoxToolPreviewLineThicknessPx,
				.payload_offset = kPayloadOffset,
				.payload_count = kPayloadCount,
		});
	};

	if (preview.view_index.has_value()) {
		if (*preview.view_index < view_count) {
			append_overlay(*preview.view_index);
		}
		return;
	}

	overlays.reserve(overlays.size() + view_count);
	for (std::size_t index = 0; index < view_count; ++index) {
		append_overlay(static_cast<std::uint32_t>(index));
	}
}

} // namespace quader::ui
