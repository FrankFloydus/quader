/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "tools/transform_gizmo_tool.hpp"

#include "commands/document_commands.hpp"
#include "document/document.hpp"
#include "math/aabb.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace quader::tools {
namespace {

constexpr float kToolEpsilon = 0.000001F;
constexpr float kScreenEpsilon = 0.0001F;
constexpr float kPi = 3.14159265358979323846F;
constexpr float kTau = 6.28318530717958647692F;
constexpr float kMovePixelsPerGridStep = 48.0F;
constexpr float kScalePixelsPerFactor = 96.0F;
constexpr float kMinScaleFactor = 0.01F;
constexpr float kRotationSnapDegrees = 15.0F;

constexpr float kGizmoCircleSize = 1.1F;
constexpr float kGizmoPlaneSize = 0.2F;
constexpr float kGizmoPlaneDistance = 0.3F;
constexpr float kGizmoArrowSize = 0.35F;
constexpr float kGizmoArrowOffset = kGizmoCircleSize + 0.3F;
constexpr float kGizmoArrowTip = kGizmoArrowOffset + kGizmoArrowSize;
constexpr float kGizmoScaleOffset = kGizmoCircleSize + 0.3F;
constexpr float kGizmoScaleTip = kGizmoScaleOffset * 1.11F;
constexpr float kGizmoViewRotationScale = 1.14F;
constexpr float kGizmoArrowConeRadius = 0.065F;
constexpr float kGizmoScaleHalfWidth = 0.07F;

constexpr float kGizmoSizePixels = 80.0F;
constexpr float kGizmoPickRadiusPixels = 12.0F;
constexpr float kGizmoScaleHandleSizePixels = 11.2F;
constexpr float kGizmoScaleCenterSizePixels = 10.0F;
constexpr float kGizmoAxisLineWidthPixels = 1.6F;
constexpr float kGizmoPlaneLineWidthPixels = 1.2F;
constexpr float kGizmoTrackballLineWidthPixels = 2.25F;
constexpr float kGizmoRotateRingLineWidthPixels = 3.0F;

constexpr ToolPreviewColorSrgb kAxisXColor{ 245.0F / 255.0F, 51.0F / 255.0F, 82.0F / 255.0F, 230.0F / 255.0F };
constexpr ToolPreviewColorSrgb kAxisYColor{ 135.0F / 255.0F, 214.0F / 255.0F, 3.0F / 255.0F, 230.0F / 255.0F };
constexpr ToolPreviewColorSrgb kAxisZColor{ 41.0F / 255.0F, 140.0F / 255.0F, 245.0F / 255.0F, 230.0F / 255.0F };
constexpr ToolPreviewColorSrgb kAxisMixedColor{ 238.0F / 255.0F, 242.0F / 255.0F, 247.0F / 255.0F, 230.0F / 255.0F };
constexpr ToolPreviewColorSrgb kAxisXHighlightColor{ 255.0F / 255.0F, 204.0F / 255.0F, 211.0F / 255.0F, 1.0F };
constexpr ToolPreviewColorSrgb kAxisYHighlightColor{ 230.0F / 255.0F, 255.0F / 255.0F, 192.0F / 255.0F, 1.0F };
constexpr ToolPreviewColorSrgb kAxisZHighlightColor{ 202.0F / 255.0F, 228.0F / 255.0F, 255.0F / 255.0F, 1.0F };
constexpr ToolPreviewColorSrgb kAxisHighlightColor{ 255.0F / 255.0F, 250.0F / 255.0F, 204.0F / 255.0F, 1.0F };
constexpr ToolPreviewColorSrgb kTrackballColor{ 210.0F / 255.0F, 214.0F / 255.0F, 219.0F / 255.0F, 138.0F / 255.0F };
constexpr ToolPreviewColorSrgb kRotatePieFillColor{ 156.0F / 255.0F, 162.0F / 255.0F, 168.0F / 255.0F, 92.0F / 255.0F };
constexpr ToolPreviewColorSrgb kRotatePieLineColor{ 218.0F / 255.0F, 224.0F / 255.0F, 230.0F / 255.0F, 188.0F / 255.0F };
constexpr ToolPreviewColorSrgb kCubeOutlineColor{ 255.0F / 255.0F, 246.0F / 255.0F, 128.0F / 255.0F, 180.0F / 255.0F };

struct CameraBasis {
	quader::math::Vec3 eye;
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
	quader::math::Vec3 right{ -1.0F, 0.0F, 0.0F };
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
};

struct ProjectedPoint {
	quader::math::Vec2 position;
	float camera_distance = 0.0F;
};

struct GizmoAxisFrame {
	TransformGizmoAxis axis = TransformGizmoAxis::X;
	quader::math::Vec3 world_start;
	quader::math::Vec3 world_tip;
	quader::math::Vec2 screen_start;
	quader::math::Vec2 screen_tip;
};

struct GizmoPlaneFrame {
	TransformGizmoAxis axis = TransformGizmoAxis::Xy;
	std::array<quader::math::Vec3, 4> world;
	std::array<quader::math::Vec2, 4> screen;
};

struct GizmoRingSegmentFrame {
	TransformGizmoAxis axis = TransformGizmoAxis::X;
	quader::math::Vec3 world_start;
	quader::math::Vec3 world_end;
	quader::math::Vec2 screen_start;
	quader::math::Vec2 screen_end;
	bool front_facing = true;
};

struct GizmoFrame {
	bool ok = false;
	quader::math::Vec3 pivot;
	quader::math::Vec2 screen_pivot;
	ViewportCameraInput camera;
	CameraBasis basis;
	float unit_screen_size = kGizmoSizePixels;
	float pixel_world = 1.0F;
	float width_scale = 1.0F;
	std::vector<GizmoAxisFrame> axes;
	std::vector<GizmoPlaneFrame> planes;
	std::vector<GizmoRingSegmentFrame> rings;
	std::vector<GizmoRingSegmentFrame> trackball;
};

struct ObjectBounds {
	quader::math::Aabb bounds;
	bool valid = false;
};

struct CombinedBounds {
	quader::math::Aabb bounds;
	bool valid = false;
};

[[nodiscard]] quader::math::Vec2 scale2(quader::math::Vec2 value, float factor) noexcept {
	return { value.x * factor, value.y * factor };
}

[[nodiscard]] float length2(quader::math::Vec2 value) noexcept {
	return std::sqrt(std::max(0.0F, quader::math::length_squared(value)));
}

[[nodiscard]] quader::math::Vec2 normalized2_or(quader::math::Vec2 value,
		quader::math::Vec2 fallback) noexcept {
	const float kLength = length2(value);
	if (kLength <= kScreenEpsilon) {
		return fallback;
	}
	return { value.x / kLength, value.y / kLength };
}

[[nodiscard]] quader::math::Vec3 scale3(quader::math::Vec3 value, float factor) noexcept {
	return { value.x * factor, value.y * factor, value.z * factor };
}

[[nodiscard]] quader::math::Vec3 multiply_components(quader::math::Vec3 left,
		quader::math::Vec3 right) noexcept {
	return { left.x * right.x, left.y * right.y, left.z * right.z };
}

[[nodiscard]] bool tiny(quader::math::Vec3 value) noexcept {
	return quader::math::length_squared(value) <= kToolEpsilon * kToolEpsilon;
}

[[nodiscard]] quader::math::Vec3 normalized_or(quader::math::Vec3 value,
		quader::math::Vec3 fallback) noexcept {
	const quader::math::Vec3 kNormalized = quader::math::normalized(value, kToolEpsilon);
	return tiny(kNormalized) ? fallback : kNormalized;
}

[[nodiscard]] float radians_from_degrees(float degrees) noexcept {
	return degrees * kPi / 180.0F;
}

[[nodiscard]] float degrees_from_radians(float radians) noexcept {
	return radians * 180.0F / kPi;
}

[[nodiscard]] float wrap_radians(float radians) noexcept {
	float value = std::fmod(radians + kPi, kTau);
	if (value < 0.0F) {
		value += kTau;
	}
	return value - kPi;
}

[[nodiscard]] float clamp_unit(float value) noexcept {
	return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] ToolPreviewColorSrgb multiply_color(ToolPreviewColorSrgb color,
		float rgb_multiplier,
		float alpha_multiplier = 1.0F) noexcept {
	return ToolPreviewColorSrgb{
		clamp_unit(color.red * rgb_multiplier),
		clamp_unit(color.green * rgb_multiplier),
		clamp_unit(color.blue * rgb_multiplier),
		clamp_unit(color.alpha * alpha_multiplier),
	};
}

[[nodiscard]] quader::math::Vec3 rotate_x(quader::math::Vec3 value, float radians) noexcept {
	const float kSin = std::sin(radians);
	const float kCos = std::cos(radians);
	return { value.x, value.y * kCos - value.z * kSin, value.y * kSin + value.z * kCos };
}

[[nodiscard]] quader::math::Vec3 rotate_y(quader::math::Vec3 value, float radians) noexcept {
	const float kSin = std::sin(radians);
	const float kCos = std::cos(radians);
	return { value.x * kCos + value.z * kSin, value.y, -value.x * kSin + value.z * kCos };
}

[[nodiscard]] quader::math::Vec3 rotate_z(quader::math::Vec3 value, float radians) noexcept {
	const float kSin = std::sin(radians);
	const float kCos = std::cos(radians);
	return { value.x * kCos - value.y * kSin, value.x * kSin + value.y * kCos, value.z };
}

[[nodiscard]] quader::math::Vec3 rotate_euler(quader::math::Vec3 value,
		quader::math::Vec3 degrees) noexcept {
	value = rotate_x(value, radians_from_degrees(degrees.x));
	value = rotate_y(value, radians_from_degrees(degrees.y));
	return rotate_z(value, radians_from_degrees(degrees.z));
}

[[nodiscard]] quader::math::Vec3 transform_point(quader::math::Vec3 point,
		quader::document::Transform transform) noexcept {
	point = multiply_components(point, transform.scale);
	return rotate_euler(point, transform.rotation_euler) + transform.translation;
}

[[nodiscard]] quader::math::Vec3 rotate_around_axis(
		quader::math::Vec3 value,
		quader::math::Vec3 axis,
		float radians) noexcept {
	const quader::math::Vec3 kAxis = normalized_or(axis, { 0.0F, 1.0F, 0.0F });
	const float kSin = std::sin(radians);
	const float kCos = std::cos(radians);
	return scale3(value, kCos) +
			scale3(quader::math::cross(kAxis, value), kSin) +
			scale3(kAxis, quader::math::dot(kAxis, value) * (1.0F - kCos));
}

[[nodiscard]] TransformGizmoMode mode_from_tool_id(ToolId id) noexcept {
	switch (id) {
		case ToolId::Rotate:
			return TransformGizmoMode::Rotate;
		case ToolId::Scale:
			return TransformGizmoMode::Scale;
		case ToolId::Select:
		case ToolId::Move:
		case ToolId::Box:
			return TransformGizmoMode::Move;
	}
	return TransformGizmoMode::Move;
}

[[nodiscard]] std::array<TransformGizmoAxis, 3> principal_axes() noexcept {
	return { TransformGizmoAxis::X, TransformGizmoAxis::Y, TransformGizmoAxis::Z };
}

[[nodiscard]] std::array<TransformGizmoAxis, 3> plane_axes() noexcept {
	return { TransformGizmoAxis::Xy, TransformGizmoAxis::Xz, TransformGizmoAxis::Yz };
}

[[nodiscard]] bool is_principal_axis(TransformGizmoAxis axis) noexcept {
	return axis == TransformGizmoAxis::X || axis == TransformGizmoAxis::Y || axis == TransformGizmoAxis::Z;
}

[[nodiscard]] bool is_plane_axis(TransformGizmoAxis axis) noexcept {
	return axis == TransformGizmoAxis::Xy || axis == TransformGizmoAxis::Xz || axis == TransformGizmoAxis::Yz;
}

[[nodiscard]] quader::math::Vec3 axis_vector(TransformGizmoAxis axis) noexcept {
	switch (axis) {
		case TransformGizmoAxis::X:
			return { 1.0F, 0.0F, 0.0F };
		case TransformGizmoAxis::Y:
			return { 0.0F, 1.0F, 0.0F };
		case TransformGizmoAxis::Z:
			return { 0.0F, 0.0F, 1.0F };
		case TransformGizmoAxis::Xy:
			return { 1.0F, 1.0F, 0.0F };
		case TransformGizmoAxis::Xz:
			return { 1.0F, 0.0F, 1.0F };
		case TransformGizmoAxis::Yz:
			return { 0.0F, 1.0F, 1.0F };
		case TransformGizmoAxis::All:
			return { 1.0F, 1.0F, 1.0F };
		case TransformGizmoAxis::None:
			break;
	}
	return {};
}

[[nodiscard]] quader::math::Vec3 rotation_axis_vector(TransformGizmoAxis axis) noexcept {
	return is_principal_axis(axis) ? axis_vector(axis) : quader::math::Vec3{};
}

[[nodiscard]] std::array<TransformGizmoAxis, 2> components_for_plane(
		TransformGizmoAxis axis) noexcept {
	switch (axis) {
		case TransformGizmoAxis::Xy:
			return { TransformGizmoAxis::X, TransformGizmoAxis::Y };
		case TransformGizmoAxis::Xz:
			return { TransformGizmoAxis::X, TransformGizmoAxis::Z };
		case TransformGizmoAxis::Yz:
			return { TransformGizmoAxis::Y, TransformGizmoAxis::Z };
		case TransformGizmoAxis::X:
		case TransformGizmoAxis::Y:
		case TransformGizmoAxis::Z:
		case TransformGizmoAxis::All:
		case TransformGizmoAxis::None:
			break;
	}
	return { TransformGizmoAxis::X, TransformGizmoAxis::Y };
}

[[nodiscard]] TransformGizmoAxis normal_for_plane(TransformGizmoAxis axis) noexcept {
	switch (axis) {
		case TransformGizmoAxis::Xy:
			return TransformGizmoAxis::Z;
		case TransformGizmoAxis::Xz:
			return TransformGizmoAxis::Y;
		case TransformGizmoAxis::Yz:
			return TransformGizmoAxis::X;
		case TransformGizmoAxis::X:
		case TransformGizmoAxis::Y:
		case TransformGizmoAxis::Z:
		case TransformGizmoAxis::All:
		case TransformGizmoAxis::None:
			break;
	}
	return TransformGizmoAxis::Z;
}

[[nodiscard]] ToolPreviewColorSrgb color_for_axis(TransformGizmoAxis axis) noexcept {
	switch (axis) {
		case TransformGizmoAxis::X:
			return kAxisXColor;
		case TransformGizmoAxis::Y:
			return kAxisYColor;
		case TransformGizmoAxis::Z:
			return kAxisZColor;
		case TransformGizmoAxis::Xy:
		case TransformGizmoAxis::Xz:
		case TransformGizmoAxis::Yz:
		case TransformGizmoAxis::All:
		case TransformGizmoAxis::None:
			break;
	}
	return kAxisMixedColor;
}

[[nodiscard]] ToolPreviewColorSrgb color_for_plane(TransformGizmoAxis axis) noexcept {
	return color_for_axis(normal_for_plane(axis));
}

[[nodiscard]] ToolPreviewColorSrgb highlight_color_for_axis(TransformGizmoAxis axis) noexcept {
	switch (axis) {
		case TransformGizmoAxis::X:
		case TransformGizmoAxis::Yz:
			return kAxisXHighlightColor;
		case TransformGizmoAxis::Y:
		case TransformGizmoAxis::Xz:
			return kAxisYHighlightColor;
		case TransformGizmoAxis::Z:
		case TransformGizmoAxis::Xy:
			return kAxisZHighlightColor;
		case TransformGizmoAxis::All:
		case TransformGizmoAxis::None:
			break;
	}
	return kAxisHighlightColor;
}

[[nodiscard]] TransformGizmoAxis axis_from_handle(TransformGizmoHandle handle) noexcept {
	switch (handle.kind) {
		case TransformGizmoHandleKind::Axis:
		case TransformGizmoHandleKind::Plane:
		case TransformGizmoHandleKind::RotationRing:
			return handle.axis;
		case TransformGizmoHandleKind::Center:
			return TransformGizmoAxis::All;
		case TransformGizmoHandleKind::None:
			break;
	}
	return TransformGizmoAxis::None;
}

[[nodiscard]] bool highlighted(const TransformGizmoToolState &state,
		TransformGizmoAxis axis) noexcept {
	const TransformGizmoAxis kActive = axis_from_handle(state.active_handle);
	const TransformGizmoAxis kHover = axis_from_handle(state.hover_handle);
	return kActive == axis || (!state.dragging && kHover == axis);
}

[[nodiscard]] bool drag_axis_includes(TransformGizmoAxis active,
		TransformGizmoAxis axis) noexcept {
	if (active == TransformGizmoAxis::None || active == TransformGizmoAxis::All) {
		return true;
	}
	if (is_plane_axis(active)) {
		const auto kComponents = components_for_plane(active);
		return axis == kComponents[0] || axis == kComponents[1];
	}
	return active == axis;
}

[[nodiscard]] bool drag_plane_includes(TransformGizmoAxis active,
		TransformGizmoAxis plane) noexcept {
	return active == TransformGizmoAxis::None || active == TransformGizmoAxis::All || active == plane;
}

[[nodiscard]] float axis_component(quader::math::Vec3 value,
		TransformGizmoAxis axis) noexcept {
	switch (axis) {
		case TransformGizmoAxis::X:
			return value.x;
		case TransformGizmoAxis::Y:
			return value.y;
		case TransformGizmoAxis::Z:
			return value.z;
		case TransformGizmoAxis::Xy:
		case TransformGizmoAxis::Xz:
		case TransformGizmoAxis::Yz:
		case TransformGizmoAxis::All:
		case TransformGizmoAxis::None:
			break;
	}
	return 0.0F;
}

void set_axis_component(quader::math::Vec3 &value,
		TransformGizmoAxis axis,
		float component) noexcept {
	switch (axis) {
		case TransformGizmoAxis::X:
			value.x = component;
			return;
		case TransformGizmoAxis::Y:
			value.y = component;
			return;
		case TransformGizmoAxis::Z:
			value.z = component;
			return;
		case TransformGizmoAxis::Xy:
		case TransformGizmoAxis::Xz:
		case TransformGizmoAxis::Yz:
		case TransformGizmoAxis::All:
		case TransformGizmoAxis::None:
			return;
	}
}

[[nodiscard]] bool axis_includes_component(TransformGizmoAxis axis,
		TransformGizmoAxis component) noexcept {
	if (axis == TransformGizmoAxis::All || axis == component) {
		return true;
	}
	if (!is_plane_axis(axis)) {
		return false;
	}
	const auto kComponents = components_for_plane(axis);
	return component == kComponents[0] || component == kComponents[1];
}

[[nodiscard]] float grid_or_default(float grid_size) noexcept {
	return quader::math::is_finite(grid_size) && grid_size > kToolEpsilon ? grid_size : 1.0F;
}

[[nodiscard]] float snap_value(float value, float step) noexcept {
	const float kStep = std::max(step, kToolEpsilon);
	return std::round(value / kStep) * kStep;
}

[[nodiscard]] CameraBasis make_camera_basis(const ViewportCameraInput &camera) noexcept {
	CameraBasis basis;
	basis.eye = camera.eye;
	basis.forward = normalized_or(camera.target - camera.eye,
			normalized_or(camera.forward, { 0.0F, 0.0F, -1.0F }));
	basis.right = normalized_or(quader::math::cross(camera.up, basis.forward), { -1.0F, 0.0F, 0.0F });
	basis.up = normalized_or(quader::math::cross(basis.forward, basis.right), { 0.0F, 1.0F, 0.0F });
	return basis;
}

[[nodiscard]] float world_units_per_pixel(
		const ViewportCameraInput &camera,
		quader::math::Vec3 world_position) noexcept {
	const float kViewportHeightPx = std::max(1.0F, camera.viewport_size_pixels.y);
	if (camera.projection == ViewportCameraProjection::Orthographic) {
		return std::max(0.01F, camera.orthographic_size) / kViewportHeightPx;
	}

	const CameraBasis kBasis = make_camera_basis(camera);
	const float kForwardDistance = quader::math::dot(world_position - camera.eye, kBasis.forward);
	const float kDistance = std::max(0.001F, kForwardDistance);
	const float kFovRadians = radians_from_degrees(std::clamp(camera.fov_degrees, 1.0F, 179.0F));
	return (2.0F * std::tan(kFovRadians * 0.5F) * kDistance) / kViewportHeightPx;
}

[[nodiscard]] std::optional<ProjectedPoint> project_world_to_viewport(
		const ViewportCameraInput &camera,
		quader::math::Vec3 world_position) noexcept {
	const float kViewportWidthPx = std::max(1.0F, camera.viewport_size_pixels.x);
	const float kViewportHeightPx = std::max(1.0F, camera.viewport_size_pixels.y);
	const float kAspect = kViewportWidthPx / kViewportHeightPx;
	const CameraBasis kBasis = make_camera_basis(camera);
	const quader::math::Vec3 kCameraRelative = world_position - camera.eye;
	const float kCameraDistance = quader::math::dot(kCameraRelative, kBasis.forward);
	if (!quader::math::is_finite(kCameraDistance)) {
		return std::nullopt;
	}

	float ndc_x = 0.0F;
	float ndc_y = 0.0F;
	if (camera.projection == ViewportCameraProjection::Orthographic) {
		const float kExtent = std::max(0.01F, camera.orthographic_size) * 0.5F;
		ndc_x = quader::math::dot(kCameraRelative, kBasis.right) / (kExtent * kAspect);
		ndc_y = quader::math::dot(kCameraRelative, kBasis.up) / kExtent;
	} else {
		if (kCameraDistance <= kToolEpsilon) {
			return std::nullopt;
		}
		const float kTanY = std::tan(radians_from_degrees(std::clamp(camera.fov_degrees, 1.0F, 179.0F)) * 0.5F);
		const float kTanX = kTanY * kAspect;
		ndc_x = quader::math::dot(kCameraRelative, kBasis.right) / (kCameraDistance * kTanX);
		ndc_y = quader::math::dot(kCameraRelative, kBasis.up) / (kCameraDistance * kTanY);
	}

	if (!quader::math::is_finite(ndc_x) || !quader::math::is_finite(ndc_y)) {
		return std::nullopt;
	}
	return ProjectedPoint{
		.position = {
				(ndc_x + 1.0F) * 0.5F * kViewportWidthPx,
				(1.0F - (ndc_y + 1.0F) * 0.5F) * kViewportHeightPx,
		},
		.camera_distance = kCameraDistance,
	};
}

[[nodiscard]] std::optional<ViewportCameraInput> camera_for_event(
		const PointerEvent &event) noexcept {
	if (event.camera.has_value()) {
		ViewportCameraInput camera = *event.camera;
		camera.viewport_size_pixels.x = std::max(1.0F, camera.viewport_size_pixels.x);
		camera.viewport_size_pixels.y = std::max(1.0F, camera.viewport_size_pixels.y);
		return camera;
	}
	if (!event.ray.has_value()) {
		return std::nullopt;
	}

	const quader::math::Vec3 kForward = normalized_or(event.ray->direction, { 0.0F, 0.0F, -1.0F });
	quader::math::Vec3 up = { 0.0F, 1.0F, 0.0F };
	if (std::abs(quader::math::dot(up, kForward)) > 0.98F) {
		up = { 0.0F, 0.0F, 1.0F };
	}
	return ViewportCameraInput{
		.eye = event.ray->origin,
		.target = event.ray->origin + scale3(kForward, 10.0F),
		.up = up,
		.forward = kForward,
		.projection = ViewportCameraProjection::Perspective,
		.fov_degrees = 60.0F,
		.orthographic_size = 24.0F,
		.viewport_size_pixels = { 640.0F, 480.0F },
	};
}

[[nodiscard]] std::optional<quader::math::Vec3> ray_plane_intersection(
		const ViewportRay &ray,
		quader::math::Vec3 point,
		quader::math::Vec3 normal) noexcept {
	const quader::math::Vec3 kDirection = normalized_or(ray.direction, {});
	const quader::math::Vec3 kNormal = normalized_or(normal, { 0.0F, 1.0F, 0.0F });
	if (tiny(kDirection) || tiny(kNormal)) {
		return std::nullopt;
	}

	const float kDenom = quader::math::dot(kDirection, kNormal);
	if (std::abs(kDenom) <= kToolEpsilon) {
		return std::nullopt;
	}

	const float kT = quader::math::dot(point - ray.origin, kNormal) / kDenom;
	if (kT < 0.0F || !quader::math::is_finite(kT)) {
		return std::nullopt;
	}
	return ray.origin + scale3(kDirection, kT);
}

[[nodiscard]] std::optional<float> axis_distance_from_ray(
		quader::math::Vec3 axis,
		quader::math::Vec3 center,
		const ViewportRay &ray) noexcept {
	const quader::math::Vec3 kAxis = normalized_or(axis, {});
	const quader::math::Vec3 kDirection = normalized_or(ray.direction, {});
	if (tiny(kAxis) || tiny(kDirection)) {
		return std::nullopt;
	}

	const quader::math::Vec3 kDelta = center - ray.origin;
	const float kB = quader::math::dot(kAxis, kDirection);
	const float kD = quader::math::dot(kAxis, kDelta);
	const float kE = quader::math::dot(kDirection, kDelta);
	const float kDenom = 1.0F - kB * kB;
	if (std::abs(kDenom) <= kScreenEpsilon) {
		return std::nullopt;
	}

	const float kAxisDistance = (kB * kE - kD) / kDenom;
	const float kRayDistance = (kE - kB * kD) / kDenom;
	if (!quader::math::is_finite(kAxisDistance) || !quader::math::is_finite(kRayDistance) ||
			kRayDistance < 0.0F) {
		return std::nullopt;
	}
	return kAxisDistance;
}

[[nodiscard]] float distance_squared_to_segment_2d(
		quader::math::Vec2 point,
		quader::math::Vec2 start,
		quader::math::Vec2 end) noexcept {
	const quader::math::Vec2 kSegment = end - start;
	const float kLengthSquared = quader::math::length_squared(kSegment);
	if (kLengthSquared <= kScreenEpsilon) {
		return quader::math::length_squared(point - start);
	}

	const float kT = std::clamp(quader::math::dot(point - start, kSegment) / kLengthSquared, 0.0F, 1.0F);
	const quader::math::Vec2 kClosest = start + scale2(kSegment, kT);
	return quader::math::length_squared(point - kClosest);
}

[[nodiscard]] float signed_area_2d(const std::array<quader::math::Vec2, 4> &points) noexcept {
	float area = 0.0F;
	for (std::size_t index = 0; index < points.size(); ++index) {
		const quader::math::Vec2 a = points[index];
		const quader::math::Vec2 b = points[(index + 1U) % points.size()];
		area += a.x * b.y - b.x * a.y;
	}
	return area * 0.5F;
}

[[nodiscard]] bool point_in_convex_polygon(
		quader::math::Vec2 point,
		const std::array<quader::math::Vec2, 4> &polygon) noexcept {
	float sign = 0.0F;
	for (std::size_t index = 0; index < polygon.size(); ++index) {
		const quader::math::Vec2 a = polygon[index];
		const quader::math::Vec2 b = polygon[(index + 1U) % polygon.size()];
		const quader::math::Vec2 edge = b - a;
		const quader::math::Vec2 to_point = point - a;
		const float cross = edge.x * to_point.y - edge.y * to_point.x;
		if (std::abs(cross) <= kScreenEpsilon) {
			continue;
		}
		if (sign == 0.0F) {
			sign = cross;
			continue;
		}
		if ((cross > 0.0F) != (sign > 0.0F)) {
			return false;
		}
	}
	return true;
}

[[nodiscard]] GizmoFrame build_gizmo_frame(
		TransformGizmoMode mode,
		quader::math::Vec3 pivot,
		const ViewportCameraInput &camera) {
	GizmoFrame frame;
	frame.pivot = pivot;
	frame.camera = camera;
	frame.basis = make_camera_basis(camera);
	const auto kProjectedPivot = project_world_to_viewport(camera, pivot);
	if (!kProjectedPivot.has_value()) {
		return frame;
	}
	frame.screen_pivot = kProjectedPivot->position;
	frame.pixel_world = std::max(kToolEpsilon, world_units_per_pixel(camera, pivot));
	frame.unit_screen_size = kGizmoSizePixels;
	frame.width_scale = 1.0F;
	frame.ok = true;

	if (mode == TransformGizmoMode::Move || mode == TransformGizmoMode::Scale) {
		const float kAxisTip = mode == TransformGizmoMode::Scale ? kGizmoScaleTip : kGizmoArrowTip;
		const float kAxisLength = frame.pixel_world * frame.unit_screen_size * kAxisTip;
		for (const TransformGizmoAxis axis : principal_axes()) {
			const quader::math::Vec3 kTip = pivot + scale3(axis_vector(axis), kAxisLength);
			const auto kProjectedTip = project_world_to_viewport(camera, kTip);
			if (!kProjectedTip.has_value() ||
					quader::math::length_squared(kProjectedTip->position - frame.screen_pivot) <= kScreenEpsilon) {
				continue;
			}
			frame.axes.push_back(GizmoAxisFrame{
					.axis = axis,
					.world_start = pivot,
					.world_tip = kTip,
					.screen_start = frame.screen_pivot,
					.screen_tip = kProjectedTip->position,
			});
		}

		const float kPlaneOffset = frame.pixel_world * frame.unit_screen_size * (kGizmoPlaneDistance + kGizmoPlaneSize * 0.5F);
		const float kPlaneHalfSize = frame.pixel_world * frame.unit_screen_size * (kGizmoPlaneSize * 0.5F);
		for (const TransformGizmoAxis plane : plane_axes()) {
			const auto kComponents = components_for_plane(plane);
			const quader::math::Vec3 a = axis_vector(kComponents[0]);
			const quader::math::Vec3 b = axis_vector(kComponents[1]);
			const quader::math::Vec3 center = pivot + scale3(a, kPlaneOffset) + scale3(b, kPlaneOffset);
			GizmoPlaneFrame plane_frame;
			plane_frame.axis = plane;
			plane_frame.world = {
				center - scale3(a, kPlaneHalfSize) - scale3(b, kPlaneHalfSize),
				center + scale3(a, kPlaneHalfSize) - scale3(b, kPlaneHalfSize),
				center + scale3(a, kPlaneHalfSize) + scale3(b, kPlaneHalfSize),
				center - scale3(a, kPlaneHalfSize) + scale3(b, kPlaneHalfSize),
			};

			bool projected = true;
			for (std::size_t index = 0; index < plane_frame.world.size(); ++index) {
				const auto kProjected = project_world_to_viewport(camera, plane_frame.world[index]);
				if (!kProjected.has_value()) {
					projected = false;
					break;
				}
				plane_frame.screen[index] = kProjected->position;
			}
			if (projected && std::abs(signed_area_2d(plane_frame.screen)) > 2.0F) {
				frame.planes.push_back(plane_frame);
			}
		}
		return frame;
	}

	if (mode == TransformGizmoMode::Rotate) {
		constexpr int kRingSegments = 64;
		const float kRingRadius = frame.pixel_world * frame.unit_screen_size * kGizmoCircleSize;
		const quader::math::Vec3 kViewDirection = normalized_or(camera.eye - pivot, { 0.0F, 0.0F, 1.0F });
		for (const TransformGizmoAxis axis : principal_axes()) {
			const auto kComponents = components_for_plane(axis == TransformGizmoAxis::X ? TransformGizmoAxis::Yz
					: axis == TransformGizmoAxis::Y ? TransformGizmoAxis::Xz
													 : TransformGizmoAxis::Xy);
			const quader::math::Vec3 a = axis_vector(kComponents[0]);
			const quader::math::Vec3 b = axis_vector(kComponents[1]);
			for (int index = 0; index < kRingSegments; ++index) {
				const float kA0 = (static_cast<float>(index) / static_cast<float>(kRingSegments)) * kTau;
				const float kA1 = (static_cast<float>(index + 1) / static_cast<float>(kRingSegments)) * kTau;
				const quader::math::Vec3 kStart = pivot + scale3(a, std::cos(kA0) * kRingRadius) + scale3(b, std::sin(kA0) * kRingRadius);
				const quader::math::Vec3 kEnd = pivot + scale3(a, std::cos(kA1) * kRingRadius) + scale3(b, std::sin(kA1) * kRingRadius);
				const auto kProjectedStart = project_world_to_viewport(camera, kStart);
				const auto kProjectedEnd = project_world_to_viewport(camera, kEnd);
				if (!kProjectedStart.has_value() || !kProjectedEnd.has_value()) {
					continue;
				}
				const quader::math::Vec3 kMid = scale3(kStart + kEnd, 0.5F);
				const bool kFrontFacing = quader::math::dot(normalized_or(kMid - pivot, {}), kViewDirection) >= -0.01F;
				frame.rings.push_back(GizmoRingSegmentFrame{
						.axis = axis,
						.world_start = kStart,
						.world_end = kEnd,
						.screen_start = kProjectedStart->position,
						.screen_end = kProjectedEnd->position,
						.front_facing = kFrontFacing,
				});
			}
		}

		const float kTrackballRadius = kRingRadius * kGizmoViewRotationScale;
		for (int index = 0; index < kRingSegments; ++index) {
			const float kA0 = (static_cast<float>(index) / static_cast<float>(kRingSegments)) * kTau;
			const float kA1 = (static_cast<float>(index + 1) / static_cast<float>(kRingSegments)) * kTau;
			const quader::math::Vec3 kStart = pivot + scale3(frame.basis.right, std::cos(kA0) * kTrackballRadius) +
					scale3(frame.basis.up, std::sin(kA0) * kTrackballRadius);
			const quader::math::Vec3 kEnd = pivot + scale3(frame.basis.right, std::cos(kA1) * kTrackballRadius) +
					scale3(frame.basis.up, std::sin(kA1) * kTrackballRadius);
			const auto kProjectedStart = project_world_to_viewport(camera, kStart);
			const auto kProjectedEnd = project_world_to_viewport(camera, kEnd);
			if (!kProjectedStart.has_value() || !kProjectedEnd.has_value()) {
				continue;
			}
			frame.trackball.push_back(GizmoRingSegmentFrame{
					.axis = TransformGizmoAxis::All,
					.world_start = kStart,
					.world_end = kEnd,
					.screen_start = kProjectedStart->position,
					.screen_end = kProjectedEnd->position,
					.front_facing = true,
			});
		}
	}

	return frame;
}

void append_line(ToolPreview &preview,
		quader::math::Vec3 start,
		quader::math::Vec3 end,
		ToolPreviewColorSrgb color,
		float thickness_px) {
	preview.colored_world_segments.push_back(ToolPreviewColoredWorldSegment{
			.start = start,
			.end = end,
			.color_srgb = color,
			.thickness_px = thickness_px,
	});
}

void append_triangle(ToolPreview &preview,
		quader::math::Vec3 a,
		quader::math::Vec3 b,
		quader::math::Vec3 c,
		ToolPreviewColorSrgb color) {
	preview.colored_world_triangles.push_back(ToolPreviewColoredWorldTriangle{
			.a = a,
			.b = b,
			.c = c,
			.color_srgb = color,
	});
}

[[nodiscard]] quader::math::Vec3 fallback_perpendicular(quader::math::Vec3 direction) noexcept {
	quader::math::Vec3 perpendicular = quader::math::cross(direction, { 0.0F, 1.0F, 0.0F });
	if (tiny(perpendicular)) {
		perpendicular = quader::math::cross(direction, { 0.0F, 0.0F, 1.0F });
	}
	return normalized_or(perpendicular, { 1.0F, 0.0F, 0.0F });
}

void append_plane(ToolPreview &preview,
		const GizmoPlaneFrame &plane,
		ToolPreviewColorSrgb color) {
	append_triangle(preview, plane.world[0], plane.world[1], plane.world[2], color);
	append_triangle(preview, plane.world[0], plane.world[2], plane.world[3], color);
	for (std::size_t index = 0U; index < plane.world.size(); ++index) {
		append_line(preview, plane.world[index], plane.world[(index + 1U) % plane.world.size()], color, kGizmoPlaneLineWidthPixels);
	}
}

void append_arrow_handle(ToolPreview &preview,
		quader::math::Vec3 start,
		quader::math::Vec3 tip,
		ToolPreviewColorSrgb color,
		float line_width_px) {
	quader::math::Vec3 direction = tip - start;
	const float kLength = quader::math::length(direction);
	if (kLength <= kToolEpsilon) {
		return;
	}
	direction = direction / kLength;
	const float kUnitWorld = kLength / kGizmoArrowTip;
	const quader::math::Vec3 kBaseCenter = start + scale3(direction, kUnitWorld * kGizmoArrowOffset);
	append_line(preview, start, kBaseCenter, color, line_width_px);

	const quader::math::Vec3 kPerp = fallback_perpendicular(direction);
	const quader::math::Vec3 kBinormal = normalized_or(quader::math::cross(direction, kPerp), { 0.0F, 1.0F, 0.0F });
	const float kRadius = kUnitWorld * kGizmoArrowConeRadius;
	constexpr int kSegments = 16;
	for (int index = 0; index < kSegments; ++index) {
		const float kA0 = (static_cast<float>(index) / static_cast<float>(kSegments)) * kTau;
		const float kA1 = (static_cast<float>(index + 1) / static_cast<float>(kSegments)) * kTau;
		const quader::math::Vec3 a = kBaseCenter + scale3(kPerp, std::cos(kA0) * kRadius) + scale3(kBinormal, std::sin(kA0) * kRadius);
		const quader::math::Vec3 b = kBaseCenter + scale3(kPerp, std::cos(kA1) * kRadius) + scale3(kBinormal, std::sin(kA1) * kRadius);
		append_triangle(preview, tip, a, b, color);
		append_triangle(preview, kBaseCenter, b, a, color);
	}
}

void append_square_prism(ToolPreview &preview,
		quader::math::Vec3 start,
		quader::math::Vec3 end,
		float half_width,
		ToolPreviewColorSrgb color) {
	const quader::math::Vec3 kDirection = normalized_or(end - start, { 1.0F, 0.0F, 0.0F });
	const quader::math::Vec3 kU = fallback_perpendicular(kDirection);
	const quader::math::Vec3 kV = normalized_or(quader::math::cross(kDirection, kU), { 0.0F, 1.0F, 0.0F });
	const std::array<quader::math::Vec3, 4> start_corners{
		start + scale3(kU, -half_width) + scale3(kV, -half_width),
		start + scale3(kU, half_width) + scale3(kV, -half_width),
		start + scale3(kU, half_width) + scale3(kV, half_width),
		start + scale3(kU, -half_width) + scale3(kV, half_width),
	};
	const std::array<quader::math::Vec3, 4> end_corners{
		end + scale3(kU, -half_width) + scale3(kV, -half_width),
		end + scale3(kU, half_width) + scale3(kV, -half_width),
		end + scale3(kU, half_width) + scale3(kV, half_width),
		end + scale3(kU, -half_width) + scale3(kV, half_width),
	};
	for (std::size_t index = 0U; index < 4U; ++index) {
		const std::size_t next = (index + 1U) % 4U;
		append_triangle(preview, start_corners[index], end_corners[index], end_corners[next], color);
		append_triangle(preview, start_corners[index], end_corners[next], start_corners[next], color);
	}
	append_triangle(preview, start_corners[0], start_corners[2], start_corners[1], color);
	append_triangle(preview, start_corners[0], start_corners[3], start_corners[2], color);
	append_triangle(preview, end_corners[0], end_corners[1], end_corners[2], color);
	append_triangle(preview, end_corners[0], end_corners[2], end_corners[3], color);
}

void append_scale_handle(ToolPreview &preview,
		quader::math::Vec3 start,
		quader::math::Vec3 tip,
		ToolPreviewColorSrgb color,
		float line_width_px) {
	quader::math::Vec3 direction = tip - start;
	const float kLength = quader::math::length(direction);
	if (kLength <= kToolEpsilon) {
		return;
	}
	direction = direction / kLength;
	const float kUnitWorld = kLength / kGizmoScaleTip;
	const quader::math::Vec3 kBlockStart = start + scale3(direction, kUnitWorld * kGizmoScaleOffset);
	append_line(preview, start, kBlockStart, color, line_width_px);
	append_square_prism(preview, kBlockStart, tip, kUnitWorld * kGizmoScaleHalfWidth, color);
}

void append_cube_handle(ToolPreview &preview,
		const ViewportCameraInput &camera,
		quader::math::Vec3 center,
		float world_size,
		ToolPreviewColorSrgb color,
		bool hovered) {
	if (world_size <= kToolEpsilon) {
		return;
	}

	const float kHalf = world_size * 0.5F;
	const std::array<quader::math::Vec3, 8> corners{
		center + quader::math::Vec3{ -kHalf, -kHalf, -kHalf },
		center + quader::math::Vec3{ kHalf, -kHalf, -kHalf },
		center + quader::math::Vec3{ kHalf, kHalf, -kHalf },
		center + quader::math::Vec3{ -kHalf, kHalf, -kHalf },
		center + quader::math::Vec3{ -kHalf, -kHalf, kHalf },
		center + quader::math::Vec3{ kHalf, -kHalf, kHalf },
		center + quader::math::Vec3{ kHalf, kHalf, kHalf },
		center + quader::math::Vec3{ -kHalf, kHalf, kHalf },
	};
	constexpr std::array<std::array<int, 4>, 6> kFaces{
		std::array<int, 4>{ 1, 5, 6, 2 },
		std::array<int, 4>{ 4, 0, 3, 7 },
		std::array<int, 4>{ 3, 2, 6, 7 },
		std::array<int, 4>{ 4, 5, 1, 0 },
		std::array<int, 4>{ 5, 4, 7, 6 },
		std::array<int, 4>{ 0, 1, 2, 3 },
	};
	constexpr std::array<quader::math::Vec3, 6> kNormals{
		quader::math::Vec3{ 1.0F, 0.0F, 0.0F },
		quader::math::Vec3{ -1.0F, 0.0F, 0.0F },
		quader::math::Vec3{ 0.0F, 1.0F, 0.0F },
		quader::math::Vec3{ 0.0F, -1.0F, 0.0F },
		quader::math::Vec3{ 0.0F, 0.0F, 1.0F },
		quader::math::Vec3{ 0.0F, 0.0F, -1.0F },
	};
	const quader::math::Vec3 kViewDirection = normalized_or(camera.eye - center, { 0.0F, 0.0F, 1.0F });
	const quader::math::Vec3 kLightDirection = normalized_or({ -0.45F, -0.65F, 0.62F }, { 0.0F, 0.0F, 1.0F });
	for (std::size_t face_index = 0U; face_index < kFaces.size(); ++face_index) {
		const float kFacing = quader::math::dot(kNormals[face_index], kViewDirection);
		if (kFacing <= kToolEpsilon) {
			continue;
		}
		const float kLit = std::max(quader::math::dot(kNormals[face_index], kLightDirection), 0.0F);
		const float kShade = std::clamp(0.46F + kLit * 0.36F + std::min(kFacing, 1.0F) * 0.18F, 0.42F, 1.12F);
		const ToolPreviewColorSrgb kFillColor = multiply_color(color, hovered ? kShade * 1.15F : kShade);
		const auto &face = kFaces[face_index];
		append_triangle(preview, corners[static_cast<std::size_t>(face[0])], corners[static_cast<std::size_t>(face[1])], corners[static_cast<std::size_t>(face[2])], kFillColor);
		append_triangle(preview, corners[static_cast<std::size_t>(face[0])], corners[static_cast<std::size_t>(face[2])], corners[static_cast<std::size_t>(face[3])], kFillColor);
		if (hovered) {
			append_line(preview, corners[static_cast<std::size_t>(face[0])], corners[static_cast<std::size_t>(face[1])], kCubeOutlineColor, 1.0F);
			append_line(preview, corners[static_cast<std::size_t>(face[1])], corners[static_cast<std::size_t>(face[2])], kCubeOutlineColor, 1.0F);
			append_line(preview, corners[static_cast<std::size_t>(face[2])], corners[static_cast<std::size_t>(face[3])], kCubeOutlineColor, 1.0F);
			append_line(preview, corners[static_cast<std::size_t>(face[3])], corners[static_cast<std::size_t>(face[0])], kCubeOutlineColor, 1.0F);
		}
	}
}

[[nodiscard]] float first_axis_world_length(const GizmoFrame &frame) noexcept {
	if (frame.axes.empty()) {
		return frame.pixel_world * frame.unit_screen_size * kGizmoScaleTip;
	}
	return quader::math::length(frame.axes.front().world_tip - frame.axes.front().world_start);
}

void append_rotate_indicator(ToolPreview &preview,
		const TransformGizmoToolState &state,
		const GizmoFrame &frame,
		quader::math::Vec3 start_direction,
		float angle_degrees,
		bool active) {
	if (!active || state.active_handle.kind != TransformGizmoHandleKind::RotationRing ||
			!quader::math::is_finite(angle_degrees)) {
		return;
	}
	const quader::math::Vec3 kAxis = rotation_axis_vector(state.active_handle.axis);
	if (tiny(kAxis) || tiny(start_direction)) {
		return;
	}

	const float kRingRadius = frame.pixel_world * frame.unit_screen_size * kGizmoCircleSize;
	const float kPieRadius = kRingRadius * 0.98F;
	const float kAngleRadians = radians_from_degrees(angle_degrees);
	const int kArcSteps = std::clamp(static_cast<int>(std::abs(kAngleRadians) / (kPi / 24.0F)) + 2, 3, 72);
	quader::math::Vec3 previous = frame.pivot + scale3(start_direction, kPieRadius);
	if (std::abs(kAngleRadians) > 0.005F) {
		for (int step = 1; step <= kArcSteps; ++step) {
			const float t = static_cast<float>(step) / static_cast<float>(kArcSteps);
			const quader::math::Vec3 direction = rotate_around_axis(start_direction, kAxis, kAngleRadians * t);
			const quader::math::Vec3 point = frame.pivot + scale3(direction, kPieRadius);
			append_triangle(preview, frame.pivot, previous, point, kRotatePieFillColor);
			previous = point;
		}
	}
	const quader::math::Vec3 current_direction = normalized_or(rotate_around_axis(start_direction, kAxis, kAngleRadians), start_direction);
	const quader::math::Vec3 start_point = frame.pivot + scale3(start_direction, kPieRadius);
	const quader::math::Vec3 current_point = frame.pivot + scale3(current_direction, kRingRadius * 1.04F);
	append_line(preview, frame.pivot, start_point, kRotatePieLineColor, 1.25F);
	append_line(preview, frame.pivot, current_point, kRotatePieLineColor, 1.55F);
}

[[nodiscard]] ObjectBounds object_bounds_for_transform(
		const quader::document::MeshObject &object,
		quader::document::Transform transform) {
	ObjectBounds result;
	for (const quader::mesh::VertexId vertex : object.mesh.vertex_ids()) {
		auto position = object.mesh.vertex_position(vertex);
		if (!position) {
			continue;
		}
		quader::math::expand(result.bounds, transform_point(position.value(), transform));
		result.valid = true;
	}
	return result;
}

[[nodiscard]] ObjectBounds local_bounds_for_object(
		const quader::document::MeshObject &object) {
	ObjectBounds result;
	for (const quader::mesh::VertexId vertex : object.mesh.vertex_ids()) {
		auto position = object.mesh.vertex_position(vertex);
		if (!position) {
			continue;
		}
		quader::math::expand(result.bounds, position.value());
		result.valid = true;
	}
	return result;
}

[[nodiscard]] ObjectBounds transformed_bounds_from_local_bounds(
		quader::math::Vec3 min,
		quader::math::Vec3 max,
		quader::document::Transform transform) {
	const std::array<quader::math::Vec3, 8> corners{
		quader::math::Vec3{ min.x, min.y, min.z },
		quader::math::Vec3{ max.x, min.y, min.z },
		quader::math::Vec3{ max.x, max.y, min.z },
		quader::math::Vec3{ min.x, max.y, min.z },
		quader::math::Vec3{ min.x, min.y, max.z },
		quader::math::Vec3{ max.x, min.y, max.z },
		quader::math::Vec3{ max.x, max.y, max.z },
		quader::math::Vec3{ min.x, max.y, max.z },
	};

	ObjectBounds result;
	for (const quader::math::Vec3 corner : corners) {
		quader::math::expand(result.bounds, transform_point(corner, transform));
		result.valid = true;
	}
	return result;
}

[[nodiscard]] float bounds_extent_component(quader::math::Aabb bounds,
		TransformGizmoAxis component) noexcept {
	switch (component) {
		case TransformGizmoAxis::X:
			return std::abs(bounds.max.x - bounds.min.x);
		case TransformGizmoAxis::Y:
			return std::abs(bounds.max.y - bounds.min.y);
		case TransformGizmoAxis::Z:
			return std::abs(bounds.max.z - bounds.min.z);
		case TransformGizmoAxis::Xy:
		case TransformGizmoAxis::Xz:
		case TransformGizmoAxis::Yz:
		case TransformGizmoAxis::All:
		case TransformGizmoAxis::None:
			break;
	}
	return 0.0F;
}

[[nodiscard]] float base_scale_distance(
		TransformGizmoAxis axis,
		quader::math::Aabb bounds,
		quader::math::Vec3 pivot) noexcept {
	const auto max_abs = [](float left, float right) {
		return std::max(std::abs(left), std::abs(right));
	};
	switch (axis) {
		case TransformGizmoAxis::X:
			return max_abs(bounds.max.x - pivot.x, bounds.min.x - pivot.x);
		case TransformGizmoAxis::Y:
			return max_abs(bounds.max.y - pivot.y, bounds.min.y - pivot.y);
		case TransformGizmoAxis::Z:
			return max_abs(bounds.max.z - pivot.z, bounds.min.z - pivot.z);
		case TransformGizmoAxis::Xy:
			return std::max(max_abs(bounds.max.x - pivot.x, bounds.min.x - pivot.x),
					max_abs(bounds.max.y - pivot.y, bounds.min.y - pivot.y));
		case TransformGizmoAxis::Xz:
			return std::max(max_abs(bounds.max.x - pivot.x, bounds.min.x - pivot.x),
					max_abs(bounds.max.z - pivot.z, bounds.min.z - pivot.z));
		case TransformGizmoAxis::Yz:
			return std::max(max_abs(bounds.max.y - pivot.y, bounds.min.y - pivot.y),
					max_abs(bounds.max.z - pivot.z, bounds.min.z - pivot.z));
		case TransformGizmoAxis::All:
			return std::max(std::max(max_abs(bounds.max.x - pivot.x, bounds.min.x - pivot.x),
									max_abs(bounds.max.y - pivot.y, bounds.min.y - pivot.y)),
					max_abs(bounds.max.z - pivot.z, bounds.min.z - pivot.z));
		case TransformGizmoAxis::None:
			break;
	}
	return 0.0F;
}

[[nodiscard]] float minimum_scale_factor_for_grid(
		TransformGizmoAxis axis,
		quader::math::Aabb bounds,
		float grid_size,
		bool shrinking) noexcept {
	float factor = kMinScaleFactor;
	for (const TransformGizmoAxis component : principal_axes()) {
		if (!axis_includes_component(axis, component)) {
			continue;
		}
		const float kExtent = bounds_extent_component(bounds, component);
		if (kExtent <= kScreenEpsilon) {
			continue;
		}
		float axis_factor = grid_size / kExtent;
		if (shrinking && axis_factor > 1.0F) {
			axis_factor = 1.0F;
		}
		factor = std::max(factor, axis_factor);
	}
	return factor;
}

[[nodiscard]] float snapped_scale_factor(
		TransformGizmoAxis axis,
		float factor,
		const CombinedBounds &bounds,
		quader::math::Vec3 pivot,
		bool grid_enabled,
		float grid_size) noexcept {
	if (!bounds.valid || !grid_enabled || grid_size <= kScreenEpsilon) {
		return factor;
	}
	if (std::abs(factor - 1.0F) <= kToolEpsilon) {
		return 1.0F;
	}
	const float kBaseDistance = base_scale_distance(axis, bounds.bounds, pivot);
	if (kBaseDistance <= kScreenEpsilon) {
		return factor;
	}
	const float kBaseExtent = kBaseDistance * 2.0F;
	const float kRawExtent = std::max(kToolEpsilon, kBaseExtent * factor);
	const float kGrid = grid_or_default(grid_size);
	const bool kShrinking = factor < 1.0F;
	const float kSnappedUnits = kShrinking ? std::floor(kRawExtent / kGrid) : std::round(kRawExtent / kGrid);
	const float kSnappedExtent = kSnappedUnits * kGrid;
	const float kSnappedFactor = kSnappedExtent <= kScreenEpsilon ? kMinScaleFactor : kSnappedExtent / kBaseExtent;
	const float kMinimumFactor = minimum_scale_factor_for_grid(axis, bounds.bounds, kGrid, kShrinking);
	return std::max(kMinimumFactor, kSnappedFactor);
}

[[nodiscard]] quader::math::Vec3 axis_scale(TransformGizmoAxis axis,
		float factor) noexcept {
	quader::math::Vec3 scale{ 1.0F, 1.0F, 1.0F };
	switch (axis) {
		case TransformGizmoAxis::X:
			scale.x = factor;
			break;
		case TransformGizmoAxis::Y:
			scale.y = factor;
			break;
		case TransformGizmoAxis::Z:
			scale.z = factor;
			break;
		case TransformGizmoAxis::Xy:
			scale.x = factor;
			scale.y = factor;
			break;
		case TransformGizmoAxis::Xz:
			scale.x = factor;
			scale.z = factor;
			break;
		case TransformGizmoAxis::Yz:
			scale.y = factor;
			scale.z = factor;
			break;
		case TransformGizmoAxis::All:
			scale = { factor, factor, factor };
			break;
		case TransformGizmoAxis::None:
			break;
	}
	return scale;
}

} // namespace

