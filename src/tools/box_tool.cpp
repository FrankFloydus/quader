/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "tools/box_tool.hpp"

#include "commands/document_commands.hpp"
#include "mesh/ops/box_builder.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace quader::tools {
namespace {

constexpr float kMinimumBoxSize = 0.0001F;

struct ConstructionPoint {
	bool ok = false;
	quader::math::Vec3 point;
};

enum class SurfaceHitFallback {
	Disabled,
	Enabled,
};

[[nodiscard]] float grid_or_default(float grid_size) noexcept {
	return quader::math::is_finite(grid_size) && grid_size > kMinimumBoxSize ? grid_size : 1.0F;
}

[[nodiscard]] quader::math::Vec3 scale(quader::math::Vec3 value, float factor) noexcept {
	return quader::math::Vec3{ value.x * factor, value.y * factor, value.z * factor };
}

[[nodiscard]] bool tiny(quader::math::Vec3 value) noexcept {
	return quader::math::length_squared(value) <= kMinimumBoxSize * kMinimumBoxSize;
}

[[nodiscard]] quader::math::Vec3 normalized_axis(quader::math::Vec3 value,
		quader::math::Vec3 fallback = { 0.0F, 1.0F, 0.0F }) noexcept {
	const quader::math::Vec3 kNormalized = quader::math::normalized(value, kMinimumBoxSize);
	return tiny(kNormalized) ? fallback : kNormalized;
}

[[nodiscard]] quader::math::Vec3 project_to_plane(quader::math::Vec3 point,
		const BoxConstructionPlane &plane) noexcept {
	const float kDistance = quader::math::dot(point - plane.origin, plane.normal);
	return point - scale(plane.normal, kDistance);
}

[[nodiscard]] std::optional<quader::math::Vec3> ray_plane_point(
		const BoxConstructionPlane &plane,
		const ViewportRay &ray) noexcept {
	const quader::math::Vec3 kDirection = normalized_axis(ray.direction, {});
	if (tiny(kDirection)) {
		return std::nullopt;
	}

	const float kDenom = quader::math::dot(kDirection, plane.normal);
	if (std::abs(kDenom) <= kMinimumBoxSize) {
		return std::nullopt;
	}

	const float kT = quader::math::dot(plane.origin - ray.origin, plane.normal) / kDenom;
	if (kT < 0.0F || !quader::math::is_finite(kT)) {
		return std::nullopt;
	}

	return project_to_plane(ray.origin + scale(kDirection, kT), plane);
}

[[nodiscard]] ConstructionPoint construction_point_from_event(const BoxConstructionPlane &plane,
		const PointerEvent &event,
		SurfaceHitFallback surface_hit_fallback) noexcept {
	if (event.ray.has_value()) {
		if (const auto kPoint = ray_plane_point(plane, *event.ray)) {
			return ConstructionPoint{
				.ok = true,
				.point = *kPoint,
			};
		}
	}

	if (surface_hit_fallback == SurfaceHitFallback::Enabled && event.surface_hit.has_value()) {
		return ConstructionPoint{
			.ok = true,
			.point = project_to_plane(event.surface_hit->position, plane),
		};
	}

	return {};
}

[[nodiscard]] std::array<float, 2> directional_snap_pair(float start_value,
		float end_value,
		float grid_size) noexcept {
	const float kGrid = grid_or_default(grid_size);
	if (end_value >= start_value) {
		const float kStart = std::floor(start_value / kGrid) * kGrid;
		float end = std::ceil(end_value / kGrid) * kGrid;
		if (end - kStart <= kMinimumBoxSize) {
			end = kStart + kGrid;
		}
		return { kStart, end };
	}

	const float kStart = std::ceil(start_value / kGrid) * kGrid;
	float end = std::floor(end_value / kGrid) * kGrid;
	if (kStart - end <= kMinimumBoxSize) {
		end = kStart - kGrid;
	}
	return { kStart, end };
}

[[nodiscard]] quader::math::Vec3 plane_point(const BoxConstructionPlane &plane,
		float u,
		float v) noexcept {
	return plane.snap_origin + scale(plane.axis_u, u) + scale(plane.axis_v, v);
}

[[nodiscard]] float plane_coordinate(quader::math::Vec3 point,
		quader::math::Vec3 origin,
		quader::math::Vec3 axis) noexcept {
	return quader::math::dot(point - origin, axis);
}

[[nodiscard]] std::array<quader::math::Vec3, 4> footprint_points(quader::math::Vec3 start,
		quader::math::Vec3 end,
		quader::math::Vec3 axis_u,
		quader::math::Vec3 axis_v) noexcept {
	const quader::math::Vec3 kDelta = end - start;
	const quader::math::Vec3 kU = scale(axis_u, quader::math::dot(kDelta, axis_u));
	const quader::math::Vec3 kV = scale(axis_v, quader::math::dot(kDelta, axis_v));
	return {
		start,
		start + kV,
		start + kU + kV,
		start + kU,
	};
}

[[nodiscard]] bool footprint_has_area(const std::array<quader::math::Vec3, 4> &footprint) noexcept {
	return !tiny(quader::math::cross(footprint[1] - footprint[0], footprint[3] - footprint[0]));
}

[[nodiscard]] std::array<quader::math::Vec3, 8> preview_corners(const BoxToolState &state) noexcept {
	const quader::math::Vec3 kNormal = state.height >= 0.0F ? state.plane.normal : scale(state.plane.normal, -1.0F);
	const quader::math::Vec3 kHeight = scale(kNormal, std::abs(state.height));
	return {
		state.footprint[0],
		state.footprint[1],
		state.footprint[2],
		state.footprint[3],
		state.footprint[0] + kHeight,
		state.footprint[1] + kHeight,
		state.footprint[2] + kHeight,
		state.footprint[3] + kHeight,
	};
}

} // namespace

