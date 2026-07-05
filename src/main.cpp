#include "app/application.hpp"

#include <QApplication>

int main(int argc, char *argv[]) {
	QApplication qt_app(argc, argv);

	quader::app::Application app(qt_app);
	return app.run();
}