TransformGizmoTool::TransformGizmoTool(ToolId id) noexcept : id_(id), mode_(mode_from_tool_id(id)) {
	if (id_ != ToolId::Move && id_ != ToolId::Rotate && id_ != ToolId::Scale) {
		id_ = ToolId::Move;
		mode_ = TransformGizmoMode::Move;
	}
}

ToolId TransformGizmoTool::id() const noexcept {
	return id_;
}

void TransformGizmoTool::activate(ToolContext &context) {
	(void)context;
	reset();
}

void TransformGizmoTool::deactivate(ToolContext &context) {
	if (state_.dragging) {
		(void)restore_original_transforms(context);
	}
	reset();
}

void TransformGizmoTool::cancel(ToolContext &context) {
	if (state_.dragging) {
		(void)restore_original_transforms(context);
	}
	reset();
}

bool TransformGizmoTool::on_pointer_event(const PointerEvent &event, ToolContext &context) {
	if (event.navigation_active) {
		if (state_.dragging) {
			cancel_drag(context);
			rebuild_preview();
		}
		return false;
	}

	if (state_.dragging) {
		if (event.phase == PointerPhase::Move || event.phase == PointerPhase::Hover) {
			return update_drag(event, context);
		}
		if (event.phase == PointerPhase::Release && event.button == PointerButton::Left) {
			(void)update_drag(event, context);
			(void)commit_drag(context);
			clear_drag_state();
			(void)refresh_targets(context, event);
			rebuild_preview();
			return true;
		}
		return false;
	}

	if (event.phase == PointerPhase::Press && event.button == PointerButton::Left && event.pressed) {
		return begin_drag(event, context);
	}

	if (event.phase == PointerPhase::Hover || event.phase == PointerPhase::Move) {
		if (!refresh_targets(context, event)) {
			preview_.clear();
			return false;
		}

		const std::optional<TransformGizmoHandle> kHit = pick_handle(event);
		state_.hover_handle = kHit.value_or(TransformGizmoHandle{});
		rebuild_preview();
		return true;
	}

	return false;
}

