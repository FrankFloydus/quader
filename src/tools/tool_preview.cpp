/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "tools/tool_preview.hpp"

namespace quader::tools {

bool ToolPreview::empty() const noexcept {
	return !active && status_text.empty() && !view_index.has_value() && points.empty() && segments.empty() && world_points.empty() && world_segments.empty() && boxes.empty();
}

void ToolPreview::clear() {
	active = false;
	status_text.clear();
	view_index = std::nullopt;
	points.clear();
	segments.clear();
	world_points.clear();
	world_segments.clear();
	boxes.clear();
	overlay_only = true;
}

} // namespace quader::tools
