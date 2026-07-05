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

#include <QString>

class QApplication;

namespace quader::ui {

/// Qt application metadata applied before settings and widgets are used.
struct QtApplicationMetadata {
	/// Application name reported to Qt.
	QString application_name;
	/// Organization name reported to Qt.
	QString organization_name;
};

/// Apply Qt application metadata from app configuration.
void apply_qt_application_metadata(QApplication &app, const QtApplicationMetadata &metadata);

} // namespace quader::ui
