#include "ui/services/import_ui_controller.hpp"

#include "io/file_format.hpp"
#include "io/import_export_registry.hpp"
#include "io/import_service.hpp"
#include "io/io_error.hpp"
#include "ui/services/notification_service.hpp"

#include <string>
#include <utility>

namespace quader::ui {
namespace {

[[nodiscard]] QString display_name_for_format(const quader::io::FileFormatDescriptor &format) {
	if (!format.display_name.empty()) {
		return QString::fromStdString(format.display_name);
	}

	return QString::fromStdString(format.id);
}

[[nodiscard]] QString pattern_for_extension(const std::string &extension) {
	const auto kNormalized = quader::io::normalize_extension(extension);
	if (kNormalized.empty()) {
		return {};
	}

	return QStringLiteral("*.%1").arg(QString::fromStdString(kNormalized));
}

void append_unique_pattern(QStringList &patterns, QString pattern) {
	if (pattern.isEmpty() || patterns.contains(pattern)) {
		return;
	}

	patterns.push_back(std::move(pattern));
}

[[nodiscard]] QString detail_from_error(const quader::io::IoError &error) {
	if (!error.diagnostic.message.empty()) {
		return QString::fromStdString(error.diagnostic.message);
	}

	for (const auto &diagnostic : error.related) {
		if (!diagnostic.message.empty()) {
			return QString::fromStdString(diagnostic.message);
		}
	}

	return QStringLiteral("The selected file could not be imported.");
}

[[nodiscard]] QString first_warning_detail(const quader::io::IoDiagnosticList &diagnostics) {
	for (const auto &diagnostic : diagnostics) {
		if (diagnostic.severity == quader::io::IoSeverity::Warning && !diagnostic.message.empty()) {
			return QString::fromStdString(diagnostic.message);
		}
	}

	return {};
}

[[nodiscard]] std::filesystem::path path_from_qstring(const QString &path) {
#ifdef _WIN32
	return std::filesystem::path(path.toStdWString());
#else
	return std::filesystem::path(path.toStdString());
#endif
}

} // namespace

ImportUiController::ImportUiController(
		IFileDialogService &dialogs,
		const quader::io::ImportExportRegistry &registry,
		const quader::io::ImportService &import_service,
		NotificationService &notifications) noexcept
		: dialogs_(dialogs),
		  registry_(registry),
		  import_service_(import_service),
		  notifications_(notifications) {
}

QList<FileDialogFilter> ImportUiController::import_filters() const {
	return import_filters_from_formats(registry_.import_formats());
}

ImportUiResult ImportUiController::open_scene(QWidget *parent) {
	if (registry_.import_formats().empty()) {
		notifications_.show_warning(NotificationMessage{
				NotificationSeverity::Warning,
				QStringLiteral("No importer registered"),
				QStringLiteral("Open is unavailable until a production scene importer is registered."),
				5000,
		});
		return ImportUiResult::NoImporterRegistered;
	}

	const auto kSelected = dialogs_.choose_open_file(parent, make_open_request());
	if (!kSelected.has_value()) {
		return ImportUiResult::Canceled;
	}

	const std::filesystem::path kSelectedPath = path_from_qstring(*kSelected);
	auto imported = import_service_.import_file(quader::io::ImportRequest{ kSelectedPath, {} });
	if (!imported) {
		present_import_failure(imported.error());
		return ImportUiResult::ImportFailed;
	}

	present_import_warnings(imported.value().diagnostics);
	present_parsed_not_applied(kSelectedPath);
	return ImportUiResult::ParsedNotApplied;
}

OpenFileDialogRequest ImportUiController::make_open_request() const {
	return OpenFileDialogRequest{
		QStringLiteral("Open Scene"),
		{},
		import_filters(),
	};
}

void ImportUiController::present_import_failure(const quader::io::IoError &error) {
	notifications_.show_warning(NotificationMessage{
			NotificationSeverity::Warning,
			QStringLiteral("Import failed"),
			detail_from_error(error),
			7000,
	});
}

void ImportUiController::present_import_warnings(
		const quader::io::IoDiagnosticList &diagnostics) {
	const QString kDetail = first_warning_detail(diagnostics);
	if (kDetail.isEmpty()) {
		return;
	}

	notifications_.show_warning(NotificationMessage{
			NotificationSeverity::Warning,
			QStringLiteral("Import warning"),
			kDetail,
			7000,
	});
}

void ImportUiController::present_parsed_not_applied(const std::filesystem::path &path) {
	notifications_.show_warning(NotificationMessage{
			NotificationSeverity::Warning,
			QStringLiteral("Import parsed"),
			QStringLiteral("%1 parsed successfully, but applying imported content is not wired yet.")
					.arg(QString::fromStdString(path.filename().string())),
			7000,
	});
}

QList<FileDialogFilter> import_filters_from_formats(
		const std::vector<quader::io::FileFormatDescriptor> &formats) {
	QList<FileDialogFilter> filters;
	QStringList all_patterns;
	QList<FileDialogFilter> format_filters;

	for (const auto &format : formats) {
		QStringList patterns;
		for (const auto &extension : format.extensions) {
			const QString kPattern = pattern_for_extension(extension);
			append_unique_pattern(patterns, kPattern);
			append_unique_pattern(all_patterns, kPattern);
		}

		if (patterns.empty()) {
			continue;
		}

		format_filters.push_back(FileDialogFilter{
				display_name_for_format(format),
				patterns,
		});
	}

	if (!all_patterns.empty()) {
		filters.push_back(FileDialogFilter{
				QStringLiteral("All Supported Files"),
				all_patterns,
		});
	}

	for (const auto &filter : format_filters) {
		filters.push_back(filter);
	}

	return filters;
}

} // namespace quader::ui