BoxConstructionPlane make_box_construction_plane(quader::math::Vec3 origin,
		quader::math::Vec3 normal) {
	BoxConstructionPlane plane;
	plane.origin = origin;
	plane.normal = normalized_axis(normal);
	plane.snap_origin = project_to_plane({}, plane);

	quader::math::Vec3 preferred{ 1.0F, 0.0F, 0.0F };
	if (std::abs(quader::math::dot(preferred, plane.normal)) > 0.9F) {
		preferred = { 0.0F, 0.0F, 1.0F };
	}

	const quader::math::Vec3 kProjected = preferred - scale(plane.normal, quader::math::dot(preferred, plane.normal));
	plane.axis_u = normalized_axis(kProjected, { 1.0F, 0.0F, 0.0F });
	plane.axis_v = normalized_axis(quader::math::cross(plane.normal, plane.axis_u),
			{ 0.0F, 0.0F, 1.0F });
	return plane;
}

ToolId BoxTool::id() const noexcept {
	return ToolId::Box;
}

void BoxTool::activate(ToolContext &context) {
	(void)context;
	reset();
}

void BoxTool::deactivate(ToolContext &context) {
	(void)context;
	reset();
}

void BoxTool::cancel(ToolContext &context) {
	(void)context;
	reset();
}

bool BoxTool::on_pointer_event(const PointerEvent &event, ToolContext &context) {
	if (event.navigation_active) {
		return false;
	}

	if (event.phase == PointerPhase::Hover || event.phase == PointerPhase::Move) {
		return state_.stage == BoxToolStage::Idle ? update_hover(event)
												  : update_footprint(event);
	}

	if (event.button != PointerButton::Left) {
		return false;
	}

	if (event.phase == PointerPhase::Press && event.pressed) {
		if (state_.stage == BoxToolStage::Idle) {
			return begin_footprint(event);
		}
		return finish_footprint_and_commit(event, context);
	}

	if (event.phase == PointerPhase::Release && state_.stage == BoxToolStage::Footprint) {
		return finish_footprint_and_commit(event, context);
	}

	return false;
}

bool BoxTool::on_key_event(const KeyEvent &event, ToolContext &context) {
	(void)context;
	if (!event.pressed || event.auto_repeat || event.key_code != 27) {
		return false;
	}

	reset();
	return true;
}

ToolPreview BoxTool::preview() const {
	return preview_;
}

const BoxToolState &BoxTool::state() const noexcept {
	return state_;
}

bool BoxTool::update_hover(const PointerEvent &event) {
	const quader::math::Vec3 kOrigin = event.surface_hit.has_value() ? event.surface_hit->position
																	 : quader::math::Vec3{};
	const quader::math::Vec3 kNormal = event.surface_hit.has_value() ? event.surface_hit->normal
																	 : quader::math::Vec3{ 0.0F, 1.0F, 0.0F };
	state_.plane = make_box_construction_plane(kOrigin, kNormal);
	const ConstructionPoint kPoint = construction_point_from_event(
			state_.plane,
			event,
			SurfaceHitFallback::Enabled);
	if (!kPoint.ok) {
		preview_.clear();
		return false;
	}

	state_.raw_start = kPoint.point;
	state_.raw_end = kPoint.point;
	state_.view_index = event.view_index;
	if (!update_footprint(PointerEvent{
				.position = event.position,
				.button = PointerButton::None,
				.pressed = false,
				.phase = PointerPhase::Move,
				.navigation_active = false,
				.snap_to_grid = event.snap_to_grid,
				.grid_size = event.grid_size,
				.ray = event.ray,
				.surface_hit = event.surface_hit,
				.view_index = event.view_index,
		})) {
		preview_.clear();
		return false;
	}
	state_.hover_cell = state_.footprint;
	state_.stage = BoxToolStage::Idle;
	rebuild_preview();
	return true;
}

