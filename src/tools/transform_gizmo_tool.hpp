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
#include "document/transform.hpp"
#include "math/vec2.hpp"
#include "math/vec3.hpp"
#include "tools/editor_input_event.hpp"
#include "tools/tool.hpp"
#include "tools/tool_preview.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace quader::tools {

/// Transform operation implemented by a viewport gizmo tool.
enum class TransformGizmoMode {
	/// Translate selected objects.
	Move,
	/// Rotate selected objects.
	Rotate,
	/// Scale selected objects.
	Scale,
};

/// Principal axis addressed by a transform gizmo handle.
enum class TransformGizmoAxis : std::uint8_t {
	/// X axis.
	X = 0,
	/// Y axis.
	Y = 1,
	/// Z axis.
	Z = 2,
	/// XY plane.
	Xy = 3,
	/// XZ plane.
	Xz = 4,
	/// YZ plane.
	Yz = 5,
	/// All axes.
	All = 6,
	/// No axis.
	None = 255,
};

/// Transform gizmo handle family.
enum class TransformGizmoHandleKind {
	/// No handle is selected.
	None,
	/// Axis handle.
	Axis,
	/// Two-axis planar handle. `axis` stores the plane axis.
	Plane,
	/// Rotation ring handle.
	RotationRing,
	/// Center handle for Quader uniform scale.
	Center,
};

/// Stable handle descriptor used for hover, drag, and tests.
struct TransformGizmoHandle {
	/// Handle family.
	TransformGizmoHandleKind kind = TransformGizmoHandleKind::None;
	/// Axis for axis/ring handles, plane axis for planar handles, or All for center.
	TransformGizmoAxis axis = TransformGizmoAxis::X;

	/// Compare handles by value.
	friend bool operator==(const TransformGizmoHandle &, const TransformGizmoHandle &) = default;
};

/// Narrow state snapshot exposed for tests and diagnostics.
struct TransformGizmoToolState {
	/// True while a drag session is active.
	bool dragging = false;
	/// Hovered handle, when any.
	TransformGizmoHandle hover_handle;
	/// Active drag handle, when any.
	TransformGizmoHandle active_handle;
	/// Current shared pivot in world space.
	quader::math::Vec3 pivot;
	/// Current camera-scaled gizmo size in world units.
	float gizmo_scale = 1.0F;
	/// Number of live document object targets.
	std::size_t target_count = 0U;
	/// View index used for source-view preview overlays.
	std::uint32_t view_index = 0U;
};

/// Godot-style viewport transform gizmo for document object transforms.
class TransformGizmoTool final : public ITool {
public:
	/**
	 * Construct a transform gizmo tool from a Move, Rotate, or Scale tool id.
	 *
	 * @param id Tool id represented by this gizmo.
	 */
	explicit TransformGizmoTool(ToolId id) noexcept;

	/// Return the stable tool id.
	[[nodiscard]] ToolId id() const noexcept override;
	/// Reset transient interaction state when activated.
	void activate(ToolContext &context) override;
	/// Cancel any drag and clear preview state when deactivated.
	void deactivate(ToolContext &context) override;
	/// Cancel the current drag without mutating the document.
	void cancel(ToolContext &context) override;

	/// Handle viewport pointer input for hover, picking, dragging, and commit.
	[[nodiscard]] bool on_pointer_event(const PointerEvent &event, ToolContext &context) override;
	/// Handle Escape cancellation.
	[[nodiscard]] bool on_key_event(const KeyEvent &event, ToolContext &context) override;
	/// Return the current gizmo preview payload.
	[[nodiscard]] ToolPreview preview() const override;

	/// Return the configured transform mode.
	[[nodiscard]] TransformGizmoMode mode() const noexcept;
	/// Return the current state snapshot.
	[[nodiscard]] const TransformGizmoToolState &state() const noexcept;

private:
	struct TargetSnapshot {
		quader::document::ObjectId object;
		quader::document::Transform original;
		quader::math::Vec3 local_min;
		quader::math::Vec3 local_max;
		bool has_local_bounds = false;
	};

	[[nodiscard]] bool refresh_targets(const ToolContext &context, const PointerEvent &event);
	[[nodiscard]] std::optional<TransformGizmoHandle> pick_handle(const PointerEvent &event) const;
	[[nodiscard]] bool begin_drag(const PointerEvent &event, ToolContext &context);
	[[nodiscard]] bool update_drag(const PointerEvent &event, ToolContext &context);
	[[nodiscard]] bool commit_drag(ToolContext &context);
	[[nodiscard]] bool apply_preview_transforms(ToolContext &context);
	[[nodiscard]] bool restore_original_transforms(ToolContext &context);
	void cancel_drag(ToolContext &context);
	void clear_drag_state();
	void reset();
	void rebuild_preview();

	ToolId id_ = ToolId::Move;
	TransformGizmoMode mode_ = TransformGizmoMode::Move;
	TransformGizmoToolState state_;
	ToolPreview preview_;
	std::vector<TargetSnapshot> targets_;
	std::vector<quader::document::Transform> preview_transforms_;
	quader::tools::ViewportRay drag_start_ray_;
	quader::math::Vec2 drag_start_screen_;
	quader::math::Vec3 drag_start_pivot_;
	std::optional<quader::tools::ViewportCameraInput> last_camera_;
	std::optional<quader::tools::ViewportCameraInput> drag_camera_;
	quader::math::Vec2 drag_screen_axis_{ 1.0F, 0.0F };
	bool drag_has_screen_axis_ = false;
	float drag_axis_start_distance_ = 0.0F;
	bool drag_has_axis_start_distance_ = false;
	float drag_rotate_last_angle_radians_ = 0.0F;
	float drag_rotate_accumulated_degrees_ = 0.0F;
	float drag_rotate_view_sign_ = 1.0F;
	bool drag_rotate_edge_on_ = false;
	quader::math::Vec2 drag_rotate_edge_axis_{ 1.0F, 0.0F };
	float drag_rotate_edge_radius_pixels_ = 1.0F;
	quader::math::Vec3 drag_rotate_start_direction_;
	quader::math::Vec3 drag_rotate_current_position_;
	bool drag_rotate_indicator_ = false;
	float drag_start_scale_ = 1.0F;
	std::string status_text_;
};

} // namespace quader::tools
