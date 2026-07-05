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
