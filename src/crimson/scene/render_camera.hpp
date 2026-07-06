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

#include "math/vec3.hpp"

namespace crimson {

/// Default near clip distance for viewport cameras, in meters.
inline constexpr float kDefaultCameraNearPlaneM = 0.01F;
/// Default far clip distance for viewport cameras, in meters.
inline constexpr float kDefaultCameraFarPlaneM = 1000.0F;

/// Camera projection mode for renderer views.
enum class CameraProjection {
	/// Perspective projection.
	Perspective,
	/// Orthographic projection.
	Orthographic,
};

/// Camera data consumed by a render view.
struct RenderCamera {
	/// Camera eye position in world space.
	quader::math::Vec3 eye{};
	/// Camera target position in world space.
	quader::math::Vec3 target{};
	/// Camera up direction.
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	/// Camera forward direction.
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
	/// Projection mode.
	CameraProjection projection = CameraProjection::Perspective;
	/// Near clip distance in meters.
	float near_plane_m = kDefaultCameraNearPlaneM;
	/// Far clip distance in meters.
	float far_plane_m = kDefaultCameraFarPlaneM;
	/// Perspective vertical field of view in degrees.
	float vertical_fov_degrees = 60.0F;
	/// Orthographic view height in meters.
	float orthographic_height_m = 24.0F;
	/// Camera exposure value at ISO 100.
	float exposure_ev100 = 12.0F;
	/// Exposure compensation in stops.
	float exposure_compensation_ev = 0.0F;
};

} // namespace crimson