bool TransformGizmoTool::on_key_event(const KeyEvent &event, ToolContext &context) {
	if (!event.pressed || event.auto_repeat || event.key_code != 27) {
		return false;
	}

	const bool kHadPreview = !preview_.empty() || state_.dragging;
	if (state_.dragging) {
		(void)restore_original_transforms(context);
	}
	reset();
	return kHadPreview;
}

ToolPreview TransformGizmoTool::preview() const {
	return preview_;
}

TransformGizmoMode TransformGizmoTool::mode() const noexcept {
	return mode_;
}

const TransformGizmoToolState &TransformGizmoTool::state() const noexcept {
	return state_;
}

bool TransformGizmoTool::refresh_targets(const ToolContext &context, const PointerEvent &event) {
	const auto &document = context.document();
	targets_.clear();
	preview_transforms_.clear();

	for (const quader::document::ObjectId object_id : document.selection().selected_objects()) {
		if (!object_id.is_valid()) {
			continue;
		}
		if (std::find_if(targets_.begin(), targets_.end(), [object_id](const TargetSnapshot &target) {
				return target.object == object_id;
			}) != targets_.end()) {
			continue;
		}

		const auto *object = document.find_mesh_object(object_id);
		if (object == nullptr) {
			continue;
		}

		const ObjectBounds kLocalBounds = local_bounds_for_object(*object);
		targets_.push_back(TargetSnapshot{
				.object = object_id,
				.original = object->transform,
				.local_min = kLocalBounds.bounds.min,
				.local_max = kLocalBounds.bounds.max,
				.has_local_bounds = kLocalBounds.valid,
		});
	}

	state_.target_count = targets_.size();
	state_.view_index = event.view_index;
	last_camera_ = camera_for_event(event);
	if (targets_.empty()) {
		state_.pivot = {};
		state_.gizmo_scale = 1.0F;
		return false;
	}

	quader::math::Aabb combined_bounds;
	bool has_bounds = false;
	quader::math::Vec3 translation_average;
	for (const TargetSnapshot &target : targets_) {
		const auto *object = document.find_mesh_object(target.object);
		if (object == nullptr) {
			continue;
		}
		translation_average = translation_average + object->transform.translation;

		const ObjectBounds kBounds = object_bounds_for_transform(*object, object->transform);
		if (!kBounds.valid) {
			continue;
		}
		quader::math::expand(combined_bounds, kBounds.bounds.min);
		quader::math::expand(combined_bounds, kBounds.bounds.max);
		has_bounds = true;
	}

	state_.pivot = has_bounds ? quader::math::center(combined_bounds)
							  : translation_average / static_cast<float>(targets_.size());
	if (last_camera_.has_value()) {
		state_.gizmo_scale = world_units_per_pixel(*last_camera_, state_.pivot) * kGizmoSizePixels;
	}
	return true;
}

