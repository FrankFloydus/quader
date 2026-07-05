#include "ui/viewport/viewport_camera_controller.hpp"

#include <algorithm>
#include <cmath>

namespace quader::ui {
namespace {

using quader::math::Vec3;
using quader::math::cross;
using quader::math::dot;
using quader::math::length;
using quader::math::normalized;

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

[[nodiscard]] Vec3 forward_from_angles(float yaw_radians, float pitch_radians)
{
    const float cp = std::cos(pitch_radians);
    return normalized(Vec3{
        std::sin(yaw_radians) * cp,
        std::sin(pitch_radians),
        -std::cos(yaw_radians) * cp,
    });
}

[[nodiscard]] Vec3 fallback_up_for_forward(Vec3 forward)
{
    if (std::abs(forward.y) > 0.999F) {
        return forward.y < 0.0F ? Vec3{0.0F, 0.0F, 1.0F} : Vec3{0.0F, 0.0F, -1.0F};
    }
    return {0.0F, 1.0F, 0.0F};
}

struct CameraBasis {
    Vec3 forward{0.0F, 0.0F, -1.0F};
    Vec3 up{0.0F, 1.0F, 0.0F};
};

struct CameraTransform {
    Vec3 x_axis{1.0F, 0.0F, 0.0F};
    Vec3 y_axis{0.0F, 1.0F, 0.0F};
    Vec3 z_axis{0.0F, 0.0F, 1.0F};
    Vec3 origin{};
};

[[nodiscard]] CameraBasis make_camera_basis(
    Vec3 forward_direction,
    Vec3 up_direction,
    Vec3 fallback_forward = {0.0F, 0.0F, -1.0F})
{
    Vec3 forward = normalized(forward_direction);
    if (length(forward) <= 0.000001F) {
        forward = normalized(fallback_forward);
    }
    if (length(forward) <= 0.000001F) {
        forward = {0.0F, 0.0F, -1.0F};
    }

    Vec3 candidate_up = normalized(up_direction);
    if (length(candidate_up) <= 0.000001F || std::abs(dot(candidate_up, forward)) > 0.999F) {
        candidate_up = fallback_up_for_forward(forward);
    }

    Vec3 right = normalized(cross(forward, candidate_up));
    if (length(right) <= 0.000001F) {
        right = {1.0F, 0.0F, 0.0F};
    }

    Vec3 up = normalized(cross(right, forward));
    if (length(up) <= 0.000001F) {
        up = fallback_up_for_forward(forward);
    }

    return {forward, up};
}

void sync_angles_for_forward(ViewportCameraState& camera, Vec3 forward)
{
    const Vec3 normalized_forward = normalized(forward);
    if (length(normalized_forward) <= 0.000001F) {
        return;
    }

    camera.pitch_radians = std::asin(std::clamp(normalized_forward.y, -1.0F, 1.0F));
    camera.yaw_radians = std::atan2(normalized_forward.x, -normalized_forward.z);
}

[[nodiscard]] Vec3 transform_vector(const CameraTransform& transform, Vec3 value)
{
    return transform.x_axis * value.x + transform.y_axis * value.y + transform.z_axis * value.z;
}

[[nodiscard]] Vec3 rotate_vector_around_axis(Vec3 value, Vec3 axis, float angle_radians)
{
    const Vec3 normal = normalized(axis);
    if (length(normal) <= 0.000001F) {
        return value;
    }

    const float c = std::cos(angle_radians);
    const float s = std::sin(angle_radians);
    const float one_minus_c = 1.0F - c;
    return value * c + cross(normal, value) * s + normal * (dot(normal, value) * one_minus_c);
}

[[nodiscard]] CameraTransform orthonormalized_basis(CameraTransform basis)
{
    Vec3 right = normalized(basis.x_axis);
    if (length(right) <= 0.000001F) {
        right = {1.0F, 0.0F, 0.0F};
    }

    Vec3 up = basis.y_axis - right * dot(basis.y_axis, right);
    up = normalized(up);
    if (length(up) <= 0.000001F) {
        const Vec3 fallback = std::abs(right.y) > 0.9F ? Vec3{0.0F, 0.0F, 1.0F} : Vec3{0.0F, 1.0F, 0.0F};
        up = normalized(fallback - right * dot(fallback, right));
    }

    Vec3 back = normalized(cross(right, up));
    if (length(back) <= 0.000001F) {
        back = {0.0F, 0.0F, 1.0F};
    }

    up = normalized(cross(back, right));
    return {right, up, back, basis.origin};
}

[[nodiscard]] CameraTransform rotation_basis(Vec3 axis, float angle_radians)
{
    const Vec3 normal = normalized(axis);
    if (length(normal) <= 0.000001F) {
        return {};
    }

    return {
        rotate_vector_around_axis({1.0F, 0.0F, 0.0F}, normal, angle_radians),
        rotate_vector_around_axis({0.0F, 1.0F, 0.0F}, normal, angle_radians),
        rotate_vector_around_axis({0.0F, 0.0F, 1.0F}, normal, angle_radians),
        {},
    };
}

[[nodiscard]] CameraTransform basis_multiply(const CameraTransform& left, const CameraTransform& right)
{
    return {
        transform_vector(left, right.x_axis),
        transform_vector(left, right.y_axis),
        transform_vector(left, right.z_axis),
        {},
    };
}

[[nodiscard]] CameraTransform rotated_camera_basis(
    const CameraTransform& camera_transform,
    const CameraTransform& rotation)
{
    return orthonormalized_basis({
        transform_vector(rotation, camera_transform.x_axis),
        transform_vector(rotation, camera_transform.y_axis),
        transform_vector(rotation, camera_transform.z_axis),
        camera_transform.origin,
    });
}

[[nodiscard]] CameraTransform clamped_rotation_to_upright(
    const CameraTransform& camera_transform,
    const CameraTransform& rotation)
{
    const Vec3 world_up{0.0F, 1.0F, 0.0F};
    const Vec3 new_up = normalized(transform_vector(rotation, camera_transform.y_axis));
    if (dot(new_up, world_up) >= 0.0F) {
        return orthonormalized_basis(rotation);
    }

    const Vec3 new_right = normalized(transform_vector(rotation, camera_transform.x_axis));
    const Vec3 new_forward = normalized(transform_vector(rotation, camera_transform.z_axis * -1.0F));
    Vec3 clamped_up = new_up - world_up * dot(new_up, world_up);
    clamped_up = length(clamped_up) <= 0.000001F ? Vec3{1.0F, 0.0F, 0.0F} : normalized(clamped_up);

    const float cos_angle = std::clamp(dot(clamped_up, new_up), -1.0F, 1.0F);
    const float correction_angle = std::acos(cos_angle);
    const float correction_sign = dot(new_forward, world_up) > 0.0F ? -1.0F : 1.0F;
    const CameraTransform correction = rotation_basis(new_right, correction_angle * correction_sign);
    return orthonormalized_basis(basis_multiply(correction, rotation));
}

[[nodiscard]] Vec3 viewport_camera_forward(const CameraTransform& camera_transform)
{
    const Vec3 forward = normalized(camera_transform.z_axis * -1.0F);
    return length(forward) <= 0.000001F ? Vec3{0.0F, 0.0F, -1.0F} : forward;
}

[[nodiscard]] Vec3 viewport_camera_right(const CameraTransform& camera_transform)
{
    const Vec3 right = normalized(camera_transform.x_axis);
    return length(right) <= 0.000001F ? Vec3{1.0F, 0.0F, 0.0F} : right;
}

[[nodiscard]] Vec3 viewport_camera_up(const CameraTransform& camera_transform)
{
    const Vec3 up = normalized(camera_transform.y_axis);
    return length(up) <= 0.000001F ? Vec3{0.0F, 1.0F, 0.0F} : up;
}

[[nodiscard]] float viewport_distance_to_pivot(const CameraTransform& camera_transform, Vec3 pivot)
{
    const Vec3 to_pivot = pivot - camera_transform.origin;
    const float forward_distance = dot(to_pivot, viewport_camera_forward(camera_transform));
    if (forward_distance > kMinCameraDistance) {
        return forward_distance;
    }

    return std::max(kMinCameraDistance, length(to_pivot));
}

[[nodiscard]] CameraTransform transform_from_pose(const ViewportCameraPose& pose)
{
    const CameraBasis basis = make_camera_basis(pose.target - pose.eye, pose.up);
    Vec3 right = normalized(cross(basis.forward, basis.up));
    if (length(right) <= 0.000001F) {
        right = {1.0F, 0.0F, 0.0F};
    }

    const Vec3 up = normalized(cross(right, basis.forward));
    return {
        right,
        length(up) <= 0.000001F ? basis.up : up,
        basis.forward * -1.0F,
        pose.eye,
    };
}

void apply_navigation_transform(
    ViewportCameraState& camera,
    const CameraTransform& transform,
    Vec3 pivot,
    bool has_camera_size = false,
    float camera_size = 0.0F,
    bool preserve_view_distance = false)
{
    const Vec3 forward = viewport_camera_forward(transform);
    const CameraBasis basis = make_camera_basis(forward, transform.y_axis, camera.forward);
    const float requested_distance = preserve_view_distance
        ? camera.distance
        : viewport_distance_to_pivot(transform, pivot);
    const float view_distance = std::clamp(requested_distance, kMinCameraDistance, kMaxCameraDistance);

    camera.pivot = pivot;
    camera.forward = basis.forward;
    camera.up = basis.up;
    camera.target = transform.origin + camera.forward * view_distance;
    camera.distance = view_distance;
    if (has_camera_size) {
        camera.orthographic_size = std::max(0.0001F, camera_size);
    }
    sync_angles_for_forward(camera, camera.forward);
}

void apply_orbit_transform(ViewportCameraState& camera, const CameraTransform& transform, Vec3 pivot)
{
    Vec3 forward = normalized(pivot - transform.origin);
    if (length(forward) <= 0.000001F) {
        forward = viewport_camera_forward(transform);
    }

    const float radius = std::clamp(length(pivot - transform.origin), kMinCameraDistance, kMaxCameraDistance);
    const CameraBasis basis = make_camera_basis(forward, transform.y_axis, camera.forward);

    camera.pivot = pivot;
    camera.forward = basis.forward;
    camera.up = basis.up;
    camera.target = pivot;
    camera.distance = radius;
    sync_angles_for_forward(camera, camera.forward);
}

[[nodiscard]] float capped_orthographic_zoom_size(float current_size, float factor, float steps)
{
    const float safe_current_size = std::max(0.0001F, current_size);
    const float requested_size = safe_current_size / factor;
    if (steps < 0.0F) {
        if (safe_current_size >= kViewportGridDefaultPlaneSize) {
            return safe_current_size;
        }
        return std::clamp(requested_size, 0.0001F, kViewportGridDefaultPlaneSize);
    }

    return std::max(0.0001F, requested_size);
}

[[nodiscard]] int clamped_camera_index(int camera_index) noexcept
{
    return std::clamp(camera_index, 0, 3);
}

} // namespace

ViewportCameraController::ViewportCameraController()
{
    reset_default_cameras();
}

void ViewportCameraController::reset_default_cameras()
{
    for (ViewportCameraState& camera : cameras_) {
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
        camera.locked_forward = {0.0F, 0.0F, -1.0F};
        camera.locked_up = {0.0F, 1.0F, 0.0F};
        camera.locked_orthographic_view = false;
    }

    cameras_[1].projection = CameraProjection::Orthographic;
    cameras_[1].distance = 64.0F;
    cameras_[1].locked_forward = {0.0F, -1.0F, 0.0F};
    cameras_[1].locked_up = {0.0F, 0.0F, 1.0F};
    cameras_[1].forward = cameras_[1].locked_forward;
    cameras_[1].up = cameras_[1].locked_up;
    cameras_[1].locked_orthographic_view = true;

    cameras_[2].projection = CameraProjection::Orthographic;
    cameras_[2].distance = 64.0F;
    cameras_[2].locked_forward = {0.0F, 0.0F, 1.0F};
    cameras_[2].locked_up = {0.0F, 1.0F, 0.0F};
    cameras_[2].forward = cameras_[2].locked_forward;
    cameras_[2].up = cameras_[2].locked_up;
    cameras_[2].locked_orthographic_view = true;

    cameras_[3].projection = CameraProjection::Orthographic;
    cameras_[3].distance = 64.0F;
    cameras_[3].locked_forward = {-1.0F, 0.0F, 0.0F};
    cameras_[3].locked_up = {0.0F, 1.0F, 0.0F};
    cameras_[3].forward = cameras_[3].locked_forward;
    cameras_[3].up = cameras_[3].locked_up;
    cameras_[3].locked_orthographic_view = true;
}

const ViewportCameraState& ViewportCameraController::camera(int camera_index) const
{
    return cameras_[static_cast<std::size_t>(clamped_camera_index(camera_index))];
}

CameraProjection ViewportCameraController::camera_projection(int camera_index) const
{
    return camera(camera_index).projection;
}

NavigationMode ViewportCameraController::navigation_mode() const noexcept
{
    return navigation_mode_;
}

int ViewportCameraController::active_camera_index() const noexcept
{
    return active_camera_index_;
}

void ViewportCameraController::set_active_camera_index(int camera_index) noexcept
{
    active_camera_index_ = clamped_camera_index(camera_index);
}

bool ViewportCameraController::begin_navigation(int camera_index, NavigationMode mode, ViewportPoint position)
{
    set_active_camera_index(camera_index);
    ViewportCameraState& camera = active_camera();
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

void ViewportCameraController::update_navigation(ViewportPoint position, ViewportPixelSize viewport_size)
{
    if (navigation_mode_ == NavigationMode::None) {
        return;
    }

    ViewportCameraState& camera = active_camera();
    const ViewportPoint delta{position.x - last_navigation_mouse_.x, position.y - last_navigation_mouse_.y};
    last_navigation_mouse_ = position;
    const CameraTransform camera_transform = transform_from_pose(camera_pose(active_camera_index_));

    if (navigation_mode_ == NavigationMode::Orbit) {
        if (camera.locked_orthographic_view) {
            return;
        }

        camera.projection = CameraProjection::Perspective;

        const float yaw = static_cast<float>(delta.x) * kOrbitRadiansPerPixel;
        const float pitch = -static_cast<float>(delta.y) * kOrbitRadiansPerPixel;
        const CameraTransform yaw_rotation = rotation_basis({0.0F, 1.0F, 0.0F}, yaw);
        Vec3 right = normalized(transform_vector(yaw_rotation, viewport_camera_right(camera_transform)));
        if (length(right) <= 0.000001F) {
            right = {1.0F, 0.0F, 0.0F};
        }
        const CameraTransform pitch_rotation = rotation_basis(right, pitch);
        const CameraTransform rotation = clamped_rotation_to_upright(
            camera_transform,
            basis_multiply(pitch_rotation, yaw_rotation));

        CameraTransform result_transform = camera_transform;
        result_transform.origin = camera.pivot + transform_vector(rotation, camera_transform.origin - camera.pivot);
        const CameraTransform rotated_basis = rotated_camera_basis(camera_transform, rotation);
        result_transform.x_axis = rotated_basis.x_axis;
        result_transform.y_axis = rotated_basis.y_axis;
        result_transform.z_axis = rotated_basis.z_axis;
        apply_orbit_transform(camera, result_transform, camera.pivot);
    } else if (navigation_mode_ == NavigationMode::Pan) {
        const ViewportPixelSize safe_size = safe_viewport_size(viewport_size);
        const float viewport_height = static_cast<float>(safe_size.height);
        const float visible_height = camera.projection == CameraProjection::Orthographic
            ? std::max(0.0001F, camera.orthographic_size)
            : std::max(0.0001F, 2.0F * viewport_distance_to_pivot(camera_transform, camera.pivot)
                    * std::tan(camera.fov_degrees * 3.14159265359F / 180.0F * 0.5F));
        const float world_per_pixel = visible_height / viewport_height;
        const Vec3 move = viewport_camera_right(camera_transform) * (static_cast<float>(delta.x) * world_per_pixel)
            + viewport_camera_up(camera_transform) * (static_cast<float>(delta.y) * world_per_pixel);
        CameraTransform result_transform = camera_transform;
        result_transform.origin = result_transform.origin + move;
        apply_navigation_transform(camera, result_transform, camera.pivot + move);
    } else if (navigation_mode_ == NavigationMode::Fly) {
        const float yaw = static_cast<float>(delta.x) * kFlyLookRadiansPerPixel;
        const float pitch = -static_cast<float>(delta.y) * kFlyLookRadiansPerPixel;
        const CameraTransform yaw_rotation = rotation_basis({0.0F, 1.0F, 0.0F}, yaw);
        Vec3 right = normalized(transform_vector(yaw_rotation, viewport_camera_right(camera_transform)));
        if (length(right) <= 0.000001F) {
            right = {1.0F, 0.0F, 0.0F};
        }
        const CameraTransform pitch_rotation = rotation_basis(right, pitch);
        const CameraTransform rotation = clamped_rotation_to_upright(
            camera_transform,
            basis_multiply(pitch_rotation, yaw_rotation));
        const CameraTransform rotated_basis = rotated_camera_basis(camera_transform, rotation);

        CameraTransform result_transform = camera_transform;
        result_transform.x_axis = rotated_basis.x_axis;
        result_transform.y_axis = rotated_basis.y_axis;
        result_transform.z_axis = rotated_basis.z_axis;
        apply_navigation_transform(camera, result_transform, camera.pivot, false, 0.0F, true);
        camera.pivot = camera.target;
        camera.projection = CameraProjection::Perspective;
        camera.locked_orthographic_view = false;
    }
}

void ViewportCameraController::end_navigation()
{
    navigation_mode_ = NavigationMode::None;
    clear_fly_keys();
}

void ViewportCameraController::set_navigation_anchor(ViewportPoint position) noexcept
{
    last_navigation_mouse_ = position;
}

void ViewportCameraController::apply_wheel_zoom(
    int camera_index,
    float wheel_steps,
    ViewportPixelSize viewport_size)
{
    (void)viewport_size;
    set_active_camera_index(camera_index);

    ViewportCameraState& camera = active_camera();
    const float zoom_factor = std::pow(kNavigationZoomStepFactor, wheel_steps);
    const CameraTransform camera_transform = transform_from_pose(camera_pose(active_camera_index_));
    if (camera.projection == CameraProjection::Orthographic) {
        const float new_size = capped_orthographic_zoom_size(camera.orthographic_size, zoom_factor, wheel_steps);
        apply_navigation_transform(camera, camera_transform, camera.pivot, true, new_size);
    } else {
        const float distance = viewport_distance_to_pivot(camera_transform, camera.pivot);
        const float new_distance = std::max(kMinCameraDistance, distance / zoom_factor);
        CameraTransform result_transform = camera_transform;
        result_transform.origin = result_transform.origin
            + viewport_camera_forward(camera_transform) * (distance - new_distance);
        apply_navigation_transform(camera, result_transform, camera.pivot);
    }
}

void ViewportCameraController::adjust_fly_speed(float wheel_steps) noexcept
{
    fly_speed_ = std::clamp(fly_speed_ + wheel_steps * kFlySpeedWheelStep, kFlyMinSpeed, kFlyMaxSpeed);
}

void ViewportCameraController::set_fly_key(ViewportFlyKey key, bool pressed) noexcept
{
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

void ViewportCameraController::clear_fly_keys() noexcept
{
    fly_forward_ = false;
    fly_backward_ = false;
    fly_left_ = false;
    fly_right_ = false;
}

void ViewportCameraController::tick_fly_navigation(float delta_seconds)
{
    if (navigation_mode_ != NavigationMode::Fly || delta_seconds <= 0.0F) {
        return;
    }

    ViewportCameraState& camera = active_camera();
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

    const CameraTransform camera_transform = transform_from_pose(camera_pose(active_camera_index_));
    Vec3 world_direction = viewport_camera_right(camera_transform) * local_direction.x
        + viewport_camera_up(camera_transform) * local_direction.y
        + viewport_camera_forward(camera_transform) * local_direction.z;
    world_direction = normalized(world_direction);
    if (length(world_direction) <= 0.000001F) {
        return;
    }

    const Vec3 move = world_direction * (fly_speed_ * std::clamp(delta_seconds, 0.0F, 0.25F));
    CameraTransform result_transform = camera_transform;
    result_transform.origin = result_transform.origin + move;
    apply_navigation_transform(camera, result_transform, camera.pivot + move, false, 0.0F, true);
    camera.pivot = camera.target;
}

ViewportCameraPose ViewportCameraController::camera_pose(int camera_index) const
{
    const ViewportCameraState& camera = this->camera(camera_index);
    Vec3 forward = camera.projection == CameraProjection::Orthographic && camera.locked_orthographic_view
        ? normalized(camera.locked_forward)
        : normalized(camera.forward);
    if (length(forward) <= 0.000001F) {
        forward = forward_from_angles(camera.yaw_radians, camera.pitch_radians);
    }
    if (length(forward) <= 0.000001F) {
        forward = {0.0F, 0.0F, -1.0F};
    }

    Vec3 up_candidate = camera.projection == CameraProjection::Orthographic && camera.locked_orthographic_view
        ? normalized(camera.locked_up)
        : normalized(camera.up);
    if (length(up_candidate) <= 0.000001F || std::abs(dot(up_candidate, forward)) > 0.999F) {
        up_candidate = fallback_up_for_forward(forward);
    }

    const CameraBasis basis = make_camera_basis(forward, up_candidate);

    const Vec3 eye = camera.target - basis.forward * std::max(kMinCameraDistance, camera.distance);
    return {eye, camera.target, basis.up, basis.forward};
}

ViewportCameraSnapshotArray ViewportCameraController::camera_snapshots() const
{
    ViewportCameraSnapshotArray snapshots{};
    for (int index = 0; index < 4; ++index) {
        const ViewportCameraState& state = camera(index);
        const ViewportCameraPose pose = camera_pose(index);
        snapshots[static_cast<std::size_t>(index)] = ViewportCameraSnapshot{
            .camera_index = index,
            .eye = pose.eye,
            .target = pose.target,
            .up = pose.up,
            .forward = pose.forward,
            .projection = state.projection,
            .fov_degrees = state.fov_degrees,
            .orthographic_size = state.orthographic_size,
        };
    }
    return snapshots;
}

ViewportPixelSize ViewportCameraController::safe_viewport_size(ViewportPixelSize viewport_size) const noexcept
{
    return ViewportPixelSize{
        .width = std::max(1, viewport_size.width),
        .height = std::max(1, viewport_size.height),
    };
}

ViewportCameraState& ViewportCameraController::active_camera()
{
    return cameras_[static_cast<std::size_t>(active_camera_index_)];
}

} // namespace quader::ui