bool BoxTool::begin_footprint(const PointerEvent &event) {
	const quader::math::Vec3 kOrigin = event.surface_hit.has_value() ? event.surface_hit->position
																	 : quader::math::Vec3{};
	const quader::math::Vec3 kNormal = event.surface_hit.has_value() ? event.surface_hit->normal
																	 : quader::math::Vec3{ 0.0F, 1.0F, 0.0F };
	state_.plane = make_box_construction_plane(kOrigin, kNormal);
	const ConstructionPoint kPoint = construction_point_from_event(
			state_.plane,
			event,
			SurfaceHitFallback::Enabled);
	if (!kPoint.ok) {
		return false;
	}

	state_.stage = BoxToolStage::Footprint;
	state_.raw_start = kPoint.point;
	state_.raw_end = kPoint.point;
	state_.height = 0.0F;
	state_.view_index = event.view_index;
	state_.preview_valid = false;
	return update_footprint(event);
}

bool BoxTool::update_footprint(const PointerEvent &event) {
	const ConstructionPoint kPoint = construction_point_from_event(
			state_.plane,
			event,
			state_.stage == BoxToolStage::Footprint ? SurfaceHitFallback::Disabled : SurfaceHitFallback::Enabled);
	if (!kPoint.ok) {
		return false;
	}

	state_.raw_end = kPoint.point;
	state_.view_index = event.view_index;
	if (event.snap_to_grid) {
		const float kStartU = plane_coordinate(state_.raw_start, state_.plane.snap_origin, state_.plane.axis_u);
		const float kEndU = plane_coordinate(state_.raw_end, state_.plane.snap_origin, state_.plane.axis_u);
		const float kStartV = plane_coordinate(state_.raw_start, state_.plane.snap_origin, state_.plane.axis_v);
		const float kEndV = plane_coordinate(state_.raw_end, state_.plane.snap_origin, state_.plane.axis_v);
		const auto kUPair = directional_snap_pair(kStartU, kEndU, event.grid_size);
		const auto kVPair = directional_snap_pair(kStartV, kEndV, event.grid_size);
		state_.start = plane_point(state_.plane, kUPair[0], kVPair[0]);
		state_.end = plane_point(state_.plane, kUPair[1], kVPair[1]);
	} else {
		state_.start = project_to_plane(state_.raw_start, state_.plane);
		state_.end = project_to_plane(state_.raw_end, state_.plane);
	}

	state_.height = grid_or_default(event.grid_size);
	state_.footprint = footprint_points(state_.start, state_.end, state_.plane.axis_u, state_.plane.axis_v);
	state_.preview_valid = footprint_has_area(state_.footprint);
	rebuild_preview();
	return true;
}

bool BoxTool::finish_footprint_and_commit(const PointerEvent &event, ToolContext &context) {
	if (state_.stage != BoxToolStage::Footprint || !update_footprint(event)) {
		return false;
	}

	state_.height = grid_or_default(event.grid_size);
	return commit(context);
}

bool BoxTool::commit(ToolContext &context) {
	if (state_.stage != BoxToolStage::Footprint || !state_.preview_valid) {
		return false;
	}

	const auto kCorners = preview_corners(state_);
	auto mesh = quader::mesh::ops::make_box_from_corners(quader::mesh::ops::BoxCorners{ kCorners });
	if (!mesh) {
		reset();
		return false;
	}

	auto result = context.execute_command(std::make_unique<quader::commands::CreateMeshObjectCommand>(
			"Box",
			std::move(mesh).value(),
			quader::document::Transform{},
			quader::document::default_box_material()));
	const bool kApplied = result.is_applied();
	reset();
	return kApplied;
}

void BoxTool::reset() {
	state_ = BoxToolState{};
	preview_.clear();
}

void BoxTool::rebuild_preview() {
	preview_.clear();

	if (state_.stage == BoxToolStage::Idle && !footprint_has_area(state_.hover_cell)) {
		return;
	}

	const std::array<quader::math::Vec3, 4> &footprint = state_.stage == BoxToolStage::Idle
			? state_.hover_cell
			: state_.footprint;
	if (!footprint_has_area(footprint)) {
		return;
	}

	preview_.active = true;
	preview_.overlay_only = true;
	preview_.status_text = "Box footprint";
	preview_.view_index = state_.view_index;
	for (std::size_t index = 0; index < footprint.size(); ++index) {
		preview_.world_points.push_back(footprint[index]);
		preview_.world_segments.push_back(ToolPreviewWorldSegment{
				.start = footprint[index],
				.end = footprint[(index + 1U) % footprint.size()],
		});
	}

	if (state_.stage == BoxToolStage::Footprint && state_.preview_valid) {
		preview_.boxes.push_back(ToolPreviewBox{
				.corners = preview_corners(state_),
				.active = true,
		});
	}

}

} // namespace quader::tools
