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

#include "tools/editor_input_event.hpp"
#include "tools/tool_id.hpp"
#include "ui/viewport/viewport_camera_controller.hpp"
#include "ui/viewport/viewport_layout_state.hpp"
#include "ui/viewport/viewport_render_host.hpp"

#include <array>
#include <optional>

#include <QObject>
#include <QString>

namespace quader::ui {

} // namespace quader::ui

namespace quader::tools {
class ToolManager;
}

namespace quader::ui {

/// Coordinates viewport layout, navigation, tool input, and render-host calls.
class ViewportController final : public QObject {
	Q_OBJECT

public:
	/// Construct a viewport controller without tool dispatch.
	explicit ViewportController(IViewportRenderHost &render_host, QObject *parent = nullptr);
	/// Construct a viewport controller with tool dispatch.
	ViewportController(IViewportRenderHost &render_host,
			quader::tools::ToolManager &tool_manager,
			QObject *parent = nullptr);
	/// Destroy the controller; the render host remains externally owned.
	~ViewportController() override = default;

	/// Return true when the render surface is initialized.
	[[nodiscard]] bool surface_initialized() const noexcept;
	/// Return current viewport navigation mode.
	[[nodiscard]] NavigationMode navigation_mode() const noexcept;

	/// Initialize the render surface and request a frame on success.
	void initialize_surface(const NativeViewportSurface &surface, ViewportPixelSize size, double device_pixel_ratio);
	/// Resize the render surface.
	void resize_surface(ViewportPixelSize size, double device_pixel_ratio);
	/// Shut down the render surface if initialized.
	void shutdown_surface();

	/// Enable or disable quad viewport layout.
	void set_quad_viewports_enabled(bool enabled);
	/// Return true when quad viewport layout is active.
	[[nodiscard]] bool quad_viewports_enabled() const noexcept;

	/// Enable or disable scene animation in render requests.
	void set_scene_animation_enabled(bool enabled);
	/// Return true when scene animation is enabled.
	[[nodiscard]] bool scene_animation_enabled() const noexcept;
	/// Set the viewport mesh surface shading mode.
	void set_shading_mode(ViewportShadingMode mode);
	/// Return the viewport mesh surface shading mode.
	[[nodiscard]] ViewportShadingMode shading_mode() const noexcept;
	/// Set viewport display settings used by render requests.
	void set_display_settings(ViewportDisplaySettings settings);
	/// Return current viewport display settings.
	[[nodiscard]] ViewportDisplaySettings display_settings() const noexcept;

	/// Submit one frame to the render host.
	void render_frame(double elapsed_seconds, float delta_seconds);
	/// Request that the viewport widget schedules another frame.
	void request_frame();

	/// Return mutable layout state.
	[[nodiscard]] ViewportLayoutState &layout_state() noexcept;
	/// Return immutable layout state.
	[[nodiscard]] const ViewportLayoutState &layout_state() const noexcept;
	/// Return immutable camera controller state.
	[[nodiscard]] const ViewportCameraController &camera_controller() const noexcept;

	/// Handle a mouse press and dispatch navigation/tool input.
	[[nodiscard]] bool handle_mouse_press(ViewportMouseButton button,
			ViewportPoint point,
			bool shift_modifier,
			ViewportPixelSize size,
			bool control_modifier = false,
			bool alt_modifier = false);
	/// Handle a mouse move and dispatch navigation/tool input.
	[[nodiscard]] bool handle_mouse_move(ViewportPoint point,
			ViewportPixelSize size,
			bool left_button_pressed = false,
			bool shift_modifier = false,
			bool control_modifier = false,
			bool alt_modifier = false);
	/// Handle a mouse release and dispatch navigation/tool input.
	[[nodiscard]] bool handle_mouse_release(ViewportMouseButton button,
			ViewportPoint point,
			ViewportPixelSize size,
			bool shift_modifier = false,
			bool control_modifier = false,
			bool alt_modifier = false);
	/// Clear transient hover/tool state when the pointer leaves the viewport.
	void handle_mouse_leave();
	/// Handle wheel input for zoom/fly speed.
	void handle_wheel(float wheel_steps, ViewportPoint point, ViewportPixelSize size);
	/// Handle key input for navigation and tools.
	[[nodiscard]] bool handle_key(ViewportKey key, bool pressed, bool auto_repeat);
	/// Return true when fly navigation should consume a Qt action shortcut first.
	[[nodiscard]] bool overrides_action_shortcut(ViewportKey key) const noexcept;

Q_SIGNALS:
	/// Emitted after the renderer reports readiness.
	void renderer_ready(QString renderer_name);
	/// Emitted when viewport layout mode changes.
	void viewport_layout_changed(bool quad_enabled, QString layout_name);
	/// Emitted after frame stats change.
	void frame_stats_changed(quader::ui::ViewportFrameStats stats);
	/// Emitted when render-host calls fail.
	void render_error(QString summary, QString detail);
	/// Emitted when another frame should be scheduled.
	void frame_requested();

private:
	[[nodiscard]] ViewportPixelSize safe_size(ViewportPixelSize size) const noexcept;
	[[nodiscard]] ViewportPixelSize pane_size_for_camera(ViewportPixelSize size, int camera_index) const;
	[[nodiscard]] std::optional<quader::tools::ViewportRay> ray_for_point(ViewportPoint point, ViewportPixelSize size) const;
	[[nodiscard]] std::optional<quader::tools::SurfaceHit> surface_hit_for_ray(
			const quader::tools::ViewportRay &ray,
			ViewportPoint point,
			ViewportPixelSize size) const;
	[[nodiscard]] bool dispatch_tool_pointer(ViewportMouseButton button,
			ViewportPoint point,
			ViewportPixelSize size,
			quader::tools::PointerPhase phase,
			bool pressed,
			quader::tools::KeyboardModifiers modifiers = {});
	void remember_tool_pointer(ViewportPoint point, ViewportPixelSize size, bool left_pressed) noexcept;
	void refresh_transform_tool_hover();
	void emit_result_error(const ViewportRenderResult &result);

	IViewportRenderHost &render_host_;
	quader::tools::ToolManager *tool_manager_ = nullptr;
	ViewportLayoutState layout_;
	ViewportCameraController cameras_;
	ViewportPixelSize surface_size_;
	std::optional<ViewportPoint> last_tool_pointer_point_;
	ViewportPixelSize last_tool_pointer_size_;
	bool tool_pointer_left_pressed_ = false;
	double device_pixel_ratio_ = 1.0;
	ViewportShadingMode shading_mode_ = ViewportShadingMode::Shaded;
	ViewportDisplaySettings display_settings_;
	bool scene_animation_enabled_ = true;
	bool surface_initialized_ = false;
};

} // namespace quader::ui
