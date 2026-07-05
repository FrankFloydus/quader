/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include <QApplication>
#include <QString>

#include <gtest/gtest.h>

int main(int argc, char *argv[]) {
	::testing::InitGoogleTest(&argc, argv);

	QApplication app(argc, argv);
	QApplication::setApplicationName(QStringLiteral("quader_qt_tests"));
	QApplication::setOrganizationName(QStringLiteral("QuaderTests"));

	return RUN_ALL_TESTS();
}
