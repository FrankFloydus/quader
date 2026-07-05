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

#include <bgfx/bgfx.h>

namespace crimson {

bool current_render_homogeneous_depth() noexcept {
	const bgfx::Caps *caps = bgfx::getCaps();
	return caps != nullptr && caps->homogeneousDepth;
}

} // namespace crimson
