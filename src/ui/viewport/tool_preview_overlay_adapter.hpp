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
#include "document/object_id.hpp"
#include "tools/editor_input_event.hpp"
#include "tools/tool_preview.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace quader::document {
class Document;
} // namespace quader::document

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
 * Return the Crimson render-object id used for a document mesh object.
 *
 * @param id Document object id to encode for renderer-facing references.
 * @return Stable per-frame render object id matching the mesh render upload path.
 */
[[nodiscard]] crimson::RenderObjectId render_object_id_for_document_object(
		quader::document::ObjectId id) noexcept;

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

/**
 * Convert a tool preview's styled world lines and triangles into Crimson overlay commands.
 *
 * @param preview Tool preview to convert.
 * @param view_count Number of active render views.
 * @param overlays Destination overlay command list.
 * @param line_payloads Destination line segment payload list.
 * @param triangle_payloads Destination solid triangle payload list.
 */
void append_tool_preview_overlays(
		const quader::tools::ToolPreview &preview,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads);

/**
 * Convert the document selection into semantic Crimson overlay commands.
 *
 * Object mode emits selected topology wires. Component modes emit source-wire
 * topology/depth metadata for active edit targets plus selected and hovered
 * face, edge, or vertex component highlights.
 *
 * @param document Document whose selection and mesh topology are converted.
 * @param view_count Number of active render views.
 * @param overlays Destination overlay command list.
 * @param line_payloads Destination line segment payload list.
 * @param triangle_payloads Destination face-fill/source-depth payload list.
 * @param point_payloads Destination vertex-handle payload list.
 * @param selection_hover Optional transient hit used for component hover overlays.
 * @param selection_hover_suppresses_selected True only for selected-hover
 * removal preview, where the matching selected base overlay is hidden.
 */
void append_document_selection_overlays(
		const quader::document::Document &document,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads,
		std::vector<crimson::PointOverlayPrimitive> &point_payloads,
		const std::optional<quader::tools::SurfaceHit> &selection_hover = std::nullopt,
		bool selection_hover_suppresses_selected = false);

} // namespace quader::ui
