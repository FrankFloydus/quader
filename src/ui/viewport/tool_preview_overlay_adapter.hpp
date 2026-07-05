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
#include "tools/tool_preview.hpp"

#include <cstddef>
#include <vector>

namespace quader::ui {

/// Reference Box tool preview line color in sRGB.
inline constexpr crimson::ColorSrgb kBoxToolPreviewLineColorSrgb{
	1.0F,
	235.0F / 255.0F,
	41.0F / 255.0F,
	209.0F / 255.0F,
};
/// Reference Box tool preview opacity. Alpha is carried in the color value.
inline constexpr float kBoxToolPreviewLineOpacity = 1.0F;
/// Reference Box tool preview line thickness in physical pixels.
inline constexpr float kBoxToolPreviewLineThicknessPx = 2.0F;

/**
 * Convert a tool preview's world line segments into Crimson overlay commands.
 *
 * @param preview Tool preview to convert.
 * @param view_count Number of active render views.
 * @param overlays Destination overlay command list.
 * @param line_payloads Destination line segment payload list.
 */
void append_tool_preview_line_overlays(
		const quader::tools::ToolPreview &preview,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads);

} // namespace quader::ui