std::optional<TransformGizmoHandle> TransformGizmoTool::pick_handle(
		const PointerEvent &event) const {
	if (state_.target_count == 0U) {
		return std::nullopt;
	}
	const std::optional<ViewportCameraInput> kCamera = camera_for_event(event);
	if (!kCamera.has_value()) {
		return std::nullopt;
	}
	const GizmoFrame kFrame = build_gizmo_frame(mode_, state_.pivot, *kCamera);
	if (!kFrame.ok) {
		return std::nullopt;
	}
	const float kPickRadius = std::max(8.0F, kGizmoPickRadiusPixels);
	const float kPickRadiusSquared = kPickRadius * kPickRadius;

	if (mode_ == TransformGizmoMode::Scale) {
		const float kCenterSafeRadius = std::max(kPickRadius,
				std::min(std::max(24.0F, kFrame.unit_screen_size) * 0.35F,
						std::max(kPickRadius * 1.75F, 12.0F)));
		if (quader::math::length_squared(event.position - kFrame.screen_pivot) <=
				kCenterSafeRadius * kCenterSafeRadius) {
			return TransformGizmoHandle{ .kind = TransformGizmoHandleKind::Center, .axis = TransformGizmoAxis::All };
		}
	}

	if (mode_ == TransformGizmoMode::Rotate) {
		float best_distance = kPickRadiusSquared;
		std::optional<TransformGizmoAxis> best_axis;
		for (const GizmoRingSegmentFrame &segment : kFrame.rings) {
			const float kDistance = distance_squared_to_segment_2d(event.position, segment.screen_start, segment.screen_end);
			if (kDistance < best_distance) {
				best_distance = kDistance;
				best_axis = segment.axis;
			}
		}
		if (best_axis.has_value()) {
			return TransformGizmoHandle{ .kind = TransformGizmoHandleKind::RotationRing, .axis = *best_axis };
		}
		return std::nullopt;
	}

	float best_axis_tip_distance = kPickRadiusSquared;
	std::optional<TransformGizmoAxis> best_axis_tip;
	for (const GizmoAxisFrame &axis : kFrame.axes) {
		const float kDistance = quader::math::length_squared(event.position - axis.screen_tip);
		if (kDistance <= best_axis_tip_distance) {
			best_axis_tip_distance = kDistance;
			best_axis_tip = axis.axis;
		}
	}
	if (best_axis_tip.has_value()) {
		return TransformGizmoHandle{ .kind = TransformGizmoHandleKind::Axis, .axis = *best_axis_tip };
	}

	float best_axis_distance = kPickRadiusSquared;
	std::optional<TransformGizmoAxis> best_axis;
	for (const GizmoAxisFrame &axis : kFrame.axes) {
		const float kDistance = distance_squared_to_segment_2d(event.position, kFrame.screen_pivot, axis.screen_tip);
		if (kDistance <= best_axis_distance) {
			best_axis_distance = kDistance;
			best_axis = axis.axis;
		}
	}
	if (best_axis.has_value()) {
		return TransformGizmoHandle{ .kind = TransformGizmoHandleKind::Axis, .axis = *best_axis };
	}

	for (const GizmoPlaneFrame &plane : kFrame.planes) {
		if (!point_in_convex_polygon(event.position, plane.screen)) {
			continue;
		}
		return TransformGizmoHandle{ .kind = TransformGizmoHandleKind::Plane, .axis = plane.axis };
	}

	return std::nullopt;
}

