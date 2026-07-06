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

#include "crimson/scene/render_camera.hpp"
#include "crimson/scene/render_object.hpp"
#include "math/aabb.hpp"

namespace crimson {

/// Camera frustum data prepared for CPU culling.
struct CameraFrustum {
	/// Camera eye position.
	quader::math::Vec3 eye;
	/// Forward basis direction.
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
	/// Right basis direction.
	quader::math::Vec3 right{ 1.0F, 0.0F, 0.0F };
	/// Up basis direction.
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	/// Projection mode.
	CameraProjection projection = CameraProjection::Perspective;
	/// Near clip distance in meters.
	float near_plane_m = kDefaultCameraNearPlaneM;
	/// Far clip distance in meters.
	float far_plane_m = kDefaultCameraFarPlaneM;
	/// Tangent of half the vertical field of view.
	float vertical_tangent = 1.0F;
	/// Tangent of half the horizontal field of view.
	float horizontal_tangent = 1.0F;
	/// Orthographic half height in meters.
	float orthographic_half_height_m = 12.0F;
	/// Orthographic half width in meters.
	float orthographic_half_width_m = 12.0F;
};

/// CPU culling decision for one object bounds test.
struct CullingDecision {
	/// True when the object should be submitted.
	bool visible = true;
	/// True when bounds are empty or otherwise invalid.
	bool invalid_bounds = false;
};

/**
 * Build frustum data from a render camera.
 *
 * @param camera Camera to convert.
 * @param aspect_ratio View aspect ratio.
 * @return Camera frustum used by CPU culling.
 */
[[nodiscard]] CameraFrustum make_camera_frustum(const RenderCamera &camera, float aspect_ratio = 1.0F) noexcept;
/**
 * Cull object bounds against a frustum.
 *
 * @param frustum Frustum to test against.
 * @param bounds World-space bounds.
 * @return Culling decision with invalid-bounds state.
 */
[[nodiscard]] CullingDecision cull_object_bounds(
		const CameraFrustum &frustum,
		quader::math::Aabb bounds) noexcept;
/**
 * Compute squared camera distance for object sorting.
 *
 * @param object Object to measure.
 * @param camera Camera used as the distance origin.
 * @return Squared distance from camera eye to object bounds center.
 */
[[nodiscard]] float object_camera_distance_sq(const RenderObject &object, const RenderCamera &camera) noexcept;

} // namespace crimson
