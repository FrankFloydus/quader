#include "ui/viewport/viewport_camera_controller.hpp"

#include <algorithm>
#include <cmath>

namespace quader::ui {
namespace {

using quader::math::cross;
using quader::math::dot;
using quader::math::length;
using quader::math::normalized;
using quader::math::Vec3;

constexpr float kHalfPi = 1.57079632679F;
constexpr float kDefaultFovDegrees = 60.0F;
constexpr float kDefaultOrthographicSize = 24.0F;
constexpr float kDefaultCameraDistance = 11.18034F;
constexpr float kDefaultCameraYaw = -0.6435011F;
constexpr float kDefaultCameraPitch = -0.4636476F;
constexpr float kOrbitRadiansPerPixel = 0.006F;
constexpr float kFlyLookRadiansPerPixel = 0.004F;
constexpr float kNavigationZoomStepFactor = 1.18F;
constexpr float kFlySpeedWheelStep = 1.0F;
constexpr float kFlyMinSpeed = 1.0F;
constexpr float kFlyMaxSpeed = 1000.0F;
constexpr float kMinCameraDistance = 0.05F;
constexpr float kMaxCameraDistance = 500.0F;
constexpr float kViewportGridDefaultPlaneSize = 8192.0F;

[[nodiscard]] Vec3 forward_from_angles(float yaw_radians, float pitch_radians) {
	const float kCp = std::cos(pitch_radians);
	return normalized(Vec3{
			std::sin(yaw_radians) * kCp,
			std::sin(pitch_radians),
			-std::cos(yaw_radians) * kCp,
	});
}

[[nodiscard]] Vec3 fallback_up_for_forward(Vec3 forward) {
	if (std::abs(forward.y) > 0.999F) {
		return forward.y < 0.0F ? Vec3{ 0.0F, 0.0F, 1.0F } : Vec3{ 0.0F, 0.0F, -1.0F };
	}
	return { 0.0F, 1.0F, 0.0F };
}

struct CameraBasis {
	Vec3 forward{ 0.0F, 0.0F, -1.0F };
	Vec3 up{ 0.0F, 1.0F, 0.0F };
};

struct CameraTransform {
	Vec3 x_axis{ 1.0F, 0.0F, 0.0F };
	Vec3 y_axis{ 0.0F, 1.0F, 0.0F };
	Vec3 z_axis{ 0.0F, 0.0F, 1.0F };
	Vec3 origin{};
};

[[nodiscard]] CameraBasis make_camera_basis(
		Vec3 forward_direction,
		Vec3 up_direction,
		Vec3 fallback_forward = { 0.0F, 0.0F, -1.0F }) {
	Vec3 forward = normalized(forward_direction);
	if (length(forward) <= 0.000001F) {
		forward = normalized(fallback_forward);
	}
	if (length(forward) <= 0.000001F) {
		forward = { 0.0F, 0.0F, -1.0F };
	}

	Vec3 candidate_up = normalized(up_direction);
	if (length(candidate_up) <= 0.000001F || std::abs(dot(candidate_up, forward)) > 0.999F) {
		candidate_up = fallback_up_for_forward(forward);
	}

	Vec3 right = normalized(cross(forward, candidate_up));
	if (length(right) <= 0.000001F) {
		right = { 1.0F, 0.0F, 0.0F };
	}

	Vec3 up = normalized(cross(right, forward));
	if (length(up) <= 0.000001F) {
		up = fallback_up_for_forward(forward);
	}

	return { forward, up };
}

void sync_angles_for_forward(ViewportCameraState &camera, Vec3 forward) {
	const Vec3 kNormalizedForward = normalized(forward);
	if (length(kNormalizedForward) <= 0.000001F) {
		return;
	}

	camera.pitch_radians = std::asin(std::clamp(kNormalizedForward.y, -1.0F, 1.0F));
	camera.yaw_radians = std::atan2(kNormalizedForward.x, -kNormalizedForward.z);
}

[[nodiscard]] Vec3 transform_vector(const CameraTransform &transform, Vec3 value) {
	return transform.x_axis * value.x + transform.y_axis * value.y + transform.z_axis * value.z;
}

[[nodiscard]] Vec3 rotate_vector_around_axis(Vec3 value, Vec3 axis, float angle_radians) {
	const Vec3 kNormal = normalized(axis);
	if (length(kNormal) <= 0.000001F) {
		return value;
	}

	const float kC = std::cos(angle_radians);
	const float kS = std::sin(angle_radians);
	const float kOneMinusC = 1.0F - kC;
	return value * kC + cross(kNormal, value) * kS + kNormal * (dot(kNormal, value) * kOneMinusC);
}

[[nodiscard]] CameraTransform orthonormalized_basis(CameraTransform basis) {
	Vec3 right = normalized(basis.x_axis);
	if (length(right) <= 0.000001F) {
		right = { 1.0F, 0.0F, 0.0F };
	}

	Vec3 up = basis.y_axis - right * dot(basis.y_axis, right);
	up = normalized(up);
	if (length(up) <= 0.000001F) {
		const Vec3 kFallback = std::abs(right.y) > 0.9F ? Vec3{ 0.0F, 0.0F, 1.0F } : Vec3{ 0.0F, 1.0F, 0.0F };
		up = normalized(kFallback - right * dot(kFallback, right));
	}

	Vec3 back = normalized(cross(right, up));
	if (length(back) <= 0.000001F) {
		back = { 0.0F, 0.0F, 1.0F };
	}

	up = normalized(cross(back, right));
	return { right, up, back, basis.origin };
}

[[nodiscard]] CameraTransform rotation_basis(Vec3 axis, float angle_radians) {
	const Vec3 kNormal = normalized(axis);
	if (length(kNormal) <= 0.000001F) {
		return {};
	}

	return {
		rotate_vector_around_axis({ 1.0F, 0.0F, 0.0F }, kNormal, angle_radians),
		rotate_vector_around_axis({ 0.0F, 1.0F, 0.0F }, kNormal, angle_radians),
		rotate_vector_around_axis({ 0.0F, 0.0F, 1.0F }, kNormal, angle_radians),
		{},
	};
}

[[nodiscard]] CameraTransform basis_multiply(const CameraTransform &left, const CameraTransform &right) {
	return {
		transform_vector(left, right.x_axis),
		transform_vector(left, right.y_axis),
		transform_vector(left, right.z_axis),
		{},
	};
}

[[nodiscard]] CameraTransform rotated_camera_basis(
		const CameraTransform &camera_transform,
		const CameraTransform &rotation) {
	return orthonormalized_basis({
			transform_vector(rotation, camera_transform.x_axis),
			transform_vector(rotation, camera_transform.y_axis),
			transform_vector(rotation, camera_transform.z_axis),
			camera_transform.origin,
	});
}

[[nodiscard]] CameraTransform clamped_rotation_to_upright(
		const CameraTransform &camera_transform,
		const CameraTransform &rotation) {
	const Vec3 kWorldUp{ 0.0F, 1.0F, 0.0F };
	const Vec3 kNewUp = normalized(transform_vector(rotation, camera_transform.y_axis));
	if (dot(kNewUp, kWorldUp) >= 0.0F) {
		return orthonormalized_basis(rotation);
	}

	const Vec3 kNewRight = normalized(transform_vector(rotation, camera_transform.x_axis));
	const Vec3 kNewForward = normalized(transform_vector(rotation, camera_transform.z_axis * -1.0F));
	Vec3 clamped_up = kNewUp - kWorldUp * dot(kNewUp, kWorldUp);
	clamped_up = length(clamped_up) <= 0.000001F ? Vec3{ 1.0F, 0.0F, 0.0F } : normalized(clamped_up);

	const float kCosAngle = std::clamp(dot(clamped_up, kNewUp), -1.0F, 1.0F);
	const float kCorrectionAngle = std::acos(kCosAngle);
	const float kCorrectionSign = dot(kNewForward, kWorldUp) > 0.0F ? -1.0F : 1.0F;
	const CameraTransform kCorrection = rotation_basis(kNewRight, kCorrectionAngle * kCorrectionSign);
	return orthonormalized_basis(basis_multiply(kCorrection, rotation));
}

[[nodiscard]] Vec3 viewport_camera_forward(const CameraTransform &camera_transform) {
	const Vec3 kForward = normalized(camera_transform.z_axis * -1.0F);
	return length(kForward) <= 0.000001F ? Vec3{ 0.0F, 0.0F, -1.0F } : kForward;
}

[[nodiscard]] Vec3 viewport_camera_right(const CameraTransform &camera_transform) {
	const Vec3 kRight = normalized(camera_transform.x_axis);
	return length(kRight) <= 0.000001F ? Vec3{ 1.0F, 0.0F, 0.0F } : kRight;
}

[[nodiscard]] Vec3 viewport_camera_up(const CameraTransform &camera_transform) {
	const Vec3 kUp = normalized(camera_transform.y_axis);
	return length(kUp) <= 0.000001F ? Vec3{ 0.0F, 1.0F, 0.0F } : kUp;
}

[[nodiscard]] float viewport_distance_to_pivot(const CameraTransform &camera_transform, Vec3 pivot) {
	const Vec3 kToPivot = pivot - camera_transform.origin;
	const float kForwardDistance = dot(kToPivot, viewport_camera_forward(camera_transform));
	if (kForwardDistance > kMinCameraDistance) {
		return kForwardDistance;
	}

	return std::max(kMinCameraDistance, length(kToPivot));
}

[[nodiscard]] CameraTransform transform_from_pose(const ViewportCameraPose &pose) {
	const CameraBasis kBasis = make_camera_basis(pose.target - pose.eye, pose.up);
	Vec3 right = normalized(cross(kBasis.forward, kBasis.up));
	if (length(right) <= 0.000001F) {
		right = { 1.0F, 0.0F, 0.0F };
	}

	const Vec3 kUp = normalized(cross(right, kBasis.forward));
	return {
		right,
		length(kUp) <= 0.000001F ? kBasis.up : kUp,
		kBasis.forward * -1.0F,
		pose.eye,
	};
}

void apply_navigation_transform(
		ViewportCameraState &camera,
		const CameraTransform &transform,
		Vec3 pivot,
		bool has_camera_size = false,
		float camera_size = 0.0F,
		bool preserve_view_distance = false) {
	const Vec3 kForward = viewport_camera_forward(transform);
	const CameraBasis kBasis = make_camera_basis(kForward, transform.y_axis, camera.forward);
	const float kRequestedDistance = preserve_view_distance
			? camera.distance
			: viewport_distance_to_pivot(transform, pivot);
	const float kViewDistance = std::clamp(kRequestedDistance, kMinCameraDistance, kMaxCameraDistance);

	camera.pivot = pivot;
	camera.forward = kBasis.forward;
	camera.up = kBasis.up;
	camera.target = transform.origin + camera.forward * kViewDistance;
	camera.distance = kViewDistance;
	if (has_camera_size) {
		camera.orthographic_size = std::max(0.0001F, camera_size);
	}
	sync_angles_for_forward(camera, camera.forward);
}

void apply_orbit_transform(ViewportCameraState &camera, const CameraTransform &transform, Vec3 pivot) {
	Vec3 forward = normalized(pivot - transform.origin);
	if (length(forward) <= 0.000001F) {
		forward = viewport_camera_forward(transform);
	}

	const float kRadius = std::clamp(length(pivot - transform.origin), kMinCameraDistance, kMaxCameraDistance);
	const CameraBasis kBasis = make_camera_basis(forward, transform.y_axis, camera.forward);

	camera.pivot = pivot;
	camera.forward = kBasis.forward;
	camera.up = kBasis.up;
	camera.target = pivot;
	camera.distance = kRadius;
	sync_angles_for_forward(camera, camera.forward);
}

[[nodiscard]] float capped_orthographic_zoom_size(float current_size, float factor, float steps) {
	const float kSafeCurrentSize = std::max(0.0001F, current_size);
	const float kRequestedSize = kSafeCurrentSize / factor;
	if (steps < 0.0F) {
		if (kSafeCurrentSize >= kViewportGridDefaultPlaneSize) {
			return kSafeCurrentSize;
		}
		return std::clamp(kRequestedSize, 0.0001F, kViewportGridDefaultPlaneSize);
	}

	return std::max(0.0001F, kRequestedSize);
}

[[nodiscard]] int clamped_camera_index(int camera_index) noexcept {
	return std::clamp(camera_index, 0, 3);
}

} // namespace

ViewportCameraController::ViewportCameraController() {
	reset_default_cameras();
}

void ViewportCameraController::reset_default_cameras() {
	for (ViewportCameraState &camera : cameras_) {
		camera.target = {};
		camera.pivot = {};
		camera.yaw_radians = kDefaultCameraYaw;
		camera.pitch_radians = kDefaultCameraPitch;
		camera.forward = forward_from_angles(camera.yaw_radians, camera.pitch_radians);
		camera.up = make_camera_basis(camera.forward, fallback_up_for_forward(camera.forward)).up;
		camera.distance = kDefaultCameraDistance;
		camera.projection = CameraProjection::Perspective;
		camera.orthographic_size = kDefaultOrthographicSize;
		camera.fov_degrees = kDefaultFovDegrees;
		camera.locked_forward = { 0.0F, 0.0F, -1.0F };
		camera.locked_up = { 0.0F, 1.0F, 0.0F };
		camera.locked_orthographic_view = false;
	}

	cameras_[1].projection = CameraProjection::Orthographic;
	cameras_[1].distance = 64.0F;
	cameras_[1].locked_forward = { 0.0F, -1.0F, 0.0F };
	cameras_[1].locked_up = { 0.0F, 0.0F, 1.0F };
	cameras_[1].forward = cameras_[1].locked_forward;
	cameras_[1].up = cameras_[1].locked_up;
	cameras_[1].locked_orthographic_view = true;

	cameras_[2].projection = CameraProjection::Orthographic;
	cameras_[2].distance = 64.0F;
	cameras_[2].locked_forward = { 0.0F, 0.0F, 1.0F };
	cameras_[2].locked_up = { 0.0F, 1.0F, 0.0F };
	cameras_[2].forward = cameras_[2].locked_forward;
	cameras_[2].up = cameras_[2].locked_up;
	cameras_[2].locked_orthographic_view = true;

	cameras_[3].projection = CameraProjection::Orthographic;
	cameras_[3].distance = 64.0F;
	cameras_[3].locked_forward = { -1.0F, 0.0F, 0.0F };
	cameras_[3].locked_up = { 0.0F, 1.0F, 0.0F };
	cameras_[3].forward = cameras_[3].locked_forward;
	cameras_[3].up = cameras_[3].locked_up;
	cameras_[3].locked_orthographic_view = true;
}

const ViewportCameraState &ViewportCameraController::camera(int camera_index) const {
	return cameras_[static_cast<std::size_t>(clamped_camera_index(camera_index))];
}

CameraProjection ViewportCameraController::camera_projection(int camera_index) const {
	return camera(camera_index).projection;
}

NavigationMode ViewportCameraController::navigation_mode() const noexcept {
	return navigation_mode_;
}

int ViewportCameraController::active_camera_index() const noexcept {
	return active_camera_index_;
}

void ViewportCameraController::set_active_camera_index(int camera_index) noexcept {
	active_camera_index_ = clamped_camera_index(camera_index);
}

bool ViewportCameraController::begin_navigation(int camera_index, NavigationMode mode, ViewportPoint position) {
	set_active_camera_index(camera_index);
	ViewportCameraState &camera = active_camera();
	if (mode == NavigationMode::Orbit && camera.locked_orthographic_view) {
		return false;
	}

	if (mode == NavigationMode::Orbit) {
		camera.pivot = camera.target;
	}

	navigation_mode_ = mode;
	last_navigation_mouse_ = position;

	if (mode == NavigationMode::Fly) {
		camera.projection = CameraProjection::Perspective;
		camera.locked_orthographic_view = false;
		camera.pivot = camera.target;
	}

	return true;
}

void ViewportCameraController::update_navigation(ViewportPoint position, ViewportPixelSize viewport_size) {
	if (navigation_mode_ == NavigationMode::None) {
		return;
	}

	ViewportCameraState &camera = active_camera();
	const ViewportPoint kDelta{ position.x - last_navigation_mouse_.x, position.y - last_navigation_mouse_.y };
	last_navigation_mouse_ = position;
	const CameraTransform kCameraTransform = transform_from_pose(camera_pose(active_camera_index_));

	if (navigation_mode_ == NavigationMode::Orbit) {
		if (camera.locked_orthographic_view) {
			return;
		}

		camera.projection = CameraProjection::Perspective;

		const float kYaw = static_cast<float>(kDelta.x) * kOrbitRadiansPerPixel;
		const float kPitch = -static_cast<float>(kDelta.y) * kOrbitRadiansPerPixel;
		const CameraTransform kYawRotation = rotation_basis({ 0.0F, 1.0F, 0.0F }, kYaw);
		Vec3 right = normalized(transform_vector(kYawRotation, viewport_camera_right(kCameraTransform)));
		if (length(right) <= 0.000001F) {
			right = { 1.0F, 0.0F, 0.0F };
		}
		const CameraTransform kPitchRotation = rotation_basis(right, kPitch);
		const CameraTransform kRotation = clamped_rotation_to_upright(
				kCameraTransform,
				basis_multiply(kPitchRotation, kYawRotation));

		CameraTransform result_transform = kCameraTransform;
		result_transform.origin = camera.pivot + transform_vector(kRotation, kCameraTransform.origin - camera.pivot);
		const CameraTransform kRotatedBasis = rotated_camera_basis(kCameraTransform, kRotation);
		result_transform.x_axis = kRotatedBasis.x_axis;
		result_transform.y_axis = kRotatedBasis.y_axis;
		result_transform.z_axis = kRotatedBasis.z_axis;
		apply_orbit_transform(camera, result_transform, camera.pivot);
	} else if (navigation_mode_ == NavigationMode::Pan) {
		const ViewportPixelSize kSafeSize = safe_viewport_size(viewport_size);
		const float kViewportHeight = static_cast<float>(kSafeSize.height);
		const float kVisibleHeight = camera.projection == CameraProjection::Orthographic
				? std::max(0.0001F, camera.orthographic_size)
				: std::max(0.0001F, 2.0F * viewport_distance_to_pivot(kCameraTransform, camera.pivot) * std::tan(camera.fov_degrees * 3.14159265359F / 180.0F * 0.5F));
		const float kWorldPerPixel = kVisibleHeight / kViewportHeight;
		const Vec3 kMove = viewport_camera_right(kCameraTransform) * (static_cast<float>(kDelta.x) * kWorldPerPixel) + viewport_camera_up(kCameraTransform) * (static_cast<float>(kDelta.y) * kWorldPerPixel);
		CameraTransform result_transform = kCameraTransform;
		result_transform.origin = result_transform.origin + kMove;
		apply_navigation_transform(camera, result_transform, camera.pivot + kMove);
	} else if (navigation_mode_ == NavigationMode::Fly) {
		const float kYaw = static_cast<float>(kDelta.x) * kFlyLookRadiansPerPixel;
		const float kPitch = -static_cast<float>(kDelta.y) * kFlyLookRadiansPerPixel;
		const CameraTransform kYawRotation = rotation_basis({ 0.0F, 1.0F, 0.0F }, kYaw);
		Vec3 right = normalized(transform_vector(kYawRotation, viewport_camera_right(kCameraTransform)));
		if (length(right) <= 0.000001F) {
			right = { 1.0F, 0.0F, 0.0F };
		}
		const CameraTransform kPitchRotation = rotation_basis(right, kPitch);
		const CameraTransform kRotation = clamped_rotation_to_upright(
				kCameraTransform,
				basis_multiply(kPitchRotation, kYawRotation));
		const CameraTransform kRotatedBasis = rotated_camera_basis(kCameraTransform, kRotation);

		CameraTransform result_transform = kCameraTransform;
		result_transform.x_axis = kRotatedBasis.x_axis;
		result_transform.y_axis = kRotatedBasis.y_axis;
		result_transform.z_axis = kRotatedBasis.z_axis;
		apply_navigation_transform(camera, result_transform, camera.pivot, false, 0.0F, true);
		camera.pivot = camera.target;
		camera.projection = CameraProjection::Perspective;
		camera.locked_orthographic_view = false;
	}
}

void ViewportCameraController::end_navigation() {
	navigation_mode_ = NavigationMode::None;
	clear_fly_keys();
}

void ViewportCameraController::set_navigation_anchor(ViewportPoint position) noexcept {
	last_navigation_mouse_ = position;
}

void ViewportCameraController::apply_wheel_zoom(
		int camera_index,
		float wheel_steps,
		ViewportPixelSize viewport_size) {
	(void)viewport_size;
	set_active_camera_index(camera_index);

	ViewportCameraState &camera = active_camera();
	const float kZoomFactor = std::pow(kNavigationZoomStepFactor, wheel_steps);
	const CameraTransform kCameraTransform = transform_from_pose(camera_pose(active_camera_index_));
	if (camera.projection == CameraProjection::Orthographic) {
		const float kNewSize = capped_orthographic_zoom_size(camera.orthographic_size, kZoomFactor, wheel_steps);
		apply_navigation_transform(camera, kCameraTransform, camera.pivot, true, kNewSize);
	} else {
		const float kDistance = viewport_distance_to_pivot(kCameraTransform, camera.pivot);
		const float kNewDistance = std::max(kMinCameraDistance, kDistance / kZoomFactor);
		CameraTransform result_transform = kCameraTransform;
		result_transform.origin = result_transform.origin + viewport_camera_forward(kCameraTransform) * (kDistance - kNewDistance);
		apply_navigation_transform(camera, result_transform, camera.pivot);
	}
}

void ViewportCameraController::adjust_fly_speed(float wheel_steps) noexcept {
	fly_speed_ = std::clamp(fly_speed_ + wheel_steps * kFlySpeedWheelStep, kFlyMinSpeed, kFlyMaxSpeed);
}

void ViewportCameraController::set_fly_key(ViewportFlyKey key, bool pressed) noexcept {
	switch (key) {
		case ViewportFlyKey::Forward:
			fly_forward_ = pressed;
			break;
		case ViewportFlyKey::Backward:
			fly_backward_ = pressed;
			break;
		case ViewportFlyKey::Left:
			fly_left_ = pressed;
			break;
		case ViewportFlyKey::Right:
			fly_right_ = pressed;
			break;
	}
}

void ViewportCameraController::clear_fly_keys() noexcept {
	fly_forward_ = false;
	fly_backward_ = false;
	fly_left_ = false;
	fly_right_ = false;
}

void ViewportCameraController::tick_fly_navigation(float delta_seconds) {
	if (navigation_mode_ != NavigationMode::Fly || delta_seconds <= 0.0F) {
		return;
	}

	ViewportCameraState &camera = active_camera();
	Vec3 local_direction{};
	if (fly_forward_) {
		local_direction.z += 1.0F;
	}
	if (fly_backward_) {
		local_direction.z -= 1.0F;
	}
	if (fly_right_) {
		local_direction.x -= 1.0F;
	}
	if (fly_left_) {
		local_direction.x += 1.0F;
	}

	if (length(local_direction) <= 0.000001F) {
		return;
	}

	const CameraTransform kCameraTransform = transform_from_pose(camera_pose(active_camera_index_));
	Vec3 world_direction = viewport_camera_right(kCameraTransform) * local_direction.x + viewport_camera_up(kCameraTransform) * local_direction.y + viewport_camera_forward(kCameraTransform) * local_direction.z;
	world_direction = normalized(world_direction);
	if (length(world_direction) <= 0.000001F) {
		return;
	}

	const Vec3 kMove = world_direction * (fly_speed_ * std::clamp(delta_seconds, 0.0F, 0.25F));
	CameraTransform result_transform = kCameraTransform;
	result_transform.origin = result_transform.origin + kMove;
	apply_navigation_transform(camera, result_transform, camera.pivot + kMove, false, 0.0F, true);
	camera.pivot = camera.target;
}

ViewportCameraPose ViewportCameraController::camera_pose(int camera_index) const {
	const ViewportCameraState &camera = this->camera(camera_index);
	Vec3 forward = camera.projection == CameraProjection::Orthographic && camera.locked_orthographic_view
			? normalized(camera.locked_forward)
			: normalized(camera.forward);
	if (length(forward) <= 0.000001F) {
		forward = forward_from_angles(camera.yaw_radians, camera.pitch_radians);
	}
	if (length(forward) <= 0.000001F) {
		forward = { 0.0F, 0.0F, -1.0F };
	}

	Vec3 up_candidate = camera.projection == CameraProjection::Orthographic && camera.locked_orthographic_view
			? normalized(camera.locked_up)
			: normalized(camera.up);
	if (length(up_candidate) <= 0.000001F || std::abs(dot(up_candidate, forward)) > 0.999F) {
		up_candidate = fallback_up_for_forward(forward);
	}

	const CameraBasis kBasis = make_camera_basis(forward, up_candidate);

	const Vec3 kEye = camera.target - kBasis.forward * std::max(kMinCameraDistance, camera.distance);
	return { kEye, camera.target, kBasis.up, kBasis.forward };
}

ViewportCameraSnapshotArray ViewportCameraController::camera_snapshots() const {
	ViewportCameraSnapshotArray snapshots{};
	for (int index = 0; index < 4; ++index) {
		const ViewportCameraState &state = camera(index);
		const ViewportCameraPose kPose = camera_pose(index);
		snapshots[static_cast<std::size_t>(index)] = ViewportCameraSnapshot{
			.camera_index = index,
			.eye = kPose.eye,
			.target = kPose.target,
			.up = kPose.up,
			.forward = kPose.forward,
			.projection = state.projection,
			.fov_degrees = state.fov_degrees,
			.orthographic_size = state.orthographic_size,
		};
	}
	return snapshots;
}

ViewportPixelSize ViewportCameraController::safe_viewport_size(ViewportPixelSize viewport_size) const noexcept {
	return ViewportPixelSize{
		.width = std::max(1, viewport_size.width),
		.height = std::max(1, viewport_size.height),
	};
}

ViewportCameraState &ViewportCameraController::active_camera() {
	return cameras_[static_cast<std::size_t>(active_camera_index_)];
}

} // namespace quader::ui