bool TransformGizmoTool::begin_drag(const PointerEvent &event, ToolContext &context) {
	if (!event.ray.has_value() || !refresh_targets(context, event) || !last_camera_.has_value()) {
		return false;
	}

	const std::optional<TransformGizmoHandle> kHit = pick_handle(event);
	if (!kHit.has_value()) {
		state_.hover_handle = {};
		rebuild_preview();
		return false;
	}

	const GizmoFrame kFrame = build_gizmo_frame(mode_, state_.pivot, *last_camera_);
	if (!kFrame.ok) {
		return false;
	}

	state_.dragging = true;
	state_.active_handle = *kHit;
	state_.hover_handle = *kHit;
	drag_start_ray_ = *event.ray;
	drag_start_screen_ = event.position;
	drag_start_pivot_ = state_.pivot;
	drag_camera_ = last_camera_;
	drag_start_scale_ = std::max(kToolEpsilon, state_.gizmo_scale);
	drag_screen_axis_ = { 1.0F, 0.0F };
	drag_has_screen_axis_ = false;
	drag_axis_start_distance_ = 0.0F;
	drag_has_axis_start_distance_ = false;
	drag_rotate_last_angle_radians_ = 0.0F;
	drag_rotate_accumulated_degrees_ = 0.0F;
	drag_rotate_view_sign_ = 1.0F;
	drag_rotate_edge_on_ = false;
	drag_rotate_edge_axis_ = { 1.0F, 0.0F };
	drag_rotate_edge_radius_pixels_ = 1.0F;
	drag_rotate_start_direction_ = {};
	drag_rotate_current_position_ = {};
	drag_rotate_indicator_ = false;

	const TransformGizmoAxis kAxis = axis_from_handle(*kHit);
	if (mode_ == TransformGizmoMode::Scale && kHit->kind == TransformGizmoHandleKind::Center) {
		drag_screen_axis_ = { 1.0F, 0.0F };
		drag_has_screen_axis_ = true;
	} else if (kHit->kind == TransformGizmoHandleKind::Axis || kHit->kind == TransformGizmoHandleKind::Plane) {
		for (const GizmoAxisFrame &axis : kFrame.axes) {
			if (axis.axis == kAxis) {
				drag_screen_axis_ = normalized2_or(axis.screen_tip - kFrame.screen_pivot, { 1.0F, 0.0F });
				drag_has_screen_axis_ = true;
				break;
			}
		}
		if (!drag_has_screen_axis_ && is_plane_axis(kAxis)) {
			for (const GizmoPlaneFrame &plane : kFrame.planes) {
				if (plane.axis == kAxis) {
					const quader::math::Vec2 kCentroid = scale2(plane.screen[0] + plane.screen[1] + plane.screen[2] + plane.screen[3], 0.25F);
					drag_screen_axis_ = normalized2_or(kCentroid - kFrame.screen_pivot, { 1.0F, 0.0F });
					drag_has_screen_axis_ = true;
					break;
				}
			}
		}
	}

	if ((mode_ == TransformGizmoMode::Move || mode_ == TransformGizmoMode::Scale) &&
			kHit->kind == TransformGizmoHandleKind::Axis) {
		const std::optional<float> kAxisDistance = axis_distance_from_ray(axis_vector(kHit->axis), state_.pivot, *event.ray);
		if (kAxisDistance.has_value()) {
			drag_axis_start_distance_ = *kAxisDistance;
			drag_has_axis_start_distance_ = true;
		}
	}

	if (mode_ == TransformGizmoMode::Rotate && kHit->kind == TransformGizmoHandleKind::RotationRing) {
		const quader::math::Vec2 kStartVector = event.position - kFrame.screen_pivot;
		drag_rotate_last_angle_radians_ = std::atan2(kStartVector.y, kStartVector.x);
		if (length2(kStartVector) <= kScreenEpsilon) {
			drag_rotate_last_angle_radians_ = 0.0F;
		}
		const quader::math::Vec3 kWorldAxis = rotation_axis_vector(kHit->axis);
		const quader::math::Vec3 kToCamera = last_camera_->eye - state_.pivot;
		if (!tiny(kWorldAxis) && !tiny(kToCamera)) {
			const quader::math::Vec3 kAxisNormal = normalized_or(kWorldAxis, {});
			const quader::math::Vec3 kViewToCamera = normalized_or(kToCamera, { 0.0F, 0.0F, 1.0F });
			drag_rotate_view_sign_ = quader::math::dot(kAxisNormal, kViewToCamera) >= 0.0F ? 1.0F : -1.0F;
			drag_rotate_edge_on_ = std::abs(quader::math::dot(kAxisNormal, kViewToCamera)) < 0.08715574274765817F;
			if (drag_rotate_edge_on_) {
				const quader::math::Vec3 kCameraPlaneNormal = scale3(kViewToCamera, -1.0F);
				const quader::math::Vec3 kProjectionAxis = normalized_or(quader::math::cross(kCameraPlaneNormal, kAxisNormal), { 1.0F, 0.0F, 0.0F });
				const CameraBasis kBasis = make_camera_basis(*last_camera_);
				const quader::math::Vec2 kProjectedScreenAxis{
					quader::math::dot(kProjectionAxis, normalized_or(kBasis.right, { 1.0F, 0.0F, 0.0F })),
					-quader::math::dot(kProjectionAxis, normalized_or(kBasis.up, { 0.0F, 1.0F, 0.0F })),
				};
				drag_rotate_edge_axis_ = normalized2_or(kProjectedScreenAxis, { 1.0F, 0.0F });
				drag_rotate_edge_radius_pixels_ = std::max(length2(kStartVector), 1.0F);
			}
		}
		const auto kStart = ray_plane_intersection(*event.ray, state_.pivot, kWorldAxis);
		drag_rotate_start_direction_ = kStart.has_value() ? normalized_or(*kStart - state_.pivot, axis_vector(TransformGizmoAxis::X))
														  : axis_vector(TransformGizmoAxis::X);
		drag_rotate_current_position_ = state_.pivot + drag_rotate_start_direction_;
		drag_rotate_indicator_ = true;
	}

	preview_transforms_.clear();
	preview_transforms_.reserve(targets_.size());
	for (const TargetSnapshot &target : targets_) {
		preview_transforms_.push_back(target.original);
	}
	status_text_.clear();
	rebuild_preview();
	return true;
}

