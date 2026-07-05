#pragma once

#include "app/application_config.hpp"

#include <memory>

class QApplication;

namespace quader::ui {
class MainWindow;
}

namespace quader::app {

struct AppServices;

class Application final {
public:
	Application(QApplication &qt_app, ApplicationConfig config = {});
	~Application();

	int run();

private:
	QApplication &qt_app_;
	ApplicationConfig config_;
	std::unique_ptr<AppServices> services_;
	std::unique_ptr<quader::ui::MainWindow> main_window_;
};

} // namespace quader::app
