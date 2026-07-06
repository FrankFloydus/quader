/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/viewport/viewport_controller.hpp"

#include "crimson/scene/render_camera_projection.hpp"
#include "geometry/normals.hpp"
#include "mesh/core/mesh_traversal.hpp"
#include "tools/tool_context.hpp"
#include "tools/tool_manager.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <variant>
#include <vector>

namespace quader::ui {
namespace {

constexpr float kRayEpsilon = 0.000001F;
constexpr float kPi = 3.14159265358979323846F;
constexpr float kVertexPickRadiusPx = 12.0F;
constexpr float kEdgePickRadiusPx = 8.0F;
constexpr float kHandleOcclusionBiasWorld = 0.02F;

[[nodiscard]] quader::math::Vec3 scale(quader::math::Vec3 value, float factor) noexcept {
	return quader::math::Vec3{ value.x * factor, value.y * factor, value.z * factor };
}

[[nodiscard]] quader::math::Vec3 normalized_or(quader::math::Vec3 value,
		quader::math::Vec3 fallback) noexcept {
	const auto kNormalized = quader::math::normalized(value, kRayEpsilon);
	return quader::math::length_squared(kNormalized) <= kRayEpsilon * kRayEpsilon ? fallback : kNormalized;
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

[[nodiscard]] float radians_from_degrees(float degrees) noexcept {
	return degrees * kPi / 180.0F;
}

[[nodiscard]] quader::math::Vec3 rotate_euler(quader::math::Vec3 value,
		quader::math::Vec3 degrees) noexcept {
	value = rotate_x(value, radians_from_degrees(degrees.x));
	value = rotate_y(value, radians_from_degrees(degrees.y));
	return rotate_z(value, radians_from_degrees(degrees.z));
}

[[nodiscard]] quader::math::Vec3 transform_point(quader::math::Vec3 point,
		quader::document::Transform transform) noexcept {
	point = quader::math::Vec3{
		point.x * transform.scale.x,
		point.y * transform.scale.y,
		point.z * transform.scale.z,
	};
	return rotate_euler(point, transform.rotation_euler) + transform.translation;
}

struct RayTriangleHit {
	bool hit = false;
	float distance = 0.0F;
	quader::math::Vec3 position;
};

struct PickViewContext {
	ViewportCameraSnapshot camera;
	ViewportPixelSize pane_size;
};

struct ProjectedPoint {
	quader::math::Vec2 position;
	float camera_distance = 0.0F;
};

[[nodiscard]] RayTriangleHit intersect_triangle(const quader::tools::ViewportRay &ray,
		quader::math::Vec3 a,
		quader::math::Vec3 b,
		quader::math::Vec3 c) noexcept {
	const quader::math::Vec3 kDirection = normalized_or(ray.direction, {});
	if (quader::math::length_squared(kDirection) <= kRayEpsilon * kRayEpsilon) {
		return {};
	}

	const quader::math::Vec3 kEdge1 = b - a;
	const quader::math::Vec3 kEdge2 = c - a;
	const quader::math::Vec3 kP = quader::math::cross(kDirection, kEdge2);
	const float kDet = quader::math::dot(kEdge1, kP);
	if (std::abs(kDet) <= kRayEpsilon) {
		return {};
	}

	const float kInvDet = 1.0F / kDet;
	const quader::math::Vec3 kT = ray.origin - a;
	const float kU = quader::math::dot(kT, kP) * kInvDet;
	if (kU < 0.0F || kU > 1.0F) {
		return {};
	}

	const quader::math::Vec3 kQ = quader::math::cross(kT, kEdge1);
	const float kV = quader::math::dot(kDirection, kQ) * kInvDet;
	if (kV < 0.0F || kU + kV > 1.0F) {
		return {};
	}

	const float kDistance = quader::math::dot(kEdge2, kQ) * kInvDet;
	if (kDistance < 0.0F || !quader::math::is_finite(kDistance)) {
		return {};
	}

	return RayTriangleHit{
		.hit = true,
		.distance = kDistance,
		.position = ray.origin + scale(kDirection, kDistance),
	};
}

struct RayHandleHit {
	bool hit = false;
	float ray_distance = 0.0F;
	float distance_squared = 0.0F;
	quader::math::Vec3 position;
};

struct ScreenHandleHit {
	bool hit = false;
	float screen_distance_squared = 0.0F;
	float ray_distance = 0.0F;
	quader::math::Vec3 position;
};

[[nodiscard]] float world_units_per_pixel(
		const PickViewContext &context,
		quader::math::Vec3 world_position) noexcept {
	const float kViewportHeightPx = std::max(1.0F, static_cast<float>(context.pane_size.height));
	if (context.camera.projection == CameraProjection::Orthographic) {
		return std::max(0.01F, context.camera.settings.orthographic_size) / kViewportHeightPx;
	}

	const quader::math::Vec3 kForward = normalized_or(
			context.camera.target - context.camera.eye,
			normalized_or(context.camera.forward, { 0.0F, 0.0F, -1.0F }));
	const float kForwardDistance = quader::math::dot(world_position - context.camera.eye, kForward);
	const float kDistance = std::max(0.001F, kForwardDistance);
	const float kFovRadians = radians_from_degrees(
			std::clamp(context.camera.settings.fov_degrees, 1.0F, 179.0F));
	return (2.0F * std::tan(kFovRadians * 0.5F) * kDistance) / kViewportHeightPx;
}

[[nodiscard]] float pick_radius_world(
		const PickViewContext &context,
		float radius_px,
		quader::math::Vec3 world_position) noexcept {
	return std::max(kRayEpsilon, radius_px * world_units_per_pixel(context, world_position));
}

[[nodiscard]] std::optional<ProjectedPoint> project_world_to_pane(
		const PickViewContext &context,
		quader::math::Vec3 world_position) noexcept {
	const float kViewportWidthPx = std::max(1.0F, static_cast<float>(context.pane_size.width));
	const float kViewportHeightPx = std::max(1.0F, static_cast<float>(context.pane_size.height));
	const float kAspect = kViewportWidthPx / kViewportHeightPx;
	const quader::math::Vec3 kForward = normalized_or(
			context.camera.target - context.camera.eye,
			normalized_or(context.camera.forward, { 0.0F, 0.0F, -1.0F }));
	const quader::math::Vec3 kRight = normalized_or(
			quader::math::cross(context.camera.up, kForward),
			{ -1.0F, 0.0F, 0.0F });
	const quader::math::Vec3 kUp = quader::math::cross(kForward, kRight);
	const quader::math::Vec3 kCameraRelative = world_position - context.camera.eye;
	const float kCameraDistance = quader::math::dot(kCameraRelative, kForward);
	if (!quader::math::is_finite(kCameraDistance)) {
		return std::nullopt;
	}

	float ndc_x = 0.0F;
	float ndc_y = 0.0F;
	if (context.camera.projection == CameraProjection::Orthographic) {
		const float kExtent = std::max(0.01F, context.camera.settings.orthographic_size) * 0.5F;
		ndc_x = quader::math::dot(kCameraRelative, kRight) / (kExtent * kAspect);
		ndc_y = quader::math::dot(kCameraRelative, kUp) / kExtent;
	} else {
		if (kCameraDistance <= kRayEpsilon) {
			return std::nullopt;
		}

		const float kTanY = std::tan(radians_from_degrees(
				std::clamp(context.camera.settings.fov_degrees, 1.0F, 179.0F)) * 0.5F);
		const float kTanX = kTanY * kAspect;
		ndc_x = quader::math::dot(kCameraRelative, kRight) / (kCameraDistance * kTanX);
		ndc_y = quader::math::dot(kCameraRelative, kUp) / (kCameraDistance * kTanY);
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

[[nodiscard]] float distance_squared_to_segment_2d(
		quader::math::Vec2 point,
		quader::math::Vec2 start,
		quader::math::Vec2 end,
		float &segment_t) noexcept {
	const quader::math::Vec2 kSegment = end - start;
	const float kLengthSquared = quader::math::length_squared(kSegment);
	if (kLengthSquared <= kRayEpsilon * kRayEpsilon) {
		segment_t = 0.0F;
		return quader::math::length_squared(point - start);
	}

	segment_t = std::clamp(quader::math::dot(point - start, kSegment) / kLengthSquared, 0.0F, 1.0F);
	const quader::math::Vec2 kClosest = start + kSegment * segment_t;
	return quader::math::length_squared(point - kClosest);
}

[[nodiscard]] RayHandleHit distance_to_ray_point(
		const quader::tools::ViewportRay &ray,
		quader::math::Vec3 point) noexcept {
	const quader::math::Vec3 kDirection = normalized_or(ray.direction, {});
	if (quader::math::length_squared(kDirection) <= kRayEpsilon * kRayEpsilon) {
		return {};
	}

	const float kRayDistance = quader::math::dot(point - ray.origin, kDirection);
	if (kRayDistance < 0.0F || !quader::math::is_finite(kRayDistance)) {
		return {};
	}

	const quader::math::Vec3 kClosest = ray.origin + scale(kDirection, kRayDistance);
	return RayHandleHit{
		.hit = true,
		.ray_distance = kRayDistance,
		.distance_squared = quader::math::length_squared(point - kClosest),
		.position = point,
	};
}

[[nodiscard]] RayHandleHit distance_to_ray_segment(
		const quader::tools::ViewportRay &ray,
		quader::math::Vec3 start,
		quader::math::Vec3 end) noexcept {
	const quader::math::Vec3 kDirection = normalized_or(ray.direction, {});
	const quader::math::Vec3 kSegment = end - start;
	const float kSegmentLengthSquared = quader::math::length_squared(kSegment);
	if (quader::math::length_squared(kDirection) <= kRayEpsilon * kRayEpsilon ||
			kSegmentLengthSquared <= kRayEpsilon * kRayEpsilon) {
		return distance_to_ray_point(ray, start);
	}

	const quader::math::Vec3 kDelta = ray.origin - start;
	const float kA = quader::math::dot(kDirection, kDirection);
	const float kB = quader::math::dot(kDirection, kSegment);
	const float kC = kSegmentLengthSquared;
	const float kD = quader::math::dot(kDirection, kDelta);
	const float kE = quader::math::dot(kSegment, kDelta);
	const float kDenominator = kA * kC - kB * kB;

	float ray_t = 0.0F;
	float segment_t = 0.0F;
	if (std::abs(kDenominator) > kRayEpsilon) {
		ray_t = (kB * kE - kC * kD) / kDenominator;
		segment_t = (kA * kE - kB * kD) / kDenominator;
	} else {
		RayHandleHit best;
		const auto consider_segment_t = [&](float candidate_segment_t) {
			const float kSegmentT = std::clamp(candidate_segment_t, 0.0F, 1.0F);
			const quader::math::Vec3 kSegmentPoint = start + scale(kSegment, kSegmentT);
			const float kRayT = std::max(0.0F, quader::math::dot(kSegmentPoint - ray.origin, kDirection));
			const quader::math::Vec3 kRayPoint = ray.origin + scale(kDirection, kRayT);
			const float kDistanceSquared = quader::math::length_squared(kRayPoint - kSegmentPoint);
			if (!best.hit ||
					kDistanceSquared < best.distance_squared - kRayEpsilon ||
					(std::abs(kDistanceSquared - best.distance_squared) <= kRayEpsilon && kRayT < best.ray_distance)) {
				best = RayHandleHit{
					.hit = true,
					.ray_distance = kRayT,
					.distance_squared = kDistanceSquared,
					.position = kSegmentPoint,
				};
			}
		};
		consider_segment_t(0.0F);
		consider_segment_t(1.0F);
		consider_segment_t(kE / kC);
		return best;
	}
	if (!std::isfinite(ray_t) || !std::isfinite(segment_t)) {
		return {};
	}

	if (ray_t < 0.0F) {
		ray_t = 0.0F;
		segment_t = std::clamp(kE / kC, 0.0F, 1.0F);
	} else if (segment_t < 0.0F) {
		segment_t = 0.0F;
		ray_t = std::max(0.0F, -kD / kA);
	} else if (segment_t > 1.0F) {
		segment_t = 1.0F;
		ray_t = std::max(0.0F, (kB - kD) / kA);
	}

	const quader::math::Vec3 kRayPoint = ray.origin + scale(kDirection, ray_t);
	const quader::math::Vec3 kSegmentPoint = start + scale(kSegment, segment_t);
	return RayHandleHit{
		.hit = true,
		.ray_distance = ray_t,
		.distance_squared = quader::math::length_squared(kRayPoint - kSegmentPoint),
		.position = kSegmentPoint,
	};
}

[[nodiscard]] std::uint64_t encoded_object_id(quader::document::ObjectId id) noexcept {
	return (static_cast<std::uint64_t>(id.generation()) << 32U) | id.index();
}

[[nodiscard]] std::optional<quader::math::Vec3> vertex_world_position(
		const quader::document::MeshObject &object,
		quader::mesh::VertexId vertex) {
	auto position = object.mesh.vertex_position(vertex);
	if (!position) {
		return std::nullopt;
	}
	return transform_point(position.value(), object.transform);
}

struct EdgeWorldPoints {
	quader::math::Vec3 start;
	quader::math::Vec3 end;
};

[[nodiscard]] std::optional<EdgeWorldPoints> edge_world_points(
		const quader::document::MeshObject &object,
		quader::mesh::EdgeId edge) {
	auto halfedges = object.mesh.edge_halfedges(edge);
	if (!halfedges) {
		return std::nullopt;
	}

	const quader::mesh::HalfedgeId kHalfedge = halfedges.value()[0];
	auto origin = object.mesh.halfedge_origin(kHalfedge);
	auto target = object.mesh.halfedge_target(kHalfedge);
	if (!origin || !target) {
		return std::nullopt;
	}

	auto start = vertex_world_position(object, origin.value());
	auto end = vertex_world_position(object, target.value());
	if (!start || !end) {
		return std::nullopt;
	}

	return EdgeWorldPoints{ *start, *end };
}

struct FaceHitCandidate {
	quader::document::ObjectId object_id = quader::document::ObjectId::invalid();
	quader::mesh::FaceId face_id = quader::mesh::FaceId::invalid();
	float distance = std::numeric_limits<float>::infinity();
	quader::math::Vec3 position;
	quader::math::Vec3 normal{ 0.0F, 1.0F, 0.0F };
};

[[nodiscard]] const FaceHitCandidate *nearest_face_for_object(
		const std::vector<FaceHitCandidate> &faces,
		quader::document::ObjectId object_id) noexcept {
	const FaceHitCandidate *best = nullptr;
	for (const FaceHitCandidate &face : faces) {
		if (face.object_id != object_id) {
			continue;
		}
		if (best == nullptr || face.distance < best->distance) {
			best = &face;
		}
	}
	return best;
}

[[nodiscard]] bool handle_occluded_by_same_mesh(
		const std::vector<FaceHitCandidate> &faces,
		quader::document::ObjectId object_id,
		float handle_ray_distance,
		float handle_radius_world) noexcept {
	const FaceHitCandidate *nearest_face = nearest_face_for_object(faces, object_id);
	return nearest_face != nullptr &&
			handle_ray_distance > nearest_face->distance + kHandleOcclusionBiasWorld + handle_radius_world;
}

[[nodiscard]] bool better_handle_hit(
		const RayHandleHit &candidate,
		const RayHandleHit &best) noexcept {
	if (!best.hit) {
		return true;
	}
	if (candidate.distance_squared < best.distance_squared - kRayEpsilon) {
		return true;
	}
	if (candidate.distance_squared > best.distance_squared + kRayEpsilon) {
		return false;
	}
	return candidate.ray_distance < best.ray_distance;
}

[[nodiscard]] bool better_screen_handle_hit(
		const ScreenHandleHit &candidate,
		const ScreenHandleHit &best) noexcept {
	if (!best.hit) {
		return true;
	}
	if (candidate.screen_distance_squared < best.screen_distance_squared - kRayEpsilon) {
		return true;
	}
	if (candidate.screen_distance_squared > best.screen_distance_squared + kRayEpsilon) {
		return false;
	}
	return candidate.ray_distance < best.ray_distance;
}

void append_unique_object_id(
		std::vector<quader::document::ObjectId> &objects,
		quader::document::ObjectId object) {
	if (object.is_valid() && std::find(objects.begin(), objects.end(), object) == objects.end()) {
		objects.push_back(object);
	}
}

[[nodiscard]] bool component_matches_selection_mode(
		const quader::document::ComponentRef &component,
		quader::document::SelectionMode mode) noexcept {
	switch (mode) {
		case quader::document::SelectionMode::Vertex:
			return std::holds_alternative<quader::mesh::VertexId>(component.component);
		case quader::document::SelectionMode::Edge:
			return std::holds_alternative<quader::mesh::EdgeId>(component.component);
		case quader::document::SelectionMode::Face:
			return std::holds_alternative<quader::mesh::FaceId>(component.component);
		case quader::document::SelectionMode::Object:
			return false;
	}
	return false;
}

[[nodiscard]] std::vector<quader::document::ObjectId> component_pick_object_order(
		const quader::document::Document &document,
		const std::optional<FaceHitCandidate> &best_face,
		const std::optional<quader::tools::SurfaceHit> &current_hover) {
	std::vector<quader::document::ObjectId> objects;
	if (document.selection().mode() != quader::document::SelectionMode::Object) {
		for (const auto &component : document.selection().selected_components()) {
			if (component_matches_selection_mode(component, document.selection().mode())) {
				append_unique_object_id(objects, component.object);
			}
		}
		if (current_hover.has_value()) {
			append_unique_object_id(objects, current_hover->document_object_id);
		}
		if (best_face.has_value()) {
			append_unique_object_id(objects, best_face->object_id);
		}
		for (const auto &component : document.selection().selected_components()) {
			append_unique_object_id(objects, component.object);
		}
		for (const quader::document::ObjectId object : document.selection().selected_objects()) {
			append_unique_object_id(objects, object);
		}
	} else if (best_face.has_value()) {
		append_unique_object_id(objects, best_face->object_id);
	}

	for (const quader::document::ObjectId object : document.object_ids()) {
		append_unique_object_id(objects, object);
	}
	return objects;
}

struct SelectionSurfaceHitTarget {
	quader::tools::SurfaceHitKind kind = quader::tools::SurfaceHitKind::Face;
	std::uint32_t component_index = 0;
};

[[nodiscard]] std::optional<std::size_t> pane_array_index_at(
		const ViewportLayoutState &layout,
		ViewportPixelSize size,
		ViewportPoint point) {
	const ViewportPaneArray kPanes = layout.panes_for(size);
	for (int index = 0; index < layout.pane_count(); ++index) {
		const ViewportRect &rect = kPanes[static_cast<std::size_t>(index)].rect;
		if (point.x >= rect.x && point.y >= rect.y && point.x < rect.x + rect.width && point.y < rect.y + rect.height) {
			return static_cast<std::size_t>(index);
		}
	}
	return std::nullopt;
}

} // namespace

ViewportController::ViewportController(IViewportRenderHost &render_host, QObject *parent) : QObject(parent), render_host_(render_host) {
}

ViewportController::ViewportController(IViewportRenderHost &render_host,
		quader::tools::ToolManager &tool_manager,
		QObject *parent) : QObject(parent),
						   render_host_(render_host),
						   tool_manager_(&tool_manager) {
}

bool ViewportController::surface_initialized() const noexcept {
	return surface_initialized_;
}

NavigationMode ViewportController::navigation_mode() const noexcept {
	return cameras_.navigation_mode();
}

void ViewportController::initialize_surface(
		const NativeViewportSurface &surface,
		ViewportPixelSize size,
		double device_pixel_ratio) {
	surface_size_ = safe_size(size);
	device_pixel_ratio_ = std::max(0.01, device_pixel_ratio);

	const ViewportRenderResult kResult = render_host_.initialize_surface(surface, surface_size_, device_pixel_ratio_);
	if (!kResult.ok) {
		surface_initialized_ = false;
		emit_result_error(kResult);
		return;
	}

	surface_initialized_ = true;
	const QString kRendererName = kResult.renderer_name.isEmpty() ? render_host_.renderer_name() : kResult.renderer_name;
	if (!kRendererName.isEmpty()) {
		Q_EMIT renderer_ready(kRendererName);
	}
	request_frame();
}

void ViewportController::resize_surface(ViewportPixelSize size, double device_pixel_ratio) {
	surface_size_ = safe_size(size);
	device_pixel_ratio_ = std::max(0.01, device_pixel_ratio);

	if (!surface_initialized_) {
		return;
	}

	const ViewportRenderResult kResult = render_host_.resize_surface(surface_size_, device_pixel_ratio_);
	if (!kResult.ok) {
		emit_result_error(kResult);
	}
	request_frame();
}

void ViewportController::shutdown_surface() {
	if (!surface_initialized_) {
		return;
	}

	render_host_.shutdown_surface();
	surface_initialized_ = false;
}

void ViewportController::set_quad_viewports_enabled(bool enabled) {
	if (layout_.quad_enabled() == enabled) {
		return;
	}

	layout_.set_quad_enabled(enabled);
	Q_EMIT viewport_layout_changed(
			enabled,
			enabled ? QStringLiteral("Four Viewports") : QStringLiteral("Single Viewport"));
	request_frame();
}

bool ViewportController::quad_viewports_enabled() const noexcept {
	return layout_.quad_enabled();
}

void ViewportController::set_scene_animation_enabled(bool enabled) {
	if (scene_animation_enabled_ == enabled) {
		return;
	}

	scene_animation_enabled_ = enabled;
	request_frame();
}

bool ViewportController::scene_animation_enabled() const noexcept {
	return scene_animation_enabled_;
}

void ViewportController::set_shading_mode(ViewportShadingMode mode) {
	if (shading_mode_ == mode) {
		return;
	}

	shading_mode_ = mode;
	request_frame();
}

ViewportShadingMode ViewportController::shading_mode() const noexcept {
	return shading_mode_;
}

void ViewportController::render_frame(double elapsed_seconds, float delta_seconds) {
	if (!surface_initialized_) {
		return;
	}

	cameras_.tick_fly_navigation(delta_seconds);
	refresh_transform_tool_hover();

	const ViewportPaneArray kPanes = layout_.panes_for(surface_size_);
	const ViewportCameraSnapshotArray kCameras = cameras_.camera_snapshots();
	const int kPaneCount = layout_.pane_count();
	const ViewportRenderRequest kRequest{
		.surface_size = surface_size_,
		.device_pixel_ratio = device_pixel_ratio_,
		.layout_mode = layout_.mode(),
		.panes = std::span<const ViewportPane>(kPanes.data(), static_cast<std::size_t>(kPaneCount)),
		.cameras = std::span<const ViewportCameraSnapshot>(kCameras.data(), kCameras.size()),
		.shading_mode = shading_mode_,
		.scene_animation_enabled = scene_animation_enabled_,
		.elapsed_seconds = elapsed_seconds,
	};

	const ViewportRenderResult kResult = render_host_.render_frame(kRequest);
	if (!kResult.ok) {
		emit_result_error(kResult);
		return;
	}

	if (const std::optional<ViewportFrameStats> kStats = render_host_.latest_frame_stats()) {
		Q_EMIT frame_stats_changed(*kStats);
	}
}

void ViewportController::request_frame() {
	Q_EMIT frame_requested();
}

ViewportLayoutState &ViewportController::layout_state() noexcept {
	return layout_;
}

const ViewportLayoutState &ViewportController::layout_state() const noexcept {
	return layout_;
}

const ViewportCameraController &ViewportController::camera_controller() const noexcept {
	return cameras_;
}

bool ViewportController::handle_mouse_press(
		ViewportMouseButton button,
		ViewportPoint point,
		bool shift_modifier,
		ViewportPixelSize size,
		bool control_modifier,
		bool alt_modifier) {
	remember_tool_pointer(point, size, button == ViewportMouseButton::Left);
	const auto clear_hover = [this]() {
		return tool_manager_ != nullptr && tool_manager_->clear_selection_hover();
	};

	const auto kPaneIndex = pane_array_index_at(layout_, size, point);
	if (kPaneIndex.has_value()) {
		const ViewportPaneArray kPanes = layout_.panes_for(size);
		cameras_.set_active_camera_index(kPanes[*kPaneIndex].camera_index);
	}

	if (button == ViewportMouseButton::Middle) {
		const NavigationMode kMode = shift_modifier ? NavigationMode::Pan : NavigationMode::Orbit;
		if (cameras_.begin_navigation(cameras_.active_camera_index(), kMode, point)) {
			clear_hover();
			request_frame();
			return true;
		}
	} else if (button == ViewportMouseButton::Right) {
		const NavigationMode kMode = cameras_.camera_projection(cameras_.active_camera_index()) == CameraProjection::Orthographic
				? NavigationMode::Pan
				: NavigationMode::Fly;
		if (cameras_.begin_navigation(cameras_.active_camera_index(), kMode, point)) {
			clear_hover();
			request_frame();
			return true;
		}
	}

	if (button == ViewportMouseButton::Left) {
		return dispatch_tool_pointer(button,
				point,
				size,
				quader::tools::PointerPhase::Press,
				true,
				quader::tools::KeyboardModifiers{
						.shift = shift_modifier,
						.control = control_modifier,
						.alt = alt_modifier,
				});
	}

	return false;
}

bool ViewportController::handle_mouse_move(ViewportPoint point,
		ViewportPixelSize size,
		bool left_button_pressed,
		bool shift_modifier,
		bool control_modifier,
		bool alt_modifier) {
	remember_tool_pointer(point, size, left_button_pressed);
	if (cameras_.navigation_mode() != NavigationMode::None) {
		cameras_.update_navigation(point, pane_size_for_camera(size, cameras_.active_camera_index()));
		request_frame();
		return true;
	}

	return dispatch_tool_pointer(
			left_button_pressed ? ViewportMouseButton::Left : ViewportMouseButton::Other,
			point,
			size,
			left_button_pressed ? quader::tools::PointerPhase::Move : quader::tools::PointerPhase::Hover,
			left_button_pressed,
			quader::tools::KeyboardModifiers{
					.shift = shift_modifier,
					.control = control_modifier,
					.alt = alt_modifier,
			});
}

bool ViewportController::handle_mouse_release(ViewportMouseButton button,
		ViewportPoint point,
		ViewportPixelSize size,
		bool shift_modifier,
		bool control_modifier,
		bool alt_modifier) {
	remember_tool_pointer(point, size, false);
	if (cameras_.navigation_mode() != NavigationMode::None) {
		cameras_.end_navigation();
		request_frame();
		return true;
	}

	if (button == ViewportMouseButton::Left) {
		return dispatch_tool_pointer(button,
				point,
				size,
				quader::tools::PointerPhase::Release,
				false,
				quader::tools::KeyboardModifiers{
						.shift = shift_modifier,
						.control = control_modifier,
						.alt = alt_modifier,
				});
	}

	return false;
}

void ViewportController::handle_mouse_leave() {
	if (tool_manager_ != nullptr && tool_manager_->clear_selection_hover()) {
		request_frame();
	}
}

void ViewportController::handle_wheel(float wheel_steps, ViewportPoint point, ViewportPixelSize size) {
	if (std::abs(wheel_steps) <= 0.000001F) {
		return;
	}
	remember_tool_pointer(point, size, false);

	if (cameras_.navigation_mode() == NavigationMode::Fly) {
		cameras_.adjust_fly_speed(wheel_steps);
		return;
	}

	const auto kPaneIndex = pane_array_index_at(layout_, size, point);
	const ViewportPaneArray kPanes = layout_.panes_for(size);
	const int kCameraIndex = kPaneIndex.has_value() ? kPanes[*kPaneIndex].camera_index : cameras_.active_camera_index();
	cameras_.apply_wheel_zoom(kCameraIndex, wheel_steps, pane_size_for_camera(size, kCameraIndex));
	request_frame();
}

bool ViewportController::handle_key(ViewportKey key, bool pressed, bool auto_repeat) {
	if (cameras_.navigation_mode() == NavigationMode::Fly) {
		if (!auto_repeat) {
			switch (key) {
				case ViewportKey::W:
					cameras_.set_fly_key(ViewportFlyKey::Forward, pressed);
					break;
				case ViewportKey::A:
					cameras_.set_fly_key(ViewportFlyKey::Left, pressed);
					break;
				case ViewportKey::S:
					cameras_.set_fly_key(ViewportFlyKey::Backward, pressed);
					break;
				case ViewportKey::D:
					cameras_.set_fly_key(ViewportFlyKey::Right, pressed);
					break;
				case ViewportKey::Escape:
				case ViewportKey::Other:
					break;
			}
		}
		return true;
	}

	if (key == ViewportKey::Escape && pressed && !auto_repeat && tool_manager_ != nullptr) {
		const bool kCanceled = tool_manager_->cancel_active_tool();
		if (kCanceled) {
			request_frame();
		}
		return kCanceled;
	}

	return false;
}

bool ViewportController::overrides_action_shortcut(ViewportKey key) const noexcept {
	if (cameras_.navigation_mode() != NavigationMode::Fly) {
		return false;
	}

	switch (key) {
		case ViewportKey::W:
		case ViewportKey::A:
		case ViewportKey::S:
		case ViewportKey::D:
			return true;
		case ViewportKey::Escape:
		case ViewportKey::Other:
			return false;
	}

	return false;
}

std::optional<quader::tools::ViewportRay> ViewportController::ray_for_point(
		ViewportPoint point,
		ViewportPixelSize size) const {
	const auto kPaneIndex = pane_array_index_at(layout_, size, point);
	if (!kPaneIndex.has_value()) {
		return std::nullopt;
	}

	const ViewportPaneArray kPanes = layout_.panes_for(size);
	const ViewportPane &pane = kPanes[*kPaneIndex];
	const ViewportCameraSnapshotArray kSnapshots = cameras_.camera_snapshots();
	const ViewportCameraSnapshot &camera = kSnapshots[static_cast<std::size_t>(std::clamp(pane.camera_index, 0, 3))];
	const crimson::RenderCamera kCamera{
		.eye = camera.eye,
		.target = camera.target,
		.up = camera.up,
		.forward = camera.forward,
		.projection = camera.projection == CameraProjection::Orthographic
				? crimson::CameraProjection::Orthographic
				: crimson::CameraProjection::Perspective,
		.near_plane_m = camera.settings.clip.near_clip_m,
		.far_plane_m = camera.settings.clip.far_clip_m,
		.vertical_fov_degrees = camera.settings.fov_degrees,
		.orthographic_height_m = camera.settings.orthographic_size,
	};
	const crimson::RenderCameraRay kRay = crimson::render_camera_ray_from_viewport_point(
			kCamera,
			crimson::RenderCameraViewportSize{
					static_cast<float>(std::max(1, pane.rect.width)),
					static_cast<float>(std::max(1, pane.rect.height)),
			},
			quader::math::Vec2{
					static_cast<float>(point.x - pane.rect.x),
					static_cast<float>(point.y - pane.rect.y),
			},
			crimson::current_render_homogeneous_depth());
	return quader::tools::ViewportRay{
		.origin = kRay.origin,
		.direction = kRay.direction,
	};
}

std::optional<quader::tools::SurfaceHit> ViewportController::surface_hit_for_ray(
		const quader::tools::ViewportRay &ray,
		ViewportPoint point,
		ViewportPixelSize size) const {
	if (tool_manager_ == nullptr) {
		return std::nullopt;
	}

	const auto kPaneIndex = pane_array_index_at(layout_, size, point);
	if (!kPaneIndex.has_value()) {
		return std::nullopt;
	}

	const ViewportPaneArray kPanes = layout_.panes_for(size);
	const ViewportPane &pane = kPanes[*kPaneIndex];
	const ViewportCameraSnapshotArray kSnapshots = cameras_.camera_snapshots();
	const ViewportCameraSnapshot &camera = kSnapshots[static_cast<std::size_t>(std::clamp(pane.camera_index, 0, 3))];
	const PickViewContext kPickContext{
		.camera = camera,
		.pane_size = ViewportPixelSize{
				std::max(1, pane.rect.width),
				std::max(1, pane.rect.height),
		},
	};
	const quader::math::Vec2 kLocalPoint{
		static_cast<float>(point.x - pane.rect.x),
		static_cast<float>(point.y - pane.rect.y),
	};
	const auto &document = tool_manager_->context().document();
	const bool kSelectionToolActive = tool_manager_->active_tool_id() == quader::tools::ToolId::Select;
	std::vector<FaceHitCandidate> face_hits;
	std::optional<FaceHitCandidate> best_face;
	for (const auto kObjectId : document.object_ids()) {
		const auto *object = document.find_mesh_object(kObjectId);
		if (object == nullptr) {
			continue;
		}

		for (const auto kFaceId : object->mesh.face_ids()) {
			auto face_vertices = quader::mesh::face_vertices(object->mesh, kFaceId);
			if (!face_vertices || face_vertices.value().size() < 3U) {
				continue;
			}

			std::vector<quader::math::Vec3> points;
			points.reserve(face_vertices.value().size());
			for (const auto kVertex : face_vertices.value()) {
				auto position = object->mesh.vertex_position(kVertex);
				if (!position) {
					points.clear();
					break;
				}
				points.push_back(transform_point(position.value(), object->transform));
			}
			if (points.size() < 3U) {
				continue;
			}

			const quader::math::Vec3 kNormal = normalized_or(
					quader::geometry::polygon_normal(points),
					{ 0.0F, 1.0F, 0.0F });
			for (std::size_t index = 1U; index + 1U < points.size(); ++index) {
				const RayTriangleHit kHit = intersect_triangle(ray, points[0], points[index], points[index + 1U]);
				if (!kHit.hit) {
					continue;
				}

				FaceHitCandidate face_hit{
					.object_id = kObjectId,
					.face_id = kFaceId,
					.distance = kHit.distance,
					.position = kHit.position,
					.normal = kNormal,
				};
				face_hits.push_back(face_hit);
				if (!best_face.has_value() || kHit.distance < best_face->distance) {
					best_face = face_hit;
				}
			}
		}
	}

	const std::vector<quader::document::ObjectId> kComponentPickObjects =
			component_pick_object_order(document, best_face, tool_manager_->selection_hover());
	if (kSelectionToolActive && tool_manager_->selection_mode() == quader::tools::SelectionMode::Vertex) {
		ScreenHandleHit best_handle;
		std::optional<quader::tools::SurfaceHit> best_hit;
		for (const auto kObjectId : kComponentPickObjects) {
			const auto *object = document.find_mesh_object(kObjectId);
			if (object == nullptr) {
				continue;
			}

			for (const quader::mesh::VertexId kVertex : object->mesh.vertex_ids()) {
				auto position = vertex_world_position(*object, kVertex);
				if (!position) {
					continue;
				}

				const auto kProjected = project_world_to_pane(kPickContext, *position);
				if (!kProjected) {
					continue;
				}

				const float kScreenDistanceSquared = quader::math::length_squared(kLocalPoint - kProjected->position);
				const RayHandleHit kRayHandle = distance_to_ray_point(ray, *position);
				const float kVertexPickRadiusWorld = pick_radius_world(kPickContext, kVertexPickRadiusPx, *position);
				const ScreenHandleHit kScreenHandle{
					.hit = true,
					.screen_distance_squared = kScreenDistanceSquared,
					.ray_distance = kRayHandle.ray_distance,
					.position = *position,
				};
				if (kScreenDistanceSquared > kVertexPickRadiusPx * kVertexPickRadiusPx ||
						!kRayHandle.hit ||
						handle_occluded_by_same_mesh(face_hits, kObjectId, kRayHandle.ray_distance, kVertexPickRadiusWorld) ||
						!better_screen_handle_hit(kScreenHandle, best_handle)) {
					continue;
				}

				const FaceHitCandidate *nearest_face = nearest_face_for_object(face_hits, kObjectId);
				best_handle = kScreenHandle;
				best_hit = quader::tools::SurfaceHit{
					.position = kScreenHandle.position,
					.normal = nearest_face != nullptr ? nearest_face->normal : quader::math::Vec3{ 0.0F, 1.0F, 0.0F },
					.object_id = encoded_object_id(kObjectId),
					.component_index = kVertex.index(),
					.document_object_id = kObjectId,
					.component = kVertex,
					.kind = quader::tools::SurfaceHitKind::Vertex,
				};
			}
		}

		if (best_hit.has_value()) {
			return best_hit;
		}
	}

	if (kSelectionToolActive && tool_manager_->selection_mode() == quader::tools::SelectionMode::Edge) {
		ScreenHandleHit best_handle;
		std::optional<quader::tools::SurfaceHit> best_hit;
		for (const auto kObjectId : kComponentPickObjects) {
			const auto *object = document.find_mesh_object(kObjectId);
			if (object == nullptr) {
				continue;
			}

			for (const quader::mesh::EdgeId kEdge : object->mesh.edge_ids()) {
				auto points = edge_world_points(*object, kEdge);
				if (!points) {
					continue;
				}

				const auto kProjectedStart = project_world_to_pane(kPickContext, points->start);
				const auto kProjectedEnd = project_world_to_pane(kPickContext, points->end);
				if (!kProjectedStart || !kProjectedEnd) {
					continue;
				}

				float segment_t = 0.0F;
				const float kScreenDistanceSquared = distance_squared_to_segment_2d(
						kLocalPoint,
						kProjectedStart->position,
						kProjectedEnd->position,
						segment_t);
				const RayHandleHit kRayHandle = distance_to_ray_segment(ray, points->start, points->end);
				const quader::math::Vec3 kWorldPosition = kRayHandle.hit
						? kRayHandle.position
						: points->start + scale(points->end - points->start, segment_t);
				const float kEdgePickRadiusWorld = pick_radius_world(kPickContext, kEdgePickRadiusPx, kWorldPosition);
				const ScreenHandleHit kScreenHandle{
					.hit = true,
					.screen_distance_squared = kScreenDistanceSquared,
					.ray_distance = kRayHandle.ray_distance,
					.position = kWorldPosition,
				};
				if (kScreenDistanceSquared > kEdgePickRadiusPx * kEdgePickRadiusPx ||
						!kRayHandle.hit ||
						handle_occluded_by_same_mesh(face_hits, kObjectId, kRayHandle.ray_distance, kEdgePickRadiusWorld) ||
						!better_screen_handle_hit(kScreenHandle, best_handle)) {
					continue;
				}

				const FaceHitCandidate *nearest_face = nearest_face_for_object(face_hits, kObjectId);
				best_handle = kScreenHandle;
				best_hit = quader::tools::SurfaceHit{
					.position = kScreenHandle.position,
					.normal = nearest_face != nullptr ? nearest_face->normal : quader::math::Vec3{ 0.0F, 1.0F, 0.0F },
					.object_id = encoded_object_id(kObjectId),
					.component_index = kEdge.index(),
					.document_object_id = kObjectId,
					.component = kEdge,
					.kind = quader::tools::SurfaceHitKind::Edge,
				};
			}
		}

		if (best_hit.has_value()) {
			return best_hit;
		}
	}

	if (!best_face.has_value()) {
		return std::nullopt;
	}

	const auto *object = document.find_mesh_object(best_face->object_id);
	if (object == nullptr) {
		return std::nullopt;
	}

	SelectionSurfaceHitTarget target{
		quader::tools::SurfaceHitKind::Face,
		best_face->face_id.index(),
	};
	quader::tools::SurfaceHitComponent component = best_face->face_id;
	if (kSelectionToolActive) {
		switch (tool_manager_->selection_mode()) {
			case quader::tools::SelectionMode::Object:
				target = SelectionSurfaceHitTarget{ quader::tools::SurfaceHitKind::Object, 0U };
				component = std::monostate{};
				break;
			case quader::tools::SelectionMode::Face:
				target = SelectionSurfaceHitTarget{ quader::tools::SurfaceHitKind::Face, best_face->face_id.index() };
				component = best_face->face_id;
				break;
			case quader::tools::SelectionMode::Vertex:
			case quader::tools::SelectionMode::Edge:
				target = SelectionSurfaceHitTarget{ quader::tools::SurfaceHitKind::Object, 0U };
				component = std::monostate{};
				break;
		}
	}

	return quader::tools::SurfaceHit{
		.position = best_face->position,
		.normal = best_face->normal,
		.object_id = encoded_object_id(best_face->object_id),
		.component_index = target.component_index,
		.document_object_id = best_face->object_id,
		.component = component,
		.kind = target.kind,
	};
}

bool ViewportController::dispatch_tool_pointer(ViewportMouseButton button,
		ViewportPoint point,
		ViewportPixelSize size,
		quader::tools::PointerPhase phase,
		bool pressed,
		quader::tools::KeyboardModifiers modifiers) {
	if (tool_manager_ == nullptr || tool_manager_->active_tool() == nullptr) {
		return false;
	}
	remember_tool_pointer(point, size, button == ViewportMouseButton::Left && pressed);

	const auto kViewIndex = pane_array_index_at(layout_, size, point);
	if (!kViewIndex.has_value()) {
		return false;
	}

	const ViewportPaneArray kPanes = layout_.panes_for(size);
	const ViewportPane &pane = kPanes[*kViewIndex];
	const quader::math::Vec2 kLocalPosition{
		static_cast<float>(point.x - pane.rect.x),
		static_cast<float>(point.y - pane.rect.y),
	};
	const auto kRay = ray_for_point(point, size);
	const ViewportCameraSnapshotArray kSnapshots = cameras_.camera_snapshots();
	const ViewportCameraSnapshot &camera = kSnapshots[static_cast<std::size_t>(std::clamp(pane.camera_index, 0, 3))];
	quader::tools::PointerEvent event{
		.position = kLocalPosition,
		.button = button == ViewportMouseButton::Left ? quader::tools::PointerButton::Left
				: button == ViewportMouseButton::Middle
				? quader::tools::PointerButton::Middle
				: button == ViewportMouseButton::Right ? quader::tools::PointerButton::Right
													   : quader::tools::PointerButton::None,
		.pressed = pressed,
		.phase = phase,
		.modifiers = modifiers,
		.navigation_active = cameras_.navigation_mode() != NavigationMode::None,
		.snap_to_grid = true,
		.grid_size = 1.0F,
		.ray = kRay,
		.camera = quader::tools::ViewportCameraInput{
				.eye = camera.eye,
				.target = camera.target,
				.up = camera.up,
				.forward = camera.forward,
				.projection = camera.projection == CameraProjection::Orthographic
						? quader::tools::ViewportCameraProjection::Orthographic
						: quader::tools::ViewportCameraProjection::Perspective,
				.fov_degrees = camera.settings.fov_degrees,
				.orthographic_size = camera.settings.orthographic_size,
				.viewport_size_pixels = {
						static_cast<float>(std::max(1, pane.rect.width)),
						static_cast<float>(std::max(1, pane.rect.height)),
				},
		},
		.view_index = static_cast<std::uint32_t>(*kViewIndex),
	};
	if (kRay.has_value()) {
		event.surface_hit = surface_hit_for_ray(*kRay, point, size);
	}

	const bool kHandled = tool_manager_->dispatch_pointer_event(event);
	if (kHandled) {
		request_frame();
	}
	return kHandled;
}

void ViewportController::remember_tool_pointer(ViewportPoint point,
		ViewportPixelSize size,
		bool left_pressed) noexcept {
	last_tool_pointer_point_ = point;
	last_tool_pointer_size_ = safe_size(size);
	tool_pointer_left_pressed_ = left_pressed;
}

void ViewportController::refresh_transform_tool_hover() {
	if (tool_manager_ == nullptr || tool_pointer_left_pressed_) {
		return;
	}
	const std::optional<quader::tools::ToolId> kActiveTool = tool_manager_->active_tool_id();
	if (!kActiveTool.has_value() ||
			(*kActiveTool != quader::tools::ToolId::Move &&
					*kActiveTool != quader::tools::ToolId::Rotate &&
					*kActiveTool != quader::tools::ToolId::Scale)) {
		return;
	}

	const ViewportPixelSize kSize = last_tool_pointer_point_.has_value()
			? last_tool_pointer_size_
			: surface_size_;
	ViewportPoint point = last_tool_pointer_point_.value_or(ViewportPoint{
			static_cast<double>(std::max(1, kSize.width)) * 0.5,
			static_cast<double>(std::max(1, kSize.height)) * 0.5,
	});
	const auto kViewIndex = pane_array_index_at(layout_, kSize, point);
	const ViewportPaneArray kPanes = layout_.panes_for(kSize);
	if (!kViewIndex.has_value()) {
		const int kCameraIndex = cameras_.active_camera_index();
		for (int index = 0; index < layout_.pane_count(); ++index) {
			const ViewportPane &pane = kPanes[static_cast<std::size_t>(index)];
			if (pane.camera_index != kCameraIndex) {
				continue;
			}
			point = {
				static_cast<double>(pane.rect.x) + static_cast<double>(pane.rect.width) * 0.5,
				static_cast<double>(pane.rect.y) + static_cast<double>(pane.rect.height) * 0.5,
			};
			break;
		}
	}

	const auto kRefreshViewIndex = pane_array_index_at(layout_, kSize, point);
	if (!kRefreshViewIndex.has_value()) {
		return;
	}

	const ViewportPane &pane = kPanes[*kRefreshViewIndex];
	const quader::math::Vec2 kLocalPosition{
		static_cast<float>(point.x - pane.rect.x),
		static_cast<float>(point.y - pane.rect.y),
	};
	const auto kRay = ray_for_point(point, kSize);
	const ViewportCameraSnapshotArray kSnapshots = cameras_.camera_snapshots();
	const ViewportCameraSnapshot &camera = kSnapshots[static_cast<std::size_t>(std::clamp(pane.camera_index, 0, 3))];
	quader::tools::PointerEvent event{
		.position = kLocalPosition,
		.phase = quader::tools::PointerPhase::Hover,
		.snap_to_grid = true,
		.grid_size = 1.0F,
		.ray = kRay,
		.camera = quader::tools::ViewportCameraInput{
				.eye = camera.eye,
				.target = camera.target,
				.up = camera.up,
				.forward = camera.forward,
				.projection = camera.projection == CameraProjection::Orthographic
						? quader::tools::ViewportCameraProjection::Orthographic
						: quader::tools::ViewportCameraProjection::Perspective,
				.fov_degrees = camera.settings.fov_degrees,
				.orthographic_size = camera.settings.orthographic_size,
				.viewport_size_pixels = {
						static_cast<float>(std::max(1, pane.rect.width)),
						static_cast<float>(std::max(1, pane.rect.height)),
				},
		},
		.view_index = static_cast<std::uint32_t>(*kRefreshViewIndex),
	};
	(void)tool_manager_->dispatch_pointer_event(event);
}

ViewportPixelSize ViewportController::safe_size(ViewportPixelSize size) const noexcept {
	return ViewportPixelSize{
		.width = std::max(1, size.width),
		.height = std::max(1, size.height),
	};
}

ViewportPixelSize ViewportController::pane_size_for_camera(ViewportPixelSize size, int camera_index) const {
	const ViewportPaneArray kPanes = layout_.panes_for(size);
	for (int index = 0; index < layout_.pane_count(); ++index) {
		const ViewportPane &pane = kPanes[static_cast<std::size_t>(index)];
		if (pane.camera_index == camera_index) {
			return ViewportPixelSize{
				.width = std::max(1, pane.rect.width),
				.height = std::max(1, pane.rect.height),
			};
		}
	}

	return safe_size(size);
}

void ViewportController::emit_result_error(const ViewportRenderResult &result) {
	Q_EMIT render_error(result.summary, result.detail);
}

} // namespace quader::ui
