/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/qt_app/qt_application_bootstrap.hpp"

#include <QApplication>

namespace quader::ui {

void apply_qt_application_metadata(QApplication &app, const QtApplicationMetadata &metadata) {
	app.setApplicationName(metadata.application_name);
	app.setOrganizationName(metadata.organization_name);
}

} // namespace quader::ui
