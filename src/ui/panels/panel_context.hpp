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

#include "ui/qt_app/ui_context.hpp"

namespace quader::ui {

/// Non-owning context passed to dock panels during construction.
struct PanelContext {
	/// Shared UI service context owned by the application.
	UiContext &ui;
};

} // namespace quader::ui
