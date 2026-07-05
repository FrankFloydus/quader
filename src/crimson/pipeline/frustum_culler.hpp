#pragma once

#include "crimson/scene/render_camera.hpp"
#include "crimson/scene/render_object.hpp"
#include "math/aabb.hpp"

namespace crimson {

struct CameraFrustum {
	quader::math::Vec3 eye;
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
	quader::math::Vec3 right{ 1.0F, 0.0F, 0.0F };
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	CameraProjection projection = CameraProjection::Perspective;
	float near_plane_m = 0.05F;
	float far_plane_m = 1000.0F;
	float vertical_tangent = 1.0F;
	float horizontal_tangent = 1.0F;
	float orthographic_half_height_m = 12.0F;
	float orthographic_half_width_m = 12.0F;
};

struct CullingDecision {
	bool visible = true;
	bool invalid_bounds = false;
};

[[nodiscard]] CameraFrustum make_camera_frustum(const RenderCamera &camera, float aspect_ratio = 1.0F) noexcept;
[[nodiscard]] CullingDecision cull_object_bounds(
		const CameraFrustum &frustum,
		quader::math::Aabb bounds) noexcept;
[[nodiscard]] float object_camera_distance_sq(const RenderObject &object, const RenderCamera &camera) noexcept;

} // namespace crimson
