/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/pipeline/frustum_culler.hpp"

#include "crimson/scene/render_object.hpp"
#include "math/scalar.hpp"

#include <algorithm>
#include <cmath>

namespace crimson {
namespace {

constexpr float kPi = 3.14159265358979323846F;

[[nodiscard]] float finite_or_zero(float value) noexcept {
	return quader::math::is_finite(value) ? value : 0.0F;
}

[[nodiscard]] bool finite_bounds(quader::math::Aabb bounds) noexcept {
	return !quader::math::empty(bounds) && quader::math::is_finite(bounds.min) && quader::math::is_finite(bounds.max);
}

[[nodiscard]] float axis_extent(quader::math::Vec3 half_extent, quader::math::Vec3 axis) noexcept {
	return std::fabs(axis.x) * half_extent.x + std::fabs(axis.y) * half_extent.y + std::fabs(axis.z) * half_extent.z;
}

[[nodiscard]] quader::math::Vec3 camera_forward(const RenderCamera &camera) noexcept {
	quader::math::Vec3 forward = quader::math::normalized(camera.forward);
	if (quader::math::length_squared(forward) <= quader::math::kDefaultEpsilon) {
		forward = quader::math::normalized(camera.target - camera.eye);
	}
	if (quader::math::length_squared(forward) <= quader::math::kDefaultEpsilon) {
		return { 0.0F, 0.0F, -1.0F };
	}
	return forward;
}

} // namespace

CameraFrustum make_camera_frustum(const RenderCamera &camera, float aspect_ratio) noexcept {
	const quader::math::Vec3 kForward = camera_forward(camera);
	quader::math::Vec3 right = quader::math::normalized(quader::math::cross(kForward, camera.up));
	if (quader::math::length_squared(right) <= quader::math::kDefaultEpsilon) {
		right = { 1.0F, 0.0F, 0.0F };
	}
	const quader::math::Vec3 kUp = quader::math::normalized(quader::math::cross(right, kForward));

	const float kFovDegrees = std::clamp(camera.vertical_fov_degrees, 1.0F, 179.0F);
	const float kVerticalTangent = std::tan(kFovDegrees * 0.5F * kPi / 180.0F);
	const float kSafeAspect = std::max(0.01F, aspect_ratio);
	const float kOrthoHalfHeight = std::max(0.01F, camera.orthographic_height_m) * 0.5F;

	return CameraFrustum{
		.eye = camera.eye,
		.forward = kForward,
		.right = right,
		.up = kUp,
		.projection = camera.projection,
		.near_plane_m = std::max(0.0F, camera.near_plane_m),
		.far_plane_m = std::max(camera.near_plane_m, camera.far_plane_m),
		.vertical_tangent = kVerticalTangent,
		.horizontal_tangent = kVerticalTangent * kSafeAspect,
		.orthographic_half_height_m = kOrthoHalfHeight,
		.orthographic_half_width_m = kOrthoHalfHeight * kSafeAspect,
	};
}

CullingDecision cull_object_bounds(const CameraFrustum &frustum, quader::math::Aabb bounds) noexcept {
	if (!finite_bounds(bounds)) {
		return CullingDecision{ .visible = true, .invalid_bounds = true };
	}

	const quader::math::Vec3 kCenter = quader::math::center(bounds);
	const quader::math::Vec3 kHalfExtent = (bounds.max - bounds.min) * 0.5F;
	const quader::math::Vec3 kToCenter = kCenter - frustum.eye;

	const float kZ = quader::math::dot(kToCenter, frustum.forward);
	const float kDepthRadius = axis_extent(kHalfExtent, frustum.forward);
	if (kZ + kDepthRadius < frustum.near_plane_m || kZ - kDepthRadius > frustum.far_plane_m) {
		return CullingDecision{ .visible = false };
	}

	const float kX = quader::math::dot(kToCenter, frustum.right);
	const float kXRadius = axis_extent(kHalfExtent, frustum.right);
	const float kY = quader::math::dot(kToCenter, frustum.up);
	const float kYRadius = axis_extent(kHalfExtent, frustum.up);

	if (frustum.projection == CameraProjection::Orthographic) {
		if (std::fabs(kX) > frustum.orthographic_half_width_m + kXRadius) {
			return CullingDecision{ .visible = false };
		}
		if (std::fabs(kY) > frustum.orthographic_half_height_m + kYRadius) {
			return CullingDecision{ .visible = false };
		}
		return CullingDecision{ .visible = true };
	}

	const float kZForSides = std::max(kZ, frustum.near_plane_m);
	if (std::fabs(kX) > kZForSides * frustum.horizontal_tangent + kXRadius) {
		return CullingDecision{ .visible = false };
	}
	if (std::fabs(kY) > kZForSides * frustum.vertical_tangent + kYRadius) {
		return CullingDecision{ .visible = false };
	}

	return CullingDecision{ .visible = true };
}

float object_camera_distance_sq(const RenderObject &object, const RenderCamera &camera) noexcept {
	const float kX = finite_or_zero(object.world_from_object[12]) - camera.eye.x;
	const float kY = finite_or_zero(object.world_from_object[13]) - camera.eye.y;
	const float kZ = finite_or_zero(object.world_from_object[14]) - camera.eye.z;
	return kX * kX + kY * kY + kZ * kZ;
}

} // namespace crimson
