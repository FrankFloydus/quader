#pragma once

#include "crimson/frame/frame_stats.hpp"
#include "crimson/picking/picking.hpp"

#include <vector>

namespace crimson {

struct FrameRenderResult {
	FrameStats stats;
	std::vector<PickingResult> completed_picking_results;
};

} // namespace crimson
