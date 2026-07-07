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
#include <cstdint>
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

[[nodiscard]] bool payload_range_valid(
		std::uint32_t payload_offset,
		std::uint32_t payload_count,
		std::size_t payload_size) noexcept {
	const std::size_t kBegin = static_cast<std::size_t>(payload_offset);
	const std::size_t kCount = static_cast<std::size_t>(payload_count);
	return kCount > 0 && kBegin < payload_size && kBegin + kCount <= payload_size;
}

[[nodiscard]] bool same_color(ColorSrgb left, ColorSrgb right) noexcept {
	return left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a;
}

[[nodiscard]] bool same_command_batch_key(const OverlayCommand &left, const OverlayCommand &right) noexcept {
	return left.view_index == right.view_index && left.base_shader == right.base_shader && same_color(left.color_srgb, right.color_srgb) && left.opacity == right.opacity && left.thickness_px == right.thickness_px && left.payload_offset == right.payload_offset;
}

[[nodiscard]] PreparedOverlayRenderState make_render_state(
		OverlayDepthMode depth_mode,
		OverlaySemanticRole role,
		OverlaySourceKind source_kind,
		PreparedOverlayPassKind pass_kind) noexcept {
	PreparedOverlayRenderState state;
	state.depth_mode = depth_mode;
	state.pass_kind = pass_kind;
	state.two_sided = overlay_role_renders_two_sided(role);

	if (pass_kind == PreparedOverlayPassKind::DepthStamp) {
		state.color_write_enabled = false;
		state.depth_write_enabled = true;
		state.depth_test_enabled = true;
		state.equal_depth_test_enabled = false;
		return state;
	}

	if (pass_kind == PreparedOverlayPassKind::EqualDepthColor) {
		state.color_write_enabled = true;
		state.depth_write_enabled = false;
		state.depth_test_enabled = true;
		state.equal_depth_test_enabled = true;
		return state;
	}

	state.color_write_enabled = true;
	state.depth_write_enabled = false;
	state.depth_test_enabled = depth_mode == OverlayDepthMode::DepthTested;
	state.equal_depth_test_enabled = false;
	return state;
}

[[nodiscard]] OverlayCommand command_for_prepared_batch(
		const OverlayCommand &command,
		OverlayDepthMode depth_mode,
		OverlaySemanticRole role,
		OverlaySourceKind source_kind,
		std::uint32_t payload_count) noexcept {
	OverlayCommand prepared = command;
	prepared.depth_mode = depth_mode;
	prepared.semantic_role = role;
	prepared.source_kind = source_kind;
	prepared.payload_count = payload_count;
	return prepared;
}