bool TransformGizmoTool::update_drag(const PointerEvent &event, ToolContext &context) {
	if (!state_.dragging || !event.ray.has_value() || targets_.empty()) {
		return false;
	}
	const ViewportCameraInput kCamera = camera_for_event(event).value_or(drag_camera_.value_or(ViewportCameraInput{}));
	last_camera_ = kCamera;
	preview_transforms_.clear();
	preview_transforms_.reserve(targets_.size());

	CombinedBounds combined_bounds;
	for (const TargetSnapshot &target : targets_) {
		if (!target.has_local_bounds) {
			continue;
		}
		const ObjectBounds kBounds = transformed_bounds_from_local_bounds(target.local_min, target.local_max, target.original);
		if (!kBounds.valid) {
			continue;
		}
		quader::math::expand(combined_bounds.bounds, kBounds.bounds.min);
		quader::math::expand(combined_bounds.bounds, kBounds.bounds.max);
		combined_bounds.valid = true;
	}

	if (mode_ == TransformGizmoMode::Move) {
		quader::math::Vec3 delta;
		const TransformGizmoAxis kAxis = state_.active_handle.axis;
		if (state_.active_handle.kind == TransformGizmoHandleKind::Axis) {
			const quader::math::Vec3 kAxisVector = axis_vector(kAxis);
			const float kPixelDelta = quader::math::dot(event.position - drag_start_screen_, drag_screen_axis_);
			float raw_distance = kPixelDelta * world_units_per_pixel(kCamera, drag_start_pivot_);
			if (drag_has_axis_start_distance_) {
				const std::optional<float> kCurrentAxisDistance = axis_distance_from_ray(kAxisVector, drag_start_pivot_, *event.ray);
				if (kCurrentAxisDistance.has_value()) {
					const float kRayDelta = *kCurrentAxisDistance - drag_axis_start_distance_;
					if (std::abs(kRayDelta) > kScreenEpsilon || std::abs(kPixelDelta) <= kScreenEpsilon) {
						raw_distance = kRayDelta;
					}
				}
			}
			if (event.modifiers.control) {
				raw_distance *= 0.25F;
			}
			if (event.snap_to_grid) {
				const float kStart = axis_component(drag_start_pivot_, kAxis);
				raw_distance = snap_value(kStart + raw_distance, grid_or_default(event.grid_size)) - kStart;
			}
			delta = scale3(kAxisVector, raw_distance);
		} else if (state_.active_handle.kind == TransformGizmoHandleKind::Plane) {
			const quader::math::Vec3 kNormal = axis_vector(normal_for_plane(kAxis));
			const auto kStart = ray_plane_intersection(drag_start_ray_, drag_start_pivot_, kNormal);
			const auto kCurrent = ray_plane_intersection(*event.ray, drag_start_pivot_, kNormal);
			if (!kStart || !kCurrent) {
				return false;
			}
			delta = *kCurrent - *kStart;
			delta = delta - scale3(kNormal, quader::math::dot(delta, kNormal));
			if (event.modifiers.control) {
				delta = scale3(delta, 0.25F);
			}
			if (event.snap_to_grid) {
				quader::math::Vec3 snapped_delta;
				const float kGrid = grid_or_default(event.grid_size);
				for (const TransformGizmoAxis component : components_for_plane(kAxis)) {
					const float kStartComponent = axis_component(drag_start_pivot_, component);
					const float kRawComponent = axis_component(delta, component);
					set_axis_component(snapped_delta, component, snap_value(kStartComponent + kRawComponent, kGrid) - kStartComponent);
				}
				delta = snapped_delta;
			}
		} else {
			return false;
		}

		for (const TargetSnapshot &target : targets_) {
			quader::document::Transform transform = target.original;
			transform.translation = transform.translation + delta;
			preview_transforms_.push_back(transform);
		}
		state_.pivot = drag_start_pivot_ + delta;
		status_text_ = "Move gizmo";
		(void)apply_preview_transforms(context);
		rebuild_preview();
		return true;
	}

	if (mode_ == TransformGizmoMode::Scale) {
		TransformGizmoAxis scale_axis = state_.active_handle.axis;
		if (state_.active_handle.kind == TransformGizmoHandleKind::Center) {
			scale_axis = TransformGizmoAxis::All;
		} else if (state_.active_handle.kind != TransformGizmoHandleKind::Axis &&
				state_.active_handle.kind != TransformGizmoHandleKind::Plane) {
			return false;
		}

		float axis_pixels = quader::math::dot(event.position - drag_start_screen_, drag_screen_axis_);
		if (event.modifiers.control) {
			axis_pixels *= 0.25F;
		}
		float factor = std::max(kMinScaleFactor, 1.0F + axis_pixels / kScalePixelsPerFactor);
		factor = snapped_scale_factor(scale_axis, factor, combined_bounds, drag_start_pivot_, event.snap_to_grid, event.grid_size);
		factor = std::max(kMinScaleFactor, factor);
		const quader::math::Vec3 factors = axis_scale(scale_axis, factor);
		for (const TargetSnapshot &target : targets_) {
			quader::document::Transform transform = target.original;
			transform.scale = multiply_components(transform.scale, factors);
			transform.translation = drag_start_pivot_ + multiply_components(target.original.translation - drag_start_pivot_, factors);
			preview_transforms_.push_back(transform);
		}
		state_.pivot = drag_start_pivot_;
		status_text_ = "Scale gizmo";
		(void)apply_preview_transforms(context);
		rebuild_preview();
		return true;
	}

	if (mode_ == TransformGizmoMode::Rotate) {
		if (state_.active_handle.kind != TransformGizmoHandleKind::RotationRing) {
			return false;
		}
		float angle_degrees = drag_rotate_accumulated_degrees_;
		if (drag_rotate_edge_on_) {
			const float kProjectedPixels = quader::math::dot(event.position - drag_start_screen_, drag_rotate_edge_axis_);
			angle_degrees = -kProjectedPixels * 90.0F / std::max(drag_rotate_edge_radius_pixels_, 1.0F);
			drag_rotate_accumulated_degrees_ = angle_degrees;
		} else {
			const quader::math::Vec2 kCurrentVector = event.position - (last_camera_.has_value()
							? build_gizmo_frame(mode_, drag_start_pivot_, *last_camera_).screen_pivot
							: drag_start_screen_);
			if (length2(kCurrentVector) > kScreenEpsilon) {
				const float kCurrentAngle = std::atan2(kCurrentVector.y, kCurrentVector.x);
				const float kDeltaDegrees = wrap_radians(kCurrentAngle - drag_rotate_last_angle_radians_) *
						drag_rotate_view_sign_ * 180.0F / kPi;
				drag_rotate_last_angle_radians_ = kCurrentAngle;
				drag_rotate_accumulated_degrees_ += kDeltaDegrees;
			}
			angle_degrees = drag_rotate_accumulated_degrees_;
		}
		if (event.snap_to_grid) {
			angle_degrees = snap_value(angle_degrees, kRotationSnapDegrees);
		}

		const quader::math::Vec3 kAxis = rotation_axis_vector(state_.active_handle.axis);
		const float kAngleRadians = radians_from_degrees(angle_degrees);
		for (const TargetSnapshot &target : targets_) {
			quader::document::Transform transform = target.original;
			switch (state_.active_handle.axis) {
				case TransformGizmoAxis::X:
					transform.rotation_euler.x += angle_degrees;
					break;
				case TransformGizmoAxis::Y:
					transform.rotation_euler.y += angle_degrees;
					break;
				case TransformGizmoAxis::Z:
					transform.rotation_euler.z += angle_degrees;
					break;
				case TransformGizmoAxis::Xy:
				case TransformGizmoAxis::Xz:
				case TransformGizmoAxis::Yz:
				case TransformGizmoAxis::All:
				case TransformGizmoAxis::None:
					break;
			}
			transform.translation = drag_start_pivot_ +
					rotate_around_axis(target.original.translation - drag_start_pivot_, kAxis, kAngleRadians);
			preview_transforms_.push_back(transform);
		}
		state_.pivot = drag_start_pivot_;
		drag_rotate_indicator_ = true;
		status_text_ = "Rotate gizmo";
		(void)apply_preview_transforms(context);
		rebuild_preview();
		return true;
	}

	return false;
}

