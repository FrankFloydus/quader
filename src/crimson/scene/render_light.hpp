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

/// Light type consumed by renderer snapshots.
enum class RenderLightKind {
	/// Directional light using direction and illuminance-like intensity.
	Directional,
	/// Point light using position, range, and intensity.
	Point,
	/// Spot light using position, direction, range, and intensity.
	Spot,
};

/// CPU-side renderer light payload.
struct RenderLight {
	/// Light type.
	RenderLightKind kind = RenderLightKind::Directional;
	/// Light position in world space for punctual lights.
	quader::math::Vec3 position_world{};
	/// Light direction in world space.
	quader::math::Vec3 direction_world{ -0.45F, -0.80F, -0.38F };
	/// Linear light color.
	quader::math::Vec3 color_linear{ 1.0F, 1.0F, 1.0F };
	/// Light intensity in the renderer's current unit convention.
	float intensity = 1.0F;
	/// Punctual light range in meters; zero means no explicit range.
	float range_m = 0.0F;
};

} // namespace crimson
