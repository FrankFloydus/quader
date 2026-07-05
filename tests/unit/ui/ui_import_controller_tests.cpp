#include "io/import_export_registry.hpp"
#include "io/import_service.hpp"
#include "ui/services/file_dialog_service.hpp"
#include "ui/services/import_ui_controller.hpp"
#include "ui/services/notification_service.hpp"

#include <gtest/gtest.h>

#include <QApplication>
#include <QObject>

#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

class FakeFileDialogService final : public quader::ui::IFileDialogService {
public:
	std::optional<QString> choose_open_file(
			QWidget *,
			const quader::ui::OpenFileDialogRequest &request) override {
		++open_calls;
		last_open_request = request;
		return next_open_result;
	}

	std::optional<QString> choose_save_file(
			QWidget *,
			const quader::ui::SaveFileDialogRequest &request) override {
		++save_calls;
		last_save_request = request;
		return next_save_result;
	}

	int open_calls = 0;
	int save_calls = 0;
	quader::ui::OpenFileDialogRequest last_open_request;
	quader::ui::SaveFileDialogRequest last_save_request;
	std::optional<QString> next_open_result;
	std::optional<QString> next_save_result;
};

class FakeImporter final : public quader::io::IImporter {
public:
	explicit FakeImporter(quader::io::FileFormatDescriptor descriptor) : descriptor_(std::move(descriptor)) {
	}

	[[nodiscard]] quader::io::FileFormatDescriptor descriptor() const override {
		return descriptor_;
	}

	[[nodiscard]] bool can_import(const std::filesystem::path &path) const override {
		return quader::io::descriptor_supports_extension(descriptor_, path);
	}

	[[nodiscard]] quader::foundation::Result<quader::io::ImportResult, quader::io::IoError>
	import_file(const quader::io::ImportRequest &) const override {
		++import_calls;
		quader::io::ImportResult result;
		result.payload = quader::io::DocumentFragment{};
		return quader::foundation::Result<quader::io::ImportResult, quader::io::IoError>::success(
				std::move(result));
	}

	mutable int import_calls = 0;

private:
	quader::io::FileFormatDescriptor descriptor_;
};

struct NotificationCapture {
	std::vector<quader::ui::NotificationMessage> messages;
};

quader::io::FileFormatDescriptor fake_import_format() {
	return quader::io::FileFormatDescriptor{
		"fake",
		"Fake Scene",
		{ "fake" },
		true,
		true,
		false,
		false,
	};
}

quader::io::FileFormatDescriptor gltf_import_format() {
	return quader::io::FileFormatDescriptor{
		"gltf",
		"glTF 2.0",
		{ "gltf", ".glb" },
		true,
		true,
		false,
		false,
	};
}

void connect_capture(quader::ui::NotificationService &notifications,
		NotificationCapture &capture) {
	QObject::connect(&notifications,
			&quader::ui::NotificationService::notification_posted,
			&notifications,
			[&capture](const quader::ui::NotificationMessage &message) {
				capture.messages.push_back(message);
			});
}

TEST(UiImportController, ImportFilterGenerationPreservesRegistryOrderAndAggregatePatterns) {
	const QList<quader::ui::FileDialogFilter> kFilters = quader::ui::import_filters_from_formats({
			gltf_import_format(),
			fake_import_format(),
	});

	EXPECT_EQ(kFilters.size(), 3);
	EXPECT_EQ(kFilters[0].label, QStringLiteral("All Supported Files"));
	EXPECT_EQ(kFilters[0].patterns.size(), 3);
	EXPECT_EQ(kFilters[0].patterns[0], QStringLiteral("*.gltf"));
	EXPECT_EQ(kFilters[0].patterns[1], QStringLiteral("*.glb"));
	EXPECT_EQ(kFilters[0].patterns[2], QStringLiteral("*.fake"));
	EXPECT_EQ(kFilters[1].label, QStringLiteral("glTF 2.0"));
	EXPECT_EQ(kFilters[1].patterns[0], QStringLiteral("*.gltf"));
	EXPECT_EQ(kFilters[1].patterns[1], QStringLiteral("*.glb"));
	EXPECT_EQ(kFilters[2].label, QStringLiteral("Fake Scene"));
	EXPECT_EQ(kFilters[2].patterns[0], QStringLiteral("*.fake"));
}

TEST(UiImportController, NoRegisteredImporterReportsWarningWithoutOpeningDialog) {
	quader::io::ImportExportRegistry registry;
	quader::io::ImportService import_service{ registry };
	FakeFileDialogService dialogs;
	quader::ui::NotificationService notifications;
	NotificationCapture capture;
	connect_capture(notifications, capture);

	quader::ui::ImportUiController controller{ dialogs, registry, import_service, notifications };
	const auto kResult = controller.open_scene(nullptr);

	EXPECT_EQ(kResult, quader::ui::ImportUiResult::NoImporterRegistered);
	EXPECT_EQ(dialogs.open_calls, 0);
	EXPECT_EQ(capture.messages.size(), 1U);
	EXPECT_EQ(capture.messages.front().severity, quader::ui::NotificationSeverity::Warning);
	EXPECT_EQ(capture.messages.front().summary, QStringLiteral("No importer registered"));
}

TEST(UiImportController, CanceledOpenDialogDoesNotCallImporterOrPostFailure) {
	quader::io::ImportExportRegistry registry;
	auto importer = std::make_unique<FakeImporter>(fake_import_format());
	const auto *importer_ptr = importer.get();
	EXPECT_TRUE(registry.register_importer(std::move(importer)));
	quader::io::ImportService import_service{ registry };
	FakeFileDialogService dialogs;
	quader::ui::NotificationService notifications;
	NotificationCapture capture;
	connect_capture(notifications, capture);

	quader::ui::ImportUiController controller{ dialogs, registry, import_service, notifications };
	const auto kResult = controller.open_scene(nullptr);

	EXPECT_EQ(kResult, quader::ui::ImportUiResult::Canceled);
	EXPECT_EQ(dialogs.open_calls, 1);
	EXPECT_EQ(importer_ptr->import_calls, 0);
	EXPECT_TRUE(capture.messages.empty());
}

TEST(UiImportController, UnsupportedFormatPresentsDiagnosticWithoutImporterExecution) {
	quader::io::ImportExportRegistry registry;
	auto importer = std::make_unique<FakeImporter>(fake_import_format());
	const auto *importer_ptr = importer.get();
	EXPECT_TRUE(registry.register_importer(std::move(importer)));
	quader::io::ImportService import_service{ registry };
	FakeFileDialogService dialogs;
	dialogs.next_open_result = QStringLiteral("scene.unknown");
	quader::ui::NotificationService notifications;
	NotificationCapture capture;
	connect_capture(notifications, capture);

	quader::ui::ImportUiController controller{ dialogs, registry, import_service, notifications };
	const auto kResult = controller.open_scene(nullptr);

	EXPECT_EQ(kResult, quader::ui::ImportUiResult::ImportFailed);
	EXPECT_EQ(dialogs.open_calls, 1);
	EXPECT_EQ(importer_ptr->import_calls, 0);
	EXPECT_EQ(capture.messages.size(), 1U);
	EXPECT_EQ(capture.messages.front().severity, quader::ui::NotificationSeverity::Warning);
	EXPECT_EQ(capture.messages.front().summary, QStringLiteral("Import failed"));
	EXPECT_TRUE(capture.messages.front().detail.contains(QStringLiteral("unknown")));
}

} // namespace
