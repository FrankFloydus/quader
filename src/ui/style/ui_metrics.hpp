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

namespace quader::ui {

/// Centralized V1 UI spacing and icon metrics.
struct UiMetrics {
	/// Default outer margin in pixels.
	int margin = 6;
	/// Default intra-control spacing in pixels.
	int spacing = 8;
	/// Spacing between logical sections in pixels.
	int section_spacing = 8;
	/// Small icon size in pixels.
	int small_icon = 16;
	/// Toolbar icon size in pixels.
	int toolbar_icon = 24;
};

} // namespace quader::ui
