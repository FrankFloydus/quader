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

#include "crimson/frame/frame_snapshot.hpp"
#include "crimson/overlays/overlay_command.hpp"

namespace crimson {

/**
 * Build the default viewport grid payload for a render view.
 *
 * @param view Render view that determines grid orientation and scale.
 * @return Grid overlay payload for `view`.
 */
[[nodiscard]] GridOverlayCommand make_grid_overlay_for_view(const RenderView &view);
/**
 * Build an overlay command that references one grid payload.
 *
 * @param grid Grid payload to reference.
 * @param payload_offset Offset of `grid` in the frame grid payload array.
 * @return Overlay command for the grid payload.
 */
[[nodiscard]] OverlayCommand make_grid_overlay_command(const GridOverlayCommand &grid, std::uint32_t payload_offset);

} // namespace crimson
