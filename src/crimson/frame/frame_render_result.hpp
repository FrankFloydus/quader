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

#include "crimson/frame/frame_stats.hpp"
#include "crimson/picking/picking.hpp"

#include <vector>

namespace crimson {

/// CPU-side result produced after submitting one renderer frame.
struct FrameRenderResult {
	/// Aggregated frame statistics.
	FrameStats stats;
	/// Picking requests completed by this frame submission.
	std::vector<PickingResult> completed_picking_results;
};

} // namespace crimson
