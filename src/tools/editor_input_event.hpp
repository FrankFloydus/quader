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

#include "math/vec2.hpp"
#include "math/vec3.hpp"

#include <cstdint>
#include <optional>

namespace quader::tools {

/// Pointer button translated from the viewport host.
enum class PointerButton {
	/// No button is associated with the event.
	None,
	/// Left mouse button.
	Left,
	/// Middle mouse button.
	Middle,
	/// Right mouse button.
	Right,
};

/// Pointer event phase translated from the viewport host.
enum class PointerPhase {
	/// Passive hover or neutral movement.
	Hover,
	/// Button press.
	Press,
	/// Pointer movement.
	Move,
	/// Button release.
	Release,
};

/// World-space ray from the active viewport camera through a pointer position.
struct ViewportRay {
	/// Ray origin in world coordinates.
	quader::math::Vec3 origin;
	/// Ray direction in world coordinates.
	quader::math::Vec3 direction{ 0.0F, -1.0F, 0.0F };
};

/// Kind of modeling surface hit available to a tool.
enum class SurfaceHitKind {
	/// Grid or construction-plane fallback.
	Grid,
	/// Whole object hit.
	Object,
	/// Mesh face hit.
	Face,
};

/// Document-side surface hit used as a placement seed or hover candidate.
struct SurfaceHit {
	/// Hit position in world coordinates.
	quader::math::Vec3 position;
	/// Hit normal in world coordinates.
	quader::math::Vec3 normal{ 0.0F, 1.0F, 0.0F };
	/// Stable object identifier when an object was hit.
	std::uint64_t object_id = 0;
	/// Component index when the hit refers to a sub-object element.
	std::uint32_t component_index = 0;
	/// Hit classification.
	SurfaceHitKind kind = SurfaceHitKind::Grid;
};

/// Toolkit-neutral pointer event consumed by modeling tools.
struct PointerEvent {
	/// Viewport-local pointer position in physical pixels.
	quader::math::Vec2 position;
	/// Button associated with this event.
	PointerButton button = PointerButton::None;
	/// True when `button` is currently pressed.
	bool pressed = false;
	/// Pointer phase.
	PointerPhase phase = PointerPhase::Hover;
	/// True when camera navigation owns the pointer.
	bool navigation_active = false;
	/// True when tools should snap placement to the active grid.
	bool snap_to_grid = true;
	/// Active grid size in world units.
	float grid_size = 1.0F;
	/// Optional world ray for grid and document hit testing.
	std::optional<ViewportRay> ray;
	/// Optional live surface hit; modal tools may ignore it after locking a construction plane.
	std::optional<SurfaceHit> surface_hit;
	/// View index that produced this event.
	std::uint32_t view_index = 0;
};

/// Toolkit-neutral key event consumed by modeling tools.
struct KeyEvent {
	/// Host key code.
	int key_code = 0;
	/// True when the key is pressed.
	bool pressed = false;
	/// True when the key event is an auto-repeat.
	bool auto_repeat = false;
};

} // namespace quader::tools