bool TransformGizmoTool::commit_drag(ToolContext &context) {
	if (!state_.dragging || preview_transforms_.size() != targets_.size() || targets_.empty()) {
		return false;
	}

	std::vector<quader::commands::ObjectTransformEdit> edits;
	edits.reserve(targets_.size());
	for (std::size_t index = 0U; index < targets_.size(); ++index) {
		edits.push_back(quader::commands::ObjectTransformEdit{
				.object = targets_[index].object,
				.transform = preview_transforms_[index],
		});
	}

	(void)restore_original_transforms(context);
	auto result = context.execute_command(
			std::make_unique<quader::commands::BatchSetObjectTransformsCommand>(std::move(edits)));
	return result.is_applied();
}

bool TransformGizmoTool::apply_preview_transforms(ToolContext &context) {
	if (preview_transforms_.size() != targets_.size() || targets_.empty()) {
		return false;
	}

	std::vector<ToolContextTransformPreviewEdit> edits;
	edits.reserve(targets_.size());
	for (std::size_t index = 0U; index < targets_.size(); ++index) {
		edits.push_back(ToolContextTransformPreviewEdit{
				.object = targets_[index].object,
				.transform = preview_transforms_[index],
		});
	}

	const auto result = context.preview_object_transforms(edits);
	return result.is_applied();
}

