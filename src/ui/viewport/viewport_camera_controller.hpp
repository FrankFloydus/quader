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

#include "ui/viewport/viewport_types.hpp"

#include "math/vec3.hpp"

#include <array>

namespace quader::ui {

/// Fly-navigation movement key.
enum class ViewportFlyKey {
	Forward,  ///< Move forward.
	Backward, ///< Move backward.
	Left,     ///< Strafe left.
	Right,    ///< Strafe right.
};

/// Mutable camera state for one viewport pane.
struct ViewportCameraState {
	/// Camera target point.
	quader::math::Vec3 target{};
	/// Navigation pivot point.
	quader::math::Vec3 pivot{};
	/// Camera forward vector.
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
	/// Camera up vector.
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	/// Orbit yaw angle in radians.
	float yaw_radians = -0.6435011F;
	/// Orbit pitch angle in radians.
	float pitch_radians = -0.4636476F;
	/// Orbit distance from target.
	float distance = 11.18034F;
	/// Projection mode.
	CameraProjection projection = CameraProjection::Perspective;
	/// User-facing camera properties.
	ViewportCameraSettings settings;
	/// Forward vector used by locked orthographic views.
	quader::math::Vec3 locked_forward{ 0.0F, 0.0F, -1.0F };
	/// Up vector used by locked orthographic views.
	quader::math::Vec3 locked_up{ 0.0F, 1.0F, 0.0F };
	/// True when camera orientation is locked to an orthographic view.
	bool locked_orthographic_view = false;
};

/// Derived camera pose used by viewport rendering and ray reconstruction.
struct ViewportCameraPose {
	/// Eye position.
	quader::math::Vec3 eye{};
	/// Target position.
	quader::math::Vec3 target{};
	/// Up vector.
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	/// Forward vector.
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
};

/// Manages perspective and orthographic viewport cameras and navigation input.
class ViewportCameraController final {
public:
	/// Construct default cameras.
	ViewportCameraController();

	/// Reset all cameras to default perspective/top/front/right states.
	void reset_default_cameras();
	/// Return camera state by index.
	[[nodiscard]] const ViewportCameraState &camera(int camera_index) const;
	/// Return projection mode for a camera.
	[[nodiscard]] CameraProjection camera_projection(int camera_index) const;
	/// Return sanitized settings for a camera.
	[[nodiscard]] ViewportCameraSettings camera_settings(int camera_index) const;
	/// Set user-facing settings for a camera.
	void set_camera_settings(int camera_index, ViewportCameraSettings settings);
	/// Set the same clip range on every viewport camera.
	void set_clip_range(ViewportCameraClipRange clip_range) noexcept;

	/// Return current navigation mode.
	[[nodiscard]] NavigationMode navigation_mode() const noexcept;
	/// Return active camera index.
	[[nodiscard]] int active_camera_index() const noexcept;
	/// Set active camera index, clamped to supported cameras.
	void set_active_camera_index(int camera_index) noexcept;

	/// Begin a navigation gesture for a camera.
	[[nodiscard]] bool begin_navigation(int camera_index, NavigationMode mode, ViewportPoint position);
	/// Update the active navigation gesture.
	void update_navigation(ViewportPoint position, ViewportPixelSize viewport_size);
	/// End the active navigation gesture.
	void end_navigation();
	/// Reset the navigation anchor without changing mode.
	void set_navigation_anchor(ViewportPoint position) noexcept;

	/// Apply wheel zoom to a camera.
	void apply_wheel_zoom(int camera_index, float wheel_steps, ViewportPixelSize viewport_size);
	/// Adjust fly-navigation speed.
	void adjust_fly_speed(float wheel_steps) noexcept;
	/// Set pressed state for one fly key.
	void set_fly_key(ViewportFlyKey key, bool pressed) noexcept;
	/// Clear all fly-key pressed states.
	void clear_fly_keys() noexcept;
	/// Advance fly navigation by elapsed time.
	void tick_fly_navigation(float delta_seconds);

	/// Return the derived camera pose by index.
	[[nodiscard]] ViewportCameraPose camera_pose(int camera_index) const;
	/// Return render-facing snapshots for all supported cameras.
	[[nodiscard]] ViewportCameraSnapshotArray camera_snapshots() const;

private:
	[[nodiscard]] ViewportPixelSize safe_viewport_size(ViewportPixelSize viewport_size) const noexcept;
	[[nodiscard]] ViewportCameraState &active_camera();

	std::array<ViewportCameraState, 4> cameras_{};
	NavigationMode navigation_mode_ = NavigationMode::None;
	ViewportPoint last_navigation_mouse_;
	int active_camera_index_ = 0;
	float fly_speed_ = 6.0F;
	bool fly_forward_ = false;
	bool fly_backward_ = false;
	bool fly_left_ = false;
	bool fly_right_ = false;
};

} // namespace quader::ui
