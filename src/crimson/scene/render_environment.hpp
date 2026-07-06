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

#include "crimson/renderer_types.hpp"
#include "math/vec3.hpp"

namespace crimson {

/// Environment and clear-color settings for a render snapshot.
struct RenderEnvironment {
	/// Optional environment resource handle.
	RenderEnvironmentHandle environment;
	/// Linear clear color used when no sky/environment is rendered.
	quader::math::Vec3 clear_color_linear{ 2.0F / 255.0F, 2.0F / 255.0F, 2.0F / 255.0F };
	/// Environment intensity multiplier.
	float intensity = 1.0F;
};

} // namespace crimson
