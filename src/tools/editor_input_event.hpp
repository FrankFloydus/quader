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

#include "document/object_id.hpp"
#include "math/vec2.hpp"
#include "math/vec3.hpp"
#include "mesh/core/mesh_ids.hpp"

#include <cstdint>
#include <optional>
#include <variant>

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
	/// Mesh vertex hit.
	Vertex,
	/// Mesh edge hit.
	Edge,
	/// Mesh face hit.
	Face,
};

/// Active selection granularity used by the selection tool and UI actions.
enum class SelectionMode {
	/// Select whole document objects.
	Object,
	/// Select mesh vertices on the active edit target.
	Vertex,
	/// Select mesh edges on the active edit target.
	Edge,
	/// Select mesh faces on the active edit target.
	Face,
};

/// Typed component payload carried by surface hits.
using SurfaceHitComponent = std::variant<std::monostate, quader::mesh::VertexId, quader::mesh::EdgeId, quader::mesh::FaceId>;

/// Keyboard modifier state translated from the viewport host.
struct KeyboardModifiers {
	/// Shift is pressed.
	bool shift = false;
	/// Control or Command-equivalent additive modifier is pressed.
	bool control = false;
	/// Alt is pressed.
	bool alt = false;
};

/// Document-side surface hit used as a placement seed or hover candidate.
struct SurfaceHit {
	/// Hit position in world coordinates.
	quader::math::Vec3 position;
	/// Hit normal in world coordinates.
	quader::math::Vec3 normal{ 0.0F, 1.0F, 0.0F };
	/// Legacy encoded object identifier fallback when an object was hit.
	std::uint64_t object_id = 0;
	/// Legacy component index fallback when the hit refers to a sub-object element.
	std::uint32_t component_index = 0;
	/// Typed document object id when the hit comes from document topology.
	quader::document::ObjectId document_object_id = quader::document::ObjectId::invalid();
	/// Typed mesh component id when the hit refers to mesh topology.
	SurfaceHitComponent component;
	/// Hit classification.
	SurfaceHitKind kind = SurfaceHitKind::Grid;
};

/// Toolkit-neutral camera projection used by tools that need screen-space math.
enum class ViewportCameraProjection {
	/// Perspective projection.
	Perspective,
	/// Orthographic projection.
	Orthographic,
};

/// Toolkit-neutral viewport camera snapshot for analytic tool picking/dragging.
struct ViewportCameraInput {
	/// Camera eye position in world space.
	quader::math::Vec3 eye{};
	/// Camera target point in world space.
	quader::math::Vec3 target{};
	/// Camera up vector in world space.
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	/// Camera forward vector in world space.
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
	/// Projection mode.
	ViewportCameraProjection projection = ViewportCameraProjection::Perspective;
	/// Perspective field of view in degrees.
	float fov_degrees = 60.0F;
	/// Orthographic view height in world units.
	float orthographic_size = 24.0F;
	/// Viewport-local pane size in physical pixels.
	quader::math::Vec2 viewport_size_pixels{ 1.0F, 1.0F };
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
	/// Keyboard modifiers active when the pointer event was produced.
	KeyboardModifiers modifiers;
	/// True when camera navigation owns the pointer.
	bool navigation_active = false;
	/// True when tools should snap placement to the active grid.
	bool snap_to_grid = true;
	/// Active grid size in world units.
	float grid_size = 1.0F;
	/// Optional world ray for grid and document hit testing.
	std::optional<ViewportRay> ray;
	/// Optional camera/pane data for tools that need reference screen-space projection.
	std::optional<ViewportCameraInput> camera;
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
