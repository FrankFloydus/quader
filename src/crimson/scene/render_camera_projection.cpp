/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/scene/render_camera_projection.hpp"

#include "math/scalar.hpp"

#include <algorithm>
#include <cmath>

namespace crimson {
namespace {

constexpr float kPi = 3.14159265358979323846F;

[[nodiscard]] quader::math::Vec3 normalized_or(
		quader::math::Vec3 value,
		quader::math::Vec3 fallback) noexcept {
	const quader::math::Vec3 kNormalized = quader::math::normalized(value);
	return quader::math::length_squared(kNormalized) <= quader::math::kDefaultEpsilon * quader::math::kDefaultEpsilon
			? fallback
			: kNormalized;
}

struct RenderCameraBasis {
	quader::math::Vec3 right{ -1.0F, 0.0F, 0.0F };
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	quader::math::Vec3 view{ 0.0F, 0.0F, -1.0F };
};

[[nodiscard]] float degrees_to_radians(float degrees) noexcept {
	return degrees * (kPi / 180.0F);
}

[[nodiscard]] RenderCameraBasis camera_basis(const RenderCamera &camera) noexcept {
	const quader::math::Vec3 kFallbackForward =
			normalized_or(camera.forward, { 0.0F, 0.0F, -1.0F });
	const quader::math::Vec3 kView = normalized_or(camera.target - camera.eye, kFallbackForward);
	const quader::math::Vec3 kRight = normalized_or(
			quader::math::cross(camera.up, kView),
			{ -1.0F, 0.0F, 0.0F });
	return RenderCameraBasis{
		.right = kRight,
		.up = quader::math::cross(kView, kRight),
		.view = kView,
	};
}

void clear_matrix(std::array<float, 16> &matrix) noexcept {
	matrix.fill(0.0F);
}

void make_look_at_matrix(
		std::array<float, 16> &matrix,
		const RenderCamera &camera,
		const RenderCameraBasis &basis) noexcept {
	matrix[0] = basis.right.x;
	matrix[1] = basis.up.x;
	matrix[2] = basis.view.x;
	matrix[3] = 0.0F;

	matrix[4] = basis.right.y;
	matrix[5] = basis.up.y;
	matrix[6] = basis.view.y;
	matrix[7] = 0.0F;

	matrix[8] = basis.right.z;
	matrix[9] = basis.up.z;
	matrix[10] = basis.view.z;
	matrix[11] = 0.0F;

	matrix[12] = -quader::math::dot(basis.right, camera.eye);
	matrix[13] = -quader::math::dot(basis.up, camera.eye);
	matrix[14] = -quader::math::dot(basis.view, camera.eye);
	matrix[15] = 1.0F;
}

void make_projection_matrix(
		std::array<float, 16> &matrix,
		const RenderCamera &camera,
		float aspect_ratio,
		bool homogeneous_depth) noexcept {
	clear_matrix(matrix);
	const float kNear = camera.near_plane_m;
	const float kFar = camera.far_plane_m;
	const float kDiff = kFar - kNear;
	const float kHeight = 1.0F / std::tan(degrees_to_radians(camera.vertical_fov_degrees) * 0.5F);
	const float kWidth = kHeight / aspect_ratio;
	const float kDepthScale = homogeneous_depth ? (kFar + kNear) / kDiff : kFar / kDiff;
	const float kDepthBias = homogeneous_depth ? (2.0F * kFar * kNear) / kDiff : kNear * kDepthScale;

	matrix[0] = kWidth;
	matrix[5] = kHeight;
	matrix[10] = kDepthScale;
	matrix[11] = 1.0F;
	matrix[14] = -kDepthBias;
}

void make_orthographic_matrix(
		std::array<float, 16> &matrix,
		const RenderCamera &camera,
		float aspect_ratio,
		bool homogeneous_depth) noexcept {
	clear_matrix(matrix);
	const float kExtent = std::max(0.01F, camera.orthographic_height_m) * 0.5F;
	const float kLeft = -kExtent * aspect_ratio;
	const float kRight = kExtent * aspect_ratio;
	const float kBottom = -kExtent;
	const float kTop = kExtent;
	const float kNear = camera.near_plane_m;
	const float kFar = camera.far_plane_m;

	matrix[0] = 2.0F / (kRight - kLeft);
	matrix[5] = 2.0F / (kTop - kBottom);
	matrix[10] = (homogeneous_depth ? 2.0F : 1.0F) / (kFar - kNear);
	matrix[12] = (kLeft + kRight) / (kLeft - kRight);
	matrix[13] = (kTop + kBottom) / (kBottom - kTop);
	matrix[14] = homogeneous_depth ? (kNear + kFar) / (kNear - kFar) : kNear / (kNear - kFar);
	matrix[15] = 1.0F;
}

} // namespace

RenderCameraMatrices render_camera_matrices(
		const RenderCamera &camera,
		float aspect_ratio,
		bool homogeneous_depth) {
	RenderCameraMatrices matrices;
	const float kAspect = std::max(0.0001F, aspect_ratio);
	const RenderCameraBasis kBasis = camera_basis(camera);

	make_look_at_matrix(matrices.view, camera, kBasis);
	if (camera.projection == CameraProjection::Orthographic) {
		make_orthographic_matrix(matrices.projection, camera, kAspect, homogeneous_depth);
		return matrices;
	}

	make_projection_matrix(matrices.projection, camera, kAspect, homogeneous_depth);
	return matrices;
}

RenderCameraRay render_camera_ray_from_viewport_point(
		const RenderCamera &camera,
			RenderCameraViewportSize viewport_size,
			quader::math::Vec2 local_point_px,
			bool homogeneous_depth) {
	(void)homogeneous_depth;
	const float kWidth = std::max(1.0F, viewport_size.width_px);
	const float kHeight = std::max(1.0F, viewport_size.height_px);
	const float kAspect = kWidth / kHeight;
	const float kNdcX = (local_point_px.x / kWidth) * 2.0F - 1.0F;
	const float kNdcY = ((kHeight - local_point_px.y) / kHeight) * 2.0F - 1.0F;
	const RenderCameraBasis kBasis = camera_basis(camera);
	if (camera.projection == CameraProjection::Orthographic) {
		const float kExtent = std::max(0.01F, camera.orthographic_height_m) * 0.5F;
		return RenderCameraRay{
			.origin = camera.eye + kBasis.right * (kNdcX * kExtent * kAspect) + kBasis.up * (kNdcY * kExtent) + kBasis.view * camera.near_plane_m,
			.direction = kBasis.view,
		};
	}

	const float kTanY = std::tan(degrees_to_radians(camera.vertical_fov_degrees) * 0.5F);
	const float kTanX = kTanY * kAspect;
	return RenderCameraRay{
		.origin = camera.eye,
		.direction = normalized_or(kBasis.view + kBasis.right * (kNdcX * kTanX) + kBasis.up * (kNdcY * kTanY), kBasis.view),
	};
}

} // namespace crimson
