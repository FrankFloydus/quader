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
#include <vector>

namespace quader::ui {
namespace {

constexpr float kRayEpsilon = 0.000001F;
constexpr float kPi = 3.14159265358979323846F;

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

void ViewportController::set_prototype_animation_enabled(bool enabled) {
	if (prototype_animation_enabled_ == enabled) {
		return;
	}

	prototype_animation_enabled_ = enabled;
	request_frame();
}

bool ViewportController::prototype_animation_enabled() const noexcept {
	return prototype_animation_enabled_;
}

void ViewportController::render_frame(double elapsed_seconds, float delta_seconds) {
	if (!surface_initialized_) {
		return;
	}

	cameras_.tick_fly_navigation(delta_seconds);

	const ViewportPaneArray kPanes = layout_.panes_for(surface_size_);
	const ViewportCameraSnapshotArray kCameras = cameras_.camera_snapshots();
	const int kPaneCount = layout_.pane_count();
	const ViewportRenderRequest kRequest{
		.surface_size = surface_size_,
		.device_pixel_ratio = device_pixel_ratio_,
		.layout_mode = layout_.mode(),
		.panes = std::span<const ViewportPane>(kPanes.data(), static_cast<std::size_t>(kPaneCount)),
		.cameras = std::span<const ViewportCameraSnapshot>(kCameras.data(), kCameras.size()),
		.prototype_animation_enabled = prototype_animation_enabled_,
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
		ViewportPixelSize size) {
	const auto kPaneIndex = pane_array_index_at(layout_, size, point);
	if (kPaneIndex.has_value()) {
		const ViewportPaneArray kPanes = layout_.panes_for(size);
		cameras_.set_active_camera_index(kPanes[*kPaneIndex].camera_index);
	}

	if (button == ViewportMouseButton::Middle) {
		const NavigationMode kMode = shift_modifier ? NavigationMode::Pan : NavigationMode::Orbit;
		if (cameras_.begin_navigation(cameras_.active_camera_index(), kMode, point)) {
			request_frame();
			return true;
		}
	} else if (button == ViewportMouseButton::Right) {
		const NavigationMode kMode = cameras_.camera_projection(cameras_.active_camera_index()) == CameraProjection::Orthographic
				? NavigationMode::Pan
				: NavigationMode::Fly;
		if (cameras_.begin_navigation(cameras_.active_camera_index(), kMode, point)) {
			request_frame();
			return true;
		}
	}

	if (button == ViewportMouseButton::Left) {
		return dispatch_tool_pointer(button, point, size, quader::tools::PointerPhase::Press, true);
	}

	return false;
}

bool ViewportController::handle_mouse_move(ViewportPoint point, ViewportPixelSize size) {
	if (cameras_.navigation_mode() != NavigationMode::None) {
		cameras_.update_navigation(point, pane_size_for_camera(size, cameras_.active_camera_index()));
		request_frame();
		return true;
	}

	return dispatch_tool_pointer(
			ViewportMouseButton::Other,
			point,
			size,
			quader::tools::PointerPhase::Move,
			false);
}

bool ViewportController::handle_mouse_release(ViewportMouseButton button,
		ViewportPoint point,
		ViewportPixelSize size) {
	if (cameras_.navigation_mode() != NavigationMode::None) {
		cameras_.end_navigation();
		request_frame();
		return true;
	}

	if (button == ViewportMouseButton::Left) {
		return dispatch_tool_pointer(button, point, size, quader::tools::PointerPhase::Release, false);
	}

	return false;
}

void ViewportController::handle_wheel(float wheel_steps, ViewportPoint point, ViewportPixelSize size) {
	if (std::abs(wheel_steps) <= 0.000001F) {
		return;
	}

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
		.vertical_fov_degrees = camera.fov_degrees,
		.orthographic_height_m = camera.orthographic_size,
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
		const quader::tools::ViewportRay &ray) const {
	if (tool_manager_ == nullptr) {
		return std::nullopt;
	}

	const auto &document = tool_manager_->context().document();
	float best_distance = std::numeric_limits<float>::infinity();
	std::optional<quader::tools::SurfaceHit> best_hit;
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

			const quader::math::Vec3 kNormal = normalized_or(quader::geometry::polygon_normal(points),
					{ 0.0F, 1.0F, 0.0F });
			for (std::size_t index = 1U; index + 1U < points.size(); ++index) {
				const RayTriangleHit kHit = intersect_triangle(ray, points[0], points[index], points[index + 1U]);
				if (!kHit.hit || kHit.distance >= best_distance) {
					continue;
				}

				best_distance = kHit.distance;
				best_hit = quader::tools::SurfaceHit{
					.position = kHit.position,
					.normal = kNormal,
					.object_id = (static_cast<std::uint64_t>(kObjectId.generation()) << 32U) | kObjectId.index(),
					.component_index = kFaceId.index(),
					.kind = quader::tools::SurfaceHitKind::Face,
				};
			}
		}
	}

	return best_hit;
}

bool ViewportController::dispatch_tool_pointer(ViewportMouseButton button,
		ViewportPoint point,
		ViewportPixelSize size,
		quader::tools::PointerPhase phase,
		bool pressed) {
	if (tool_manager_ == nullptr || tool_manager_->active_tool() == nullptr) {
		return false;
	}

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
	quader::tools::PointerEvent event{
		.position = kLocalPosition,
		.button = button == ViewportMouseButton::Left ? quader::tools::PointerButton::Left
				: button == ViewportMouseButton::Middle
				? quader::tools::PointerButton::Middle
				: button == ViewportMouseButton::Right ? quader::tools::PointerButton::Right
													   : quader::tools::PointerButton::None,
		.pressed = pressed,
		.phase = phase,
		.navigation_active = cameras_.navigation_mode() != NavigationMode::None,
		.snap_to_grid = true,
		.grid_size = 1.0F,
		.ray = kRay,
		.view_index = static_cast<std::uint32_t>(*kViewIndex),
	};
	if (kRay.has_value()) {
		event.surface_hit = surface_hit_for_ray(*kRay);
	}

	const bool kHandled = tool_manager_->dispatch_pointer_event(event);
	if (kHandled) {
		request_frame();
	}
	return kHandled;
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