void append_grid_payloads(
		OverlayDrawBucket &bucket,
		const OverlayCommand &command,
		std::span<const GridOverlayCommand> grid_payloads) {
	if (!payload_range_valid(command.payload_offset, command.payload_count, grid_payloads.size())) {
		return;
	}

	const std::size_t kBegin = static_cast<std::size_t>(command.payload_offset);
	const std::size_t kCount = static_cast<std::size_t>(command.payload_count);
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

[[nodiscard]] PreparedLineOverlayCommand &line_batch_for(
		OverlayDrawBucket &bucket,
		const OverlayCommand &command,
		OverlayDepthMode depth_mode,
		OverlaySemanticRole role,
		OverlaySourceKind source_kind) {
	for (PreparedLineOverlayCommand &batch : bucket.line_commands) {
		if (same_command_batch_key(batch.command, command) && batch.semantic_role == role && batch.source_kind == source_kind && batch.command.depth_mode == depth_mode) {
			return batch;
		}
	}

	PreparedLineOverlayCommand batch;
	batch.command = command_for_prepared_batch(command, depth_mode, role, source_kind, 0);
	batch.semantic_role = role;
	batch.source_kind = source_kind;
	batch.render_state = make_render_state(depth_mode, role, source_kind, PreparedOverlayPassKind::Color);
	batch.color_linear_sdr = to_linear_sdr_array(command.color_srgb, command.opacity);
	bucket.line_commands.push_back(std::move(batch));
	return bucket.line_commands.back();
}

void append_line_payloads(
		OverlayDrawLists &lists,
		const OverlayCommand &command,
		std::span<const LineOverlaySegment> line_payloads) {
	if (!payload_range_valid(command.payload_offset, command.payload_count, line_payloads.size())) {
		return;
	}

	const std::size_t kBegin = static_cast<std::size_t>(command.payload_offset);
	const std::size_t kCount = static_cast<std::size_t>(command.payload_count);
	for (std::size_t index = kBegin; index < kBegin + kCount; ++index) {
		const LineOverlaySegment &segment = line_payloads[index];
		const OverlaySemanticRole kRole = effective_overlay_role(command, segment.semantic_role);
		const OverlaySourceKind kSource = effective_overlay_source_kind(command, segment.source_kind);
		const OverlayDepthMode kDepthMode = effective_overlay_depth_mode(command, kRole, kSource);
		OverlayDrawBucket &bucket = bucket_for_depth_mode(lists, kDepthMode);
		PreparedLineOverlayCommand &batch = line_batch_for(bucket, command, kDepthMode, kRole, kSource);
		batch.segments.push_back(segment);
		batch.command.payload_count = static_cast<std::uint32_t>(batch.segments.size());
	}
}

[[nodiscard]] PreparedTriangleOverlayCommand &triangle_batch_for(
		OverlayDrawBucket &bucket,
		const OverlayCommand &command,
		OverlayDepthMode depth_mode,
		OverlaySemanticRole role,
		OverlaySourceKind source_kind,
		PreparedOverlayPassKind pass_kind) {
	for (PreparedTriangleOverlayCommand &batch : bucket.triangle_commands) {
		if (same_command_batch_key(batch.command, command) && batch.semantic_role == role && batch.source_kind == source_kind && batch.command.depth_mode == depth_mode && batch.render_state.pass_kind == pass_kind) {
			return batch;
		}
	}

	PreparedTriangleOverlayCommand batch;
	batch.command = command_for_prepared_batch(command, depth_mode, role, source_kind, 0);
	batch.semantic_role = role;
	batch.source_kind = source_kind;
	batch.render_state = make_render_state(depth_mode, role, source_kind, pass_kind);
	batch.color_linear_sdr = to_linear_sdr_array(command.color_srgb, command.opacity);
	bucket.triangle_commands.push_back(std::move(batch));
	return bucket.triangle_commands.back();
}

void append_source_wire_depth_stamp_metadata(
		OverlayDrawLists &lists,
		const OverlayCommand &command,
		const TriangleOverlayPrimitive &triangle,
		std::size_t payload_index,
		OverlaySourceKind source_kind) {
	lists.source_wire_depth_stamps.push_back(SourceWireDepthStampMetadata{
			.view_index = command.view_index,
			.source_kind = source_kind,
			.triangle = triangle,
			.payload_offset = static_cast<std::uint32_t>(payload_index),
			.payload_count = 1,
			.element = triangle.element,
	});
}

void append_triangle_payloads(
		OverlayDrawLists &lists,
		const OverlayCommand &command,
		std::span<const TriangleOverlayPrimitive> triangle_payloads) {
	if (!payload_range_valid(command.payload_offset, command.payload_count, triangle_payloads.size())) {
		return;
	}

	const std::size_t kBegin = static_cast<std::size_t>(command.payload_offset);
	const std::size_t kCount = static_cast<std::size_t>(command.payload_count);
	for (std::size_t index = kBegin; index < kBegin + kCount; ++index) {
		const TriangleOverlayPrimitive &triangle = triangle_payloads[index];
		const OverlaySemanticRole kRole = effective_overlay_role(command, triangle.semantic_role);
		const OverlaySourceKind kSource = effective_overlay_source_kind(command, triangle.source_kind);
		if (overlay_role_is_source_wire_depth_stamp(kRole)) {
			append_source_wire_depth_stamp_metadata(lists, command, triangle, index, kSource);
			continue;
		}

		if (overlay_role_is_face_fill(kRole)) {
			OverlayDrawBucket &bucket = bucket_for_depth_mode(lists, OverlayDepthMode::DepthTested);
			PreparedTriangleOverlayCommand &depth_stamp = triangle_batch_for(
					bucket,
					command,
					OverlayDepthMode::DepthTested,
					kRole,
					kSource,
					PreparedOverlayPassKind::DepthStamp);
			depth_stamp.triangles.push_back(triangle);
			depth_stamp.command.payload_count = static_cast<std::uint32_t>(depth_stamp.triangles.size());

			PreparedTriangleOverlayCommand &color = triangle_batch_for(
					bucket,
					command,
					OverlayDepthMode::DepthTested,
					kRole,
					kSource,
					PreparedOverlayPassKind::EqualDepthColor);
			color.triangles.push_back(triangle);
			color.command.payload_count = static_cast<std::uint32_t>(color.triangles.size());
			continue;
		}

		const OverlayDepthMode kDepthMode = effective_overlay_depth_mode(command, kRole, kSource);
		OverlayDrawBucket &bucket = bucket_for_depth_mode(lists, kDepthMode);
		PreparedTriangleOverlayCommand &batch = triangle_batch_for(
				bucket,
				command,
				kDepthMode,
				kRole,
				kSource,
				PreparedOverlayPassKind::Color);
		batch.triangles.push_back(triangle);
		batch.command.payload_count = static_cast<std::uint32_t>(batch.triangles.size());
	}
}

[[nodiscard]] PreparedPointOverlayCommand &point_batch_for(
		OverlayDrawBucket &bucket,
		const OverlayCommand &command,
		OverlayDepthMode depth_mode,
		OverlaySemanticRole role,
		OverlaySourceKind source_kind,
		float size_px) {
	for (PreparedPointOverlayCommand &batch : bucket.point_commands) {
		if (same_command_batch_key(batch.command, command) && batch.semantic_role == role && batch.source_kind == source_kind && batch.command.depth_mode == depth_mode && batch.size_px == size_px) {
			return batch;
		}
	}

	PreparedPointOverlayCommand batch;
	batch.command = command_for_prepared_batch(command, depth_mode, role, source_kind, 0);
	batch.semantic_role = role;
	batch.source_kind = source_kind;
	batch.render_state = make_render_state(depth_mode, role, source_kind, PreparedOverlayPassKind::Color);
	batch.color_linear_sdr = to_linear_sdr_array(command.color_srgb, command.opacity);
	batch.size_px = size_px;
	bucket.point_commands.push_back(std::move(batch));
	return bucket.point_commands.back();
}

void append_point_payloads(
		OverlayDrawLists &lists,
		const OverlayCommand &command,
		std::span<const PointOverlayPrimitive> point_payloads) {
	if (!payload_range_valid(command.payload_offset, command.payload_count, point_payloads.size())) {
		return;
	}

	const std::size_t kBegin = static_cast<std::size_t>(command.payload_offset);
	const std::size_t kCount = static_cast<std::size_t>(command.payload_count);
	for (std::size_t index = kBegin; index < kBegin + kCount; ++index) {
		const PointOverlayPrimitive &point = point_payloads[index];
		const OverlaySemanticRole kRole = effective_overlay_role(command, point.semantic_role);
		const OverlaySourceKind kSource = effective_overlay_source_kind(command, point.source_kind);
		const OverlayDepthMode kDepthMode = effective_overlay_depth_mode(command, kRole, kSource);
		const float kSizePx = point.size_px > 0.0F ? point.size_px : command.thickness_px;
		OverlayDrawBucket &bucket = bucket_for_depth_mode(lists, kDepthMode);
		PreparedPointOverlayCommand &batch = point_batch_for(bucket, command, kDepthMode, kRole, kSource, kSizePx);
		batch.points.push_back(point);
		batch.command.payload_count = static_cast<std::uint32_t>(batch.points.size());
	}
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

bool overlay_role_is_source_wire_depth_stamp(OverlaySemanticRole role) noexcept {
	return role == OverlaySemanticRole::SourceWireDepthStamp;
}

bool overlay_role_is_face_fill(OverlaySemanticRole role) noexcept {
	return role == OverlaySemanticRole::SelectedFaceFill || role == OverlaySemanticRole::HoverFaceFill;
}

bool overlay_role_renders_two_sided(OverlaySemanticRole role) noexcept {
	return overlay_role_is_face_fill(role);
}

bool overlay_role_is_source_wire(OverlaySemanticRole role) noexcept {
	return role == OverlaySemanticRole::SourceWire;
}

bool overlay_role_is_component_handle(OverlaySemanticRole role) noexcept {
	return role == OverlaySemanticRole::SelectedFaceEdge || role == OverlaySemanticRole::HoverFaceEdge || role == OverlaySemanticRole::SelectedEdge || role == OverlaySemanticRole::HoverEdge || role == OverlaySemanticRole::SelectedVertex || role == OverlaySemanticRole::HoverVertex;
}

bool overlay_role_is_component_edge(OverlaySemanticRole role) noexcept {
	return role == OverlaySemanticRole::SelectedFaceEdge || role == OverlaySemanticRole::HoverFaceEdge || role == OverlaySemanticRole::SelectedEdge || role == OverlaySemanticRole::HoverEdge;
}

bool overlay_role_is_component_edit_wire(OverlaySemanticRole role) noexcept {
	return overlay_role_is_component_edge(role);
}

bool overlay_role_is_vertex_handle(OverlaySemanticRole role) noexcept {
	return role == OverlaySemanticRole::SourceVertex || role == OverlaySemanticRole::SelectedVertex || role == OverlaySemanticRole::HoverVertex;
}

bool overlay_source_kind_is_component(OverlaySourceKind source_kind) noexcept {
	return source_kind == OverlaySourceKind::ComponentSelection || source_kind == OverlaySourceKind::ComponentHover;
}

OverlaySemanticRole effective_overlay_role(
		const OverlayCommand &command,
		OverlaySemanticRole payload_role) noexcept {
	return payload_role == OverlaySemanticRole::Generic ? command.semantic_role : payload_role;
}

OverlaySourceKind effective_overlay_source_kind(
		const OverlayCommand &command,
		OverlaySourceKind payload_source_kind) noexcept {
	return payload_source_kind == OverlaySourceKind::Unknown ? command.source_kind : payload_source_kind;
}

OverlayDepthMode effective_overlay_depth_mode(
		const OverlayCommand &command,
		OverlaySemanticRole role,
		OverlaySourceKind source_kind) noexcept {
	if (overlay_role_is_source_wire(role)) {
		return OverlayDepthMode::AlwaysOnTop;
	}
	if (overlay_source_kind_is_component(source_kind) &&
			(overlay_role_is_component_edit_wire(role) || overlay_role_is_vertex_handle(role))) {
		return OverlayDepthMode::DepthTested;
	}
	if (overlay_role_is_vertex_handle(role)) {
		return OverlayDepthMode::AlwaysOnTop;
	}
	if (overlay_role_is_source_wire_depth_stamp(role) || overlay_role_is_face_fill(role)) {
		return OverlayDepthMode::DepthTested;
	}
	return command.depth_mode;
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
	return depth_tested.commands.size() + depth_tested.grid_commands.size() + depth_tested.line_commands.size() + depth_tested.triangle_commands.size() + depth_tested.point_commands.size() + xray.commands.size() + xray.grid_commands.size() + xray.line_commands.size() + xray.triangle_commands.size() + xray.point_commands.size() + always_on_top.commands.size() + always_on_top.grid_commands.size() + always_on_top.line_commands.size() + always_on_top.triangle_commands.size() + always_on_top.point_commands.size();
}

std::size_t OverlayDrawLists::source_wire_depth_stamp_count() const noexcept {
	return source_wire_depth_stamps.size();
}

OverlayDrawLists OverlaySystem::prepare(
		std::span<const OverlayCommand> commands,
		std::span<const GridOverlayCommand> grid_payloads,
		std::span<const LineOverlaySegment> line_payloads,
		std::span<const TriangleOverlayPrimitive> triangle_payloads,
		std::span<const PointOverlayPrimitive> point_payloads) const {
	OverlayDrawLists lists;
	for (const OverlayCommand &command : commands) {
		if (command.base_shader != BaseShaderId::OverlayUnlit) {
			continue;
		}

		if (command.primitive == OverlayPrimitive::Grid) {
			OverlayDrawBucket &bucket = bucket_for_depth_mode(lists, command.depth_mode);
			const std::array<float, 4> kColor = to_linear_sdr_array(command.color_srgb, command.opacity);
			bucket.commands.push_back(PreparedOverlayCommand{
					.command = command,
					.color_linear_sdr = ColorLinear{ kColor[0], kColor[1], kColor[2], kColor[3] },
			});
			append_grid_payloads(bucket, command, grid_payloads);
		} else if (command.primitive == OverlayPrimitive::LineList) {
			OverlayDrawBucket &bucket = bucket_for_depth_mode(lists, command.depth_mode);
			const std::array<float, 4> kColor = to_linear_sdr_array(command.color_srgb, command.opacity);
			bucket.commands.push_back(PreparedOverlayCommand{
					.command = command,
					.color_linear_sdr = ColorLinear{ kColor[0], kColor[1], kColor[2], kColor[3] },
			});
			append_line_payloads(lists, command, line_payloads);
		} else if (command.primitive == OverlayPrimitive::SolidTriangles) {
			append_triangle_payloads(lists, command, triangle_payloads);
		} else if (command.primitive == OverlayPrimitive::PointList) {
			append_point_payloads(lists, command, point_payloads);
		}
	}

	return lists;
}

} // namespace crimson