bool TransformGizmoTool::restore_original_transforms(ToolContext &context) {
	if (targets_.empty()) {
		return false;
	}

	std::vector<ToolContextTransformPreviewEdit> edits;
	edits.reserve(targets_.size());
	for (const TargetSnapshot &target : targets_) {
		edits.push_back(ToolContextTransformPreviewEdit{
				.object = target.object,
				.transform = target.original,
		});
	}

	const auto result = context.preview_object_transforms(edits);
	return result.is_applied();
}

void TransformGizmoTool::cancel_drag(ToolContext &context) {
	(void)restore_original_transforms(context);
	clear_drag_state();
}

void TransformGizmoTool::clear_drag_state() {
	state_.dragging = false;
	state_.active_handle = {};
	state_.pivot = drag_start_pivot_;
	preview_transforms_.clear();
	status_text_.clear();
	drag_rotate_indicator_ = false;
}

void TransformGizmoTool::reset() {
	state_ = {};
	preview_.clear();
	targets_.clear();
	preview_transforms_.clear();
	status_text_.clear();
	drag_start_ray_ = {};
	drag_start_screen_ = {};
	drag_start_pivot_ = {};
	last_camera_ = std::nullopt;
	drag_camera_ = std::nullopt;
	drag_screen_axis_ = { 1.0F, 0.0F };
	drag_has_screen_axis_ = false;
	drag_axis_start_distance_ = 0.0F;
	drag_has_axis_start_distance_ = false;
	drag_rotate_last_angle_radians_ = 0.0F;
	drag_rotate_accumulated_degrees_ = 0.0F;
	drag_rotate_view_sign_ = 1.0F;
	drag_rotate_edge_on_ = false;
	drag_rotate_edge_axis_ = { 1.0F, 0.0F };
	drag_rotate_edge_radius_pixels_ = 1.0F;
	drag_rotate_start_direction_ = {};
	drag_rotate_current_position_ = {};
	drag_rotate_indicator_ = false;
	drag_start_scale_ = 1.0F;
}

void TransformGizmoTool::rebuild_preview() {
	preview_.clear();
	if (targets_.empty() || !last_camera_.has_value()) {
		return;
	}

	const GizmoFrame kFrame = build_gizmo_frame(mode_, state_.pivot, *last_camera_);
	if (!kFrame.ok) {
		return;
	}

	preview_.active = true;
	preview_.overlay_only = true;
	preview_.view_index = state_.view_index;
	preview_.status_text = status_text_.empty() ? "Transform gizmo" : status_text_;
	state_.gizmo_scale = kFrame.pixel_world * kFrame.unit_screen_size;

	const TransformGizmoAxis kActiveAxis = state_.dragging ? axis_from_handle(state_.active_handle) : TransformGizmoAxis::None;
	if (mode_ == TransformGizmoMode::Move || mode_ == TransformGizmoMode::Scale) {
		for (const GizmoPlaneFrame &plane : kFrame.planes) {
			if (!drag_plane_includes(kActiveAxis, plane.axis)) {
				continue;
			}
			const ToolPreviewColorSrgb kColor = highlighted(state_, plane.axis)
					? highlight_color_for_axis(plane.axis)
					: color_for_plane(plane.axis);
			append_plane(preview_, plane, kColor);
		}

		for (const GizmoAxisFrame &axis : kFrame.axes) {
			if (!drag_axis_includes(kActiveAxis, axis.axis)) {
				continue;
			}
			const ToolPreviewColorSrgb kColor = highlighted(state_, axis.axis)
					? highlight_color_for_axis(axis.axis)
					: color_for_axis(axis.axis);
			const float kLineWidth = kGizmoAxisLineWidthPixels * kFrame.width_scale;
			if (mode_ == TransformGizmoMode::Scale) {
				append_scale_handle(preview_, axis.world_start, axis.world_tip, kColor, kLineWidth);
			} else {
				append_arrow_handle(preview_, axis.world_start, axis.world_tip, kColor, kLineWidth);
			}
		}

		const bool kCenterHovered = axis_from_handle(state_.hover_handle) == TransformGizmoAxis::All ||
				axis_from_handle(state_.active_handle) == TransformGizmoAxis::All;
		if (mode_ == TransformGizmoMode::Scale && kCenterHovered) {
			const float kWorldSize = std::max(kToolEpsilon,
					first_axis_world_length(kFrame) * (kGizmoScaleCenterSizePixels / kGizmoSizePixels));
			append_cube_handle(preview_, *last_camera_, kFrame.pivot, kWorldSize, kAxisHighlightColor, true);
		}
	} else if (mode_ == TransformGizmoMode::Rotate) {
		for (const GizmoRingSegmentFrame &segment : kFrame.trackball) {
			append_line(preview_,
					segment.world_start,
					segment.world_end,
					kTrackballColor,
					kGizmoTrackballLineWidthPixels * kFrame.width_scale);
		}
		for (const GizmoRingSegmentFrame &segment : kFrame.rings) {
			if (!segment.front_facing) {
				continue;
			}
			const ToolPreviewColorSrgb kColor = highlighted(state_, segment.axis)
					? highlight_color_for_axis(segment.axis)
					: color_for_axis(segment.axis);
			append_line(preview_,
					segment.world_start,
					segment.world_end,
					kColor,
					kGizmoRotateRingLineWidthPixels * kFrame.width_scale);
		}
		append_rotate_indicator(preview_,
				state_,
				kFrame,
				drag_rotate_start_direction_,
				drag_rotate_accumulated_degrees_,
				drag_rotate_indicator_);
	}

}

} // namespace quader::tools
