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

#include <cstdint>

#include <QString>

namespace quader::ui {

/// Stable identifiers for dockable panels.
enum class PanelId : std::uint16_t {
	Properties,  ///< Properties panel.
	Scene,       ///< Scene hierarchy panel.
	Diagnostics, ///< Renderer diagnostics panel.
};

/// Return the user-visible panel name.
[[nodiscard]] QString panel_id_name(PanelId id);
/// Return the stable Qt object name used for workspace persistence.
[[nodiscard]] QString panel_object_name(PanelId id);

} // namespace quader::ui
