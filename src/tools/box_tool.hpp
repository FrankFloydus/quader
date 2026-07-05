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
#include "tools/editor_input_event.hpp"
#include "tools/tool.hpp"
#include "tools/tool_preview.hpp"

#include <array>
#include <cstdint>

namespace quader::tools {

/// Interaction stage for the reference-style Box tool.
enum class BoxToolStage {
	/// The tool is showing the hover footprint and has no active drag.
	Idle,
	/// The user is dragging a footprint; release or a second click commits it.
	Footprint,
};

/// World-space plane and basis used to construct and snap a box footprint.
struct BoxConstructionPlane {
	/// User-facing plane origin, usually the hit point or world origin.
	quader::math::Vec3 origin;
	/// Grid snapping origin projected onto the construction plane.
	quader::math::Vec3 snap_origin;
	/// Normal direction for box height and surface placement.
	quader::math::Vec3 normal{ 0.0F, 1.0F, 0.0F };
	/// First in-plane footprint axis.
	quader::math::Vec3 axis_u{ 1.0F, 0.0F, 0.0F };
	/// Second in-plane footprint axis, signed to match the reference preview parity.
	quader::math::Vec3 axis_v{ 0.0F, 0.0F, 1.0F };
};

/// Mutable state for an in-progress Box tool interaction.
struct BoxToolState {
	/// Current interaction stage.
	BoxToolStage stage = BoxToolStage::Idle;
	/// Construction plane used by the current hover or drag.
	BoxConstructionPlane plane;
	/// Unsnapped drag start point on the construction plane.
	quader::math::Vec3 raw_start;
	/// Unsnapped drag end point on the construction plane.
	quader::math::Vec3 raw_end;
	/// Snapped footprint start corner.
	quader::math::Vec3 start;
	/// Snapped footprint opposite corner.
	quader::math::Vec3 end;
	/// Default commit height, measured along `plane.normal`.
	float height = 0.0F;
	/// View index that owns the active preview.
	std::uint32_t view_index = 0;
	/// Snapped active footprint corners.
	std::array<quader::math::Vec3, 4> footprint{};
	/// Snapped hover-cell corners while idle.
	std::array<quader::math::Vec3, 4> hover_cell{};
	/// True when the active footprint can produce a non-degenerate box.
	bool preview_valid = false;
};

/// Reference-style viewport Box tool with hover footprint, drag footprint, and undoable commit.
class BoxTool final : public ITool {
public:
	/// Return the stable Box tool id.
	[[nodiscard]] ToolId id() const noexcept override;

	/// Reset interaction state when the tool becomes active.
	void activate(ToolContext &context) override;
	/// Clear previews and interaction state when another tool becomes active.
	void deactivate(ToolContext &context) override;
	/// Cancel the current Box interaction without mutating the document.
	void cancel(ToolContext &context) override;

	/// Handle neutral viewport pointer events and commit through `ToolContext` when completed.
	[[nodiscard]] bool on_pointer_event(const PointerEvent &event, ToolContext &context) override;
	/// Handle Escape cancellation.
	[[nodiscard]] bool on_key_event(const KeyEvent &event, ToolContext &context) override;
	/// Return the current footprint overlay preview.
	[[nodiscard]] ToolPreview preview() const override;
	/// Return and clear a one-shot completion request after successful commit.
	[[nodiscard]] ToolCompletionRequest consume_completion_request() noexcept override;

	/// Return the current state for tests and narrow diagnostics.
	[[nodiscard]] const BoxToolState &state() const noexcept;

private:
	[[nodiscard]] bool update_hover(const PointerEvent &event);
	[[nodiscard]] bool begin_footprint(const PointerEvent &event);
	[[nodiscard]] bool update_footprint(const PointerEvent &event);
	[[nodiscard]] bool finish_footprint_and_commit(const PointerEvent &event, ToolContext &context);
	[[nodiscard]] bool commit(ToolContext &context);

	void reset();
	void rebuild_preview();

	BoxToolState state_;
	ToolPreview preview_;
	ToolCompletionRequest completion_request_ = ToolCompletionRequest::None;
};

/**
 * Build a stable construction plane from an origin and normal.
 *
 * @param origin Plane origin in world space.
 * @param normal Preferred plane normal.
 * @return Normalized construction plane with a reference-compatible in-plane basis.
 */
[[nodiscard]] BoxConstructionPlane make_box_construction_plane(
		quader::math::Vec3 origin,
		quader::math::Vec3 normal);

} // namespace quader::tools
