#include "ui/viewport/viewport_controller.hpp"

#include <algorithm>
#include <cmath>

namespace quader::ui {

ViewportController::ViewportController(IViewportRenderHost &render_host, QObject *parent) : QObject(parent), render_host_(render_host) {
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

void ViewportController::handle_mouse_press(
		ViewportMouseButton button,
		ViewportPoint point,
		bool shift_modifier,
		ViewportPixelSize size) {
	const int kPaneIndex = layout_.pane_at(size, point);
	if (kPaneIndex >= 0) {
		cameras_.set_active_camera_index(kPaneIndex);
	}

	if (button == ViewportMouseButton::Middle) {
		const NavigationMode kMode = shift_modifier ? NavigationMode::Pan : NavigationMode::Orbit;
		if (cameras_.begin_navigation(cameras_.active_camera_index(), kMode, point)) {
			request_frame();
		}
	} else if (button == ViewportMouseButton::Right) {
		const NavigationMode kMode = cameras_.camera_projection(cameras_.active_camera_index()) == CameraProjection::Orthographic
				? NavigationMode::Pan
				: NavigationMode::Fly;
		if (cameras_.begin_navigation(cameras_.active_camera_index(), kMode, point)) {
			request_frame();
		}
	}
}

void ViewportController::handle_mouse_move(ViewportPoint point, ViewportPixelSize size) {
	if (cameras_.navigation_mode() == NavigationMode::None) {
		return;
	}

	cameras_.update_navigation(point, pane_size_for_camera(size, cameras_.active_camera_index()));
	request_frame();
}

void ViewportController::handle_mouse_release(ViewportMouseButton) {
	if (cameras_.navigation_mode() == NavigationMode::None) {
		return;
	}

	cameras_.end_navigation();
	request_frame();
}

void ViewportController::handle_wheel(float wheel_steps, ViewportPoint point, ViewportPixelSize size) {
	if (std::abs(wheel_steps) <= 0.000001F) {
		return;
	}

	if (cameras_.navigation_mode() == NavigationMode::Fly) {
		cameras_.adjust_fly_speed(wheel_steps);
		return;
	}

	const int kPaneIndex = layout_.pane_at(size, point);
	const int kCameraIndex = kPaneIndex >= 0 ? kPaneIndex : cameras_.active_camera_index();
	cameras_.apply_wheel_zoom(kCameraIndex, wheel_steps, pane_size_for_camera(size, kCameraIndex));
	request_frame();
}

void ViewportController::handle_key(ViewportKey key, bool pressed, bool auto_repeat) {
	if (auto_repeat || cameras_.navigation_mode() != NavigationMode::Fly) {
		return;
	}

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
		case ViewportKey::Other:
			break;
	}
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
