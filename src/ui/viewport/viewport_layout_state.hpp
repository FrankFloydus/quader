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

#include "ui/viewport/viewport_types.hpp"

namespace quader::ui {

/// Stores split fractions and derives viewport pane rectangles.
class ViewportLayoutState final {
public:
	/// Return the active layout mode.
	[[nodiscard]] ViewportLayoutMode mode() const noexcept;
	/// Set the active layout mode.
	void set_mode(ViewportLayoutMode mode) noexcept;

	/// Return true when quad layout is enabled.
	[[nodiscard]] bool quad_enabled() const noexcept;
	/// Enable or disable quad layout.
	void set_quad_enabled(bool enabled) noexcept;

	/// Return vertical split fraction in `[0, 1]`.
	[[nodiscard]] float vertical_split() const noexcept;
	/// Return horizontal split fraction in `[0, 1]`.
	[[nodiscard]] float horizontal_split() const noexcept;
	/// Store a clamped vertical split fraction.
	void set_vertical_split(float value) noexcept;
	/// Store a clamped horizontal split fraction.
	void set_horizontal_split(float value) noexcept;

	/// Return pane rectangles for `size`.
	[[nodiscard]] ViewportPaneArray panes_for(ViewportPixelSize size) const;
	/// Return active pane count.
	[[nodiscard]] int pane_count() const noexcept;
	/// Return the pane index under `point`, or `-1` when none.
	[[nodiscard]] int pane_at(ViewportPixelSize size, ViewportPoint point) const;

	/// Return the splitter handle under `point`.
	[[nodiscard]] ViewportSplitHandle splitter_at(ViewportPixelSize size, ViewportPoint point) const;
	/// Update split fraction from pointer position.
	void set_split_from_position(ViewportPixelSize size, ViewportSplitHandle handle, ViewportPoint point) noexcept;
	/// Return the vertical splitter rectangle.
	[[nodiscard]] ViewportRect vertical_splitter_rect(ViewportPixelSize size) const noexcept;
	/// Return the horizontal splitter rectangle.
	[[nodiscard]] ViewportRect horizontal_splitter_rect(ViewportPixelSize size) const noexcept;

private:
	[[nodiscard]] int clamped_split_start(int size, float fraction, int splitter_width) const noexcept;

	ViewportLayoutMode mode_ = ViewportLayoutMode::Single;
	float vertical_split_ = 0.5F;
	float horizontal_split_ = 0.5F;
};

} // namespace quader::ui
