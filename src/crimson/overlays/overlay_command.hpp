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

#include "crimson/color/color_space.hpp"
#include "crimson/renderer_types.hpp"
#include "crimson/scene/render_camera.hpp"
#include "crimson/scene/render_object.hpp"
#include "math/vec3.hpp"

#include <cstdint>
#include <limits>

namespace crimson {

/// Depth policy used when compositing editor overlays.
enum class OverlayDepthMode : std::uint8_t {
	DepthTested, ///< Hidden by scene depth.
	XRay,        ///< Blended through scene depth.
	AlwaysOnTop,///< Drawn without depth testing.
};

/// Overlay primitive payload type.
enum class OverlayPrimitive : std::uint8_t {
	Grid,           ///< Grid payload in `GridOverlayCommand`.
	LineList,       ///< Line segment payloads in `LineOverlaySegment`.
	SolidTriangles, ///< Triangle overlay payloads.
	PointList,      ///< Point handle payloads in `PointOverlayPrimitive`.
};

/// Semantic overlay role used by policy and render extraction code.
enum class OverlaySemanticRole : std::uint8_t {
	Generic,                ///< Legacy or caller-owned overlay without selection semantics.
	ToolPreview,            ///< Tool preview geometry owned by the active viewport tool.
	SourceWire,             ///< Component-mode source wire drawn crisp over the shaded mesh.
	SourceWireDepthStamp,   ///< Source-wire depth metadata; it does not create color draw batches.
	SourceVertex,           ///< Unselected source-wire vertex handle.
	SelectedObjectTopology, ///< Selected object topology wire in object selection mode.
	SelectedFaceFill,       ///< Filled selected face overlay.
	HoverFaceFill,          ///< Filled hovered face overlay.
	SelectedFaceEdge,       ///< Selected face boundary edge overlay.
	HoverFaceEdge,          ///< Hovered face boundary edge overlay.
	SelectedEdge,           ///< Selected edge handle overlay.
	HoverEdge,              ///< Hovered edge handle overlay.
	SelectedVertex,         ///< Selected vertex handle overlay.
	HoverVertex,            ///< Hovered vertex handle overlay.
};

/// Selection source category used for semantic overlay batching.
enum class OverlaySourceKind : std::uint8_t {
	Unknown,            ///< No source semantics were supplied.
	ObjectSelection,    ///< Object-mode selection topology.
	ComponentSelection, ///< Component-mode selected topology or handles.
	ComponentHover,     ///< Component-mode hovered topology or handles.
	SourceWire,         ///< Sticky source-wire owner topology.
	ToolPreview,        ///< Active tool preview overlay.
	Diagnostic,         ///< Diagnostics or debug overlay.
};

/// Sentinel for an unset topology component index.
inline constexpr std::uint32_t kInvalidOverlayElementIndex = std::numeric_limits<std::uint32_t>::max();

/// Stable semantic reference for an overlay primitive.
struct OverlayElementRef {
	/// Renderer object owning the referenced topology.
	RenderObjectId object_id = 0;
	/// Submesh index within the render object.
	std::uint32_t submesh_index = 0;
	/// Face index when the primitive represents or belongs to a face.
	std::uint32_t face_index = kInvalidOverlayElementIndex;
	/// Edge index when the primitive represents or belongs to an edge.
	std::uint32_t edge_index = kInvalidOverlayElementIndex;
	/// Vertex index when the primitive represents a vertex handle.
	std::uint32_t vertex_index = kInvalidOverlayElementIndex;
	/// First face incident to an edge primitive, when known.
	std::uint32_t incident_face_index0 = kInvalidOverlayElementIndex;
	/// Second face incident to an edge primitive, when known.
	std::uint32_t incident_face_index1 = kInvalidOverlayElementIndex;
};

/// Renderer command describing one overlay draw payload.
struct OverlayCommand {
	/// View index that owns the overlay.
	std::uint32_t view_index = 0;
	/// Payload primitive kind.
	OverlayPrimitive primitive = OverlayPrimitive::LineList;
	/// Overlay depth policy.
	OverlayDepthMode depth_mode = OverlayDepthMode::DepthTested;
	/// Command-level semantic role used when payloads leave their role as `Generic`.
	OverlaySemanticRole semantic_role = OverlaySemanticRole::Generic;
	/// Command-level source category used when payloads leave their source as `Unknown`.
	OverlaySourceKind source_kind = OverlaySourceKind::Unknown;
	/// Base shader used for the overlay.
	BaseShaderId base_shader = BaseShaderId::OverlayUnlit;
	/// Overlay color authored in sRGB UI space.
	ColorSrgb color_srgb{ 1.0F, 1.0F, 1.0F, 1.0F };
	/// Extra opacity multiplier.
	float opacity = 1.0F;
	/// Line thickness in physical pixels.
	float thickness_px = 1.0F;
	/// Offset into the matching overlay payload array.
	std::uint32_t payload_offset = 0;
	/// Number of payload records consumed by this command.
	std::uint32_t payload_count = 0;
};

/// Grid overlay payload consumed by the overlay renderer.
struct GridOverlayCommand {
	/// View index that owns the grid.
	std::uint32_t view_index = 0;
	/// Depth policy used when compositing this grid.
	OverlayDepthMode depth_mode = OverlayDepthMode::DepthTested;
	/// Grid plane origin in world space.
	quader::math::Vec3 origin{};
	/// First grid axis.
	quader::math::Vec3 u_axis{ 1.0F, 0.0F, 0.0F };
	/// Second grid axis.
	quader::math::Vec3 v_axis{ 0.0F, 0.0F, -1.0F };
	/// Grid plane width in world units.
	float plane_width = 1.0F;
	/// Grid plane height in world units.
	float plane_height = 1.0F;
	/// X-axis rotation in radians for shader-facing grid orientation.
	float rotation_x = 0.0F;
	/// Y-axis rotation in radians for shader-facing grid orientation.
	float rotation_y = 0.0F;
	/// Z-axis rotation in radians for shader-facing grid orientation.
	float rotation_z = 0.0F;
	/// Minor grid spacing in meters.
	float minor_spacing_m = 1.0F;
	/// Major grid spacing in meters.
	float major_spacing_m = 2.0F;
	/// Distance where grid fade begins.
	float fade_start_m = 96.0F;
	/// Distance where grid fade ends.
	float fade_end_m = 1536.0F;
	/// Minor line thickness scale.
	float minor_line_scale = 0.325F;
	/// Major line thickness scale.
	float major_line_scale = 0.250F;
	/// Axis line thickness scale.
	float axis_line_scale = 1.0F;
	/// Orthographic view height in meters.
	float orthographic_height_m = 1.0F;
	/// Viewport height in physical pixels.
	float viewport_height_px = 1.0F;
	/// Edge fade softness in meters.
	float edge_softness_m = 0.001F;
	/// Active camera far clip used for perspective far fade.
	float camera_far_plane_m = kDefaultCameraFarPlaneM;
	/// Perspective far-fade enable amount; orthographic grids use zero.
	float camera_far_fade = 0.0F;
	/// Minor line color in sRGB UI space.
	ColorSrgb minor_color{ 150.0F / 255.0F, 150.0F / 255.0F, 150.0F / 255.0F, 1.0F };
	/// Major line color in sRGB UI space.
	ColorSrgb major_color{ 210.0F / 255.0F, 210.0F / 255.0F, 210.0F / 255.0F, 1.0F };
	/// U-axis color in sRGB UI space.
	ColorSrgb axis_u_color{ 1.0F, 0.239F, 0.0F, 0.72F };
	/// V-axis color in sRGB UI space.
	ColorSrgb axis_v_color{ 0.059F, 0.612F, 1.0F, 0.72F };
};

/// World-space line segment payload for overlay line lists.
struct LineOverlaySegment {
	/// Segment start point.
	quader::math::Vec3 start;
	/// Segment end point.
	quader::math::Vec3 end;
	/// Segment semantic role; `Generic` inherits the owning command role.
	OverlaySemanticRole semantic_role = OverlaySemanticRole::Generic;
	/// Segment source category; `Unknown` inherits the owning command source.
	OverlaySourceKind source_kind = OverlaySourceKind::Unknown;
	/// Topology element represented by this segment, when any.
	OverlayElementRef element;
};

/// World-space filled triangle payload for selection face overlays.
struct TriangleOverlayPrimitive {
	/// First triangle vertex.
	quader::math::Vec3 a;
	/// Second triangle vertex.
	quader::math::Vec3 b;
	/// Third triangle vertex.
	quader::math::Vec3 c;
	/// Triangle semantic role; `Generic` inherits the owning command role.
	OverlaySemanticRole semantic_role = OverlaySemanticRole::Generic;
	/// Triangle source category; `Unknown` inherits the owning command source.
	OverlaySourceKind source_kind = OverlaySourceKind::Unknown;
	/// Topology element represented by this triangle, when any.
	OverlayElementRef element;
};

/// World-space point handle payload for vertex overlays.
struct PointOverlayPrimitive {
	/// Point center.
	quader::math::Vec3 position;
	/// Point handle size in physical pixels. Zero inherits `OverlayCommand::thickness_px`.
	float size_px = 0.0F;
	/// Point semantic role; `Generic` inherits the owning command role.
	OverlaySemanticRole semantic_role = OverlaySemanticRole::Generic;
	/// Point source category; `Unknown` inherits the owning command source.
	OverlaySourceKind source_kind = OverlaySourceKind::Unknown;
	/// Topology element represented by this point, when any.
	OverlayElementRef element;
};

/// Return the render queue corresponding to an overlay depth mode.
[[nodiscard]] RenderQueue render_queue_for_overlay_depth_mode(OverlayDepthMode depth_mode) noexcept;
/// Return true when a role is source-wire depth metadata instead of a color overlay.
[[nodiscard]] bool overlay_role_is_source_wire_depth_stamp(OverlaySemanticRole role) noexcept;
/// Return true when a role is a selected or hovered face fill.
[[nodiscard]] bool overlay_role_is_face_fill(OverlaySemanticRole role) noexcept;
/// Return true when a role must be rendered as two-sided geometry.
[[nodiscard]] bool overlay_role_renders_two_sided(OverlaySemanticRole role) noexcept;
/// Return true when a role represents the source wire itself.
[[nodiscard]] bool overlay_role_is_source_wire(OverlaySemanticRole role) noexcept;
/// Return true when a role represents selected or hovered component handles.
[[nodiscard]] bool overlay_role_is_component_handle(OverlaySemanticRole role) noexcept;
/// Return true when a role represents selected or hovered component edge handles.
[[nodiscard]] bool overlay_role_is_component_edge(OverlaySemanticRole role) noexcept;
/// Return true when a role uses the dedicated component edit-wire render path.
[[nodiscard]] bool overlay_role_is_component_edit_wire(OverlaySemanticRole role) noexcept;
/// Return true when a role represents a source, selected, or hovered vertex handle.
[[nodiscard]] bool overlay_role_is_vertex_handle(OverlaySemanticRole role) noexcept;
/// Return true when source category is component selected or hover data.
[[nodiscard]] bool overlay_source_kind_is_component(OverlaySourceKind source_kind) noexcept;
/// Resolve payload role inheritance from its owning command.
[[nodiscard]] OverlaySemanticRole effective_overlay_role(
		const OverlayCommand &command,
		OverlaySemanticRole payload_role) noexcept;
/// Resolve payload source inheritance from its owning command.
[[nodiscard]] OverlaySourceKind effective_overlay_source_kind(
		const OverlayCommand &command,
		OverlaySourceKind payload_source_kind) noexcept;
/// Resolve semantic overlay draw bucket. Component-sourced source-wire,
/// component edge lines, and component vertex handles use depth-tested
/// component overlay buckets.
[[nodiscard]] OverlayDepthMode effective_overlay_depth_mode(
		const OverlayCommand &command,
		OverlaySemanticRole role,
		OverlaySourceKind source_kind) noexcept;

} // namespace crimson
