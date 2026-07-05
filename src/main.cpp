/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "app/application.hpp"

#include <QApplication>

int main(int argc, char *argv[]) {
	QApplication qt_app(argc, argv);

	quader::app::Application app(qt_app);
	return app.run();
}
