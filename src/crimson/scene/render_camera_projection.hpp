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
#include "math/vec2.hpp"
#include "math/vec3.hpp"

#include <array>

namespace crimson {

/// Pixel size of the viewport rectangle used for camera projection.
struct RenderCameraViewportSize {
	/// Viewport width in physical pixels.
	float width_px = 1.0F;
	/// Viewport height in physical pixels.
	float height_px = 1.0F;
};

/// View and projection matrices produced with the same conventions Crimson submits to bgfx.
struct RenderCameraMatrices {
	/// Column-major view matrix.
	std::array<float, 16> view{};
	/// Column-major projection matrix.
	std::array<float, 16> projection{};
};

/// World-space ray reconstructed from a Crimson render camera and viewport position.
struct RenderCameraRay {
	/// Ray origin in world space.
	quader::math::Vec3 origin;
	/// Normalized ray direction in world space.
	quader::math::Vec3 direction{ 0.0F, 0.0F, -1.0F };
};

/**
 * Return the active bgfx homogeneous-depth convention.
 *
 * @return True when the current bgfx backend uses homogeneous depth.
 */
[[nodiscard]] bool current_render_homogeneous_depth() noexcept;

/**
 * Build render camera matrices with Crimson's bgfx-facing projection conventions.
 *
 * @param camera Camera state to project.
 * @param aspect_ratio Viewport width divided by height.
 * @param homogeneous_depth Active bgfx homogeneous-depth convention.
 * @return View and projection matrices matching scene rendering.
 */
[[nodiscard]] RenderCameraMatrices render_camera_matrices(
		const RenderCamera &camera,
		float aspect_ratio,
		bool homogeneous_depth);

/**
 * Reconstruct a world ray from a viewport-local pixel position.
 *
 * The math intentionally shares the same view-projection path as Crimson scene
 * rendering so tools and picking do not drift from what the user sees.
 *
 * @param camera Camera state to project.
 * @param viewport_size Viewport rectangle size in physical pixels.
 * @param local_point_px Pointer position relative to the viewport rectangle.
 * @param homogeneous_depth Active bgfx homogeneous-depth convention.
 * @return World-space ray corresponding to `local_point_px`.
 */
[[nodiscard]] RenderCameraRay render_camera_ray_from_viewport_point(
		const RenderCamera &camera,
		RenderCameraViewportSize viewport_size,
		quader::math::Vec2 local_point_px,
		bool homogeneous_depth);

} // namespace crimson
