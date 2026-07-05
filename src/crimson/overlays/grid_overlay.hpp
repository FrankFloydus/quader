#pragma once

#include "crimson/frame/frame_snapshot.hpp"
#include "crimson/overlays/overlay_command.hpp"

namespace crimson {

[[nodiscard]] GridOverlayCommand make_grid_overlay_for_view(const RenderView &view);
[[nodiscard]] OverlayCommand make_grid_overlay_command(const GridOverlayCommand &grid, std::uint32_t payload_offset);

} // namespace crimson
