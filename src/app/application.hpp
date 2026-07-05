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

#include "app/application_config.hpp"

#include <memory>

class QApplication;

namespace quader::ui {
class MainWindow;
}

namespace quader::app {

struct AppServices;

/// Composition root that owns Qt-facing application lifetime.
class Application final {
public:
	/// Construct services, register tools/actions, and create the main window.
	Application(QApplication &qt_app, ApplicationConfig config = {});
	/// Destroy the main window and service graph.
	~Application();

	/// Show the main window and enter the Qt event loop.
	int run();

private:
	QApplication &qt_app_;
	ApplicationConfig config_;
	std::unique_ptr<AppServices> services_;
	std::unique_ptr<quader::ui::MainWindow> main_window_;
};

} // namespace quader::app
