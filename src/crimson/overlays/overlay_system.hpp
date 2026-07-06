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
#include <cstdint>
#include <span>
#include <vector>

namespace crimson {

/// Overlay command after color conversion for renderer submission.
struct PreparedOverlayCommand {
	/// Overlay command after semantic depth policy resolution.
	OverlayCommand command;
	/// Linear SDR color after opacity multiplication.
	ColorLinear color_linear_sdr;
};

/// Render pass behavior for a prepared overlay batch.
enum class PreparedOverlayPassKind : std::uint8_t {
	Color,          ///< Normal color overlay pass.
	DepthStamp,     ///< Depth-only metadata/stamp pass.
	EqualDepthColor,///< Color pass that requires equality with the stamped depth.
};

/// Render-state contract resolved from semantic overlay policy.
struct PreparedOverlayRenderState {
	/// Effective depth bucket for this prepared batch.
	OverlayDepthMode depth_mode = OverlayDepthMode::DepthTested;
	/// Prepared pass kind.
	PreparedOverlayPassKind pass_kind = PreparedOverlayPassKind::Color;
	/// Whether color channels are written.
	bool color_write_enabled = true;
	/// Whether depth writes are enabled.
	bool depth_write_enabled = false;
	/// Whether scene depth testing is enabled.
	bool depth_test_enabled = true;
	/// Whether the pass requires equal-depth testing against a previous stamp.
	bool equal_depth_test_enabled = false;
	/// Whether back-face culling must be disabled for two-sided overlays.
	bool two_sided = false;
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
	/// Overlay command after semantic depth policy resolution.
	OverlayCommand command;
	/// Resolved semantic role for this line batch.
	OverlaySemanticRole semantic_role = OverlaySemanticRole::Generic;
	/// Resolved source category for this line batch.
	OverlaySourceKind source_kind = OverlaySourceKind::Unknown;
	/// Render state resolved for this line batch.
	PreparedOverlayRenderState render_state;
	/// Line segments referenced by the command.
	std::vector<LineOverlaySegment> segments;
	/// Command color in linear SDR.
	std::array<float, 4> color_linear_sdr{};
};

/// Prepared triangle overlay command and copied triangle payloads.
struct PreparedTriangleOverlayCommand {
	/// Overlay command after semantic depth policy resolution.
	OverlayCommand command;
	/// Resolved semantic role for this triangle batch.
	OverlaySemanticRole semantic_role = OverlaySemanticRole::Generic;
	/// Resolved source category for this triangle batch.
	OverlaySourceKind source_kind = OverlaySourceKind::Unknown;
	/// Render state resolved for this triangle batch.
	PreparedOverlayRenderState render_state;
	/// Triangle payloads referenced by the command.
	std::vector<TriangleOverlayPrimitive> triangles;
	/// Command color in linear SDR.
	std::array<float, 4> color_linear_sdr{};
};

/// Prepared point overlay command and copied point payloads.
struct PreparedPointOverlayCommand {
	/// Overlay command after semantic depth policy resolution.
	OverlayCommand command;
	/// Resolved semantic role for this point batch.
	OverlaySemanticRole semantic_role = OverlaySemanticRole::Generic;
	/// Resolved source category for this point batch.
	OverlaySourceKind source_kind = OverlaySourceKind::Unknown;
	/// Render state resolved for this point batch.
	PreparedOverlayRenderState render_state;
	/// Point payloads referenced by the command.
	std::vector<PointOverlayPrimitive> points;
	/// Command color in linear SDR.
	std::array<float, 4> color_linear_sdr{};
	/// Point size in physical pixels.
	float size_px = 1.0F;
};

/// Source-wire depth-stamp payload retained as non-rendering metadata.
struct SourceWireDepthStampMetadata {
	/// View index that owns the metadata.
	std::uint32_t view_index = 0;
	/// Source category that produced the stamp.
	OverlaySourceKind source_kind = OverlaySourceKind::Unknown;
	/// Triangle metadata retained for source-wire ownership/depth semantics.
	TriangleOverlayPrimitive triangle;
	/// First referenced triangle payload.
	std::uint32_t payload_offset = 0;
	/// Number of triangle payload records covered by this metadata.
	std::uint32_t payload_count = 0;
	/// Representative topology element for the metadata range.
	OverlayElementRef element;
};

/// Prepared overlay commands for one depth bucket.
struct OverlayDrawBucket {
	/// Generic prepared commands.
	std::vector<PreparedOverlayCommand> commands;
	/// Prepared grid commands.
	std::vector<PreparedGridOverlayCommand> grid_commands;
	/// Prepared line commands.
	std::vector<PreparedLineOverlayCommand> line_commands;
	/// Prepared triangle commands.
	std::vector<PreparedTriangleOverlayCommand> triangle_commands;
	/// Prepared point commands.
	std::vector<PreparedPointOverlayCommand> point_commands;
};

/// Prepared overlay commands split by depth policy.
struct OverlayDrawLists {
	/// Overlays hidden by scene depth.
	OverlayDrawBucket depth_tested;
	/// Overlays blended through scene depth.
	OverlayDrawBucket xray;
	/// Overlays drawn on top.
	OverlayDrawBucket always_on_top;
	/// Source-wire depth metadata, intentionally not present in render buckets.
	std::vector<SourceWireDepthStampMetadata> source_wire_depth_stamps;

	/// Return the total number of prepared command records across all buckets.
	[[nodiscard]] std::size_t command_count() const noexcept;
	/// Return the number of non-rendering source-wire depth stamp metadata records.
	[[nodiscard]] std::size_t source_wire_depth_stamp_count() const noexcept;
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
	 * @param triangle_payloads Triangle payload storage referenced by commands.
	 * @param point_payloads Point payload storage referenced by commands.
	 * @return Prepared draw lists with copied payload ranges.
	 */
	[[nodiscard]] OverlayDrawLists prepare(
			std::span<const OverlayCommand> commands,
			std::span<const GridOverlayCommand> grid_payloads,
			std::span<const LineOverlaySegment> line_payloads = {},
			std::span<const TriangleOverlayPrimitive> triangle_payloads = {},
			std::span<const PointOverlayPrimitive> point_payloads = {}) const;
};

/// Convert an sRGB overlay color to a linear SDR RGBA array.
[[nodiscard]] std::array<float, 4> to_linear_sdr_array(ColorSrgb color, float opacity = 1.0F) noexcept;

} // namespace crimson
