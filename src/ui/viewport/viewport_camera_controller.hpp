#pragma once

#include "ui/viewport/viewport_types.hpp"

#include "math/vec3.hpp"

#include <array>

namespace quader::ui {

enum class ViewportFlyKey {
	Forward,
	Backward,
	Left,
	Right,
};

struct ViewportCameraState {
	quader::math::Vec3 target{};
	quader::math::Vec3 pivot{};
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	float yaw_radians = -0.6435011F;
	float pitch_radians = -0.4636476F;
	float distance = 11.18034F;
	CameraProjection projection = CameraProjection::Perspective;
	float orthographic_size = 24.0F;
	float fov_degrees = 60.0F;
	quader::math::Vec3 locked_forward{ 0.0F, 0.0F, -1.0F };
	quader::math::Vec3 locked_up{ 0.0F, 1.0F, 0.0F };
	bool locked_orthographic_view = false;
};

struct ViewportCameraPose {
	quader::math::Vec3 eye{};
	quader::math::Vec3 target{};
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
};

class ViewportCameraController final {
public:
	ViewportCameraController();

	void reset_default_cameras();
	[[nodiscard]] const ViewportCameraState &camera(int camera_index) const;
	[[nodiscard]] CameraProjection camera_projection(int camera_index) const;

	[[nodiscard]] NavigationMode navigation_mode() const noexcept;
	[[nodiscard]] int active_camera_index() const noexcept;
	void set_active_camera_index(int camera_index) noexcept;

	[[nodiscard]] bool begin_navigation(int camera_index, NavigationMode mode, ViewportPoint position);
	void update_navigation(ViewportPoint position, ViewportPixelSize viewport_size);
	void end_navigation();
	void set_navigation_anchor(ViewportPoint position) noexcept;

	void apply_wheel_zoom(int camera_index, float wheel_steps, ViewportPixelSize viewport_size);
	void adjust_fly_speed(float wheel_steps) noexcept;
	void set_fly_key(ViewportFlyKey key, bool pressed) noexcept;
	void clear_fly_keys() noexcept;
	void tick_fly_navigation(float delta_seconds);

	[[nodiscard]] ViewportCameraPose camera_pose(int camera_index) const;
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
