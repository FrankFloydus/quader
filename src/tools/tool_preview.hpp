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

#include "math/vec2.hpp"
#include "math/vec3.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace quader::tools {

/// Screen-space preview segment emitted by editor tools.
struct ToolPreviewSegment {
	/// Segment start in viewport-local pixels.
	quader::math::Vec2 start;
	/// Segment end in viewport-local pixels.
	quader::math::Vec2 end;
};

/// World-space preview line segment emitted by editor tools.
struct ToolPreviewWorldSegment {
	/// Segment start in world coordinates.
	quader::math::Vec3 start;
	/// Segment end in world coordinates.
	quader::math::Vec3 end;
};

/// World-space box preview volume emitted by editor tools.
struct ToolPreviewBox {
	/// Eight box corners in tool-defined order.
	std::array<quader::math::Vec3, 8> corners;
	/// True when the box should be rendered.
	bool active = false;
};

/// Toolkit-neutral preview payload for active editor tools.
struct ToolPreview {
	/// True when the preview should be rendered.
	bool active = false;
	/// Optional human-readable status text for host UI surfaces.
	std::string status_text;
	/// Optional viewport index that owns this preview.
	std::optional<std::uint32_t> view_index;
	/// Screen-space preview points.
	std::vector<quader::math::Vec2> points;
	/// Screen-space preview segments.
	std::vector<ToolPreviewSegment> segments;
	/// World-space preview points.
	std::vector<quader::math::Vec3> world_points;
	/// World-space preview segments.
	std::vector<ToolPreviewWorldSegment> world_segments;
	/// World-space box preview volumes.
	std::vector<ToolPreviewBox> boxes;
	/// True when preview geometry must stay out of committed/lit scene queues.
	bool overlay_only = true;

	/// Return true when the preview carries no visible or status payload.
	[[nodiscard]] bool empty() const noexcept;
	/// Reset the preview to an empty overlay-only state.
	void clear();
};

} // namespace quader::tools
