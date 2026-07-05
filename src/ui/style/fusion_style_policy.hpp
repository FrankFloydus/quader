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

class QApplication;

namespace quader::ui {

/// Applies the V1 Qt Fusion style baseline.
class FusionStylePolicy final {
public:
	/// Apply Fusion style when available.
	[[nodiscard]] bool apply(QApplication &app) const;
};

} // namespace quader::ui
