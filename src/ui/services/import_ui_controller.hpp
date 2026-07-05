#pragma once

#include "ui/services/file_dialog_service.hpp"

#include <QList>
#include <QString>

#include <filesystem>
#include <vector>

class QWidget;

namespace quader::io {
class ImportExportRegistry;
class ImportService;
struct FileFormatDescriptor;
struct IoDiagnostic;
struct IoDiagnosticList;
struct IoError;
} // namespace quader::io

namespace quader::ui {

class NotificationService;

enum class ImportUiResult {
	NoImporterRegistered,
	Canceled,
	ImportFailed,
	ParsedNotApplied,
};

class ImportUiController final {
public:
	ImportUiController(IFileDialogService &dialogs,
			const quader::io::ImportExportRegistry &registry,
			const quader::io::ImportService &import_service,
			NotificationService &notifications) noexcept;

	[[nodiscard]] QList<FileDialogFilter> import_filters() const;
	[[nodiscard]] ImportUiResult open_scene(QWidget *parent);

private:
	[[nodiscard]] OpenFileDialogRequest make_open_request() const;
	void present_import_failure(const quader::io::IoError &error);
	void present_import_warnings(const quader::io::IoDiagnosticList &diagnostics);
	void present_parsed_not_applied(const std::filesystem::path &path);

	IFileDialogService &dialogs_;
	const quader::io::ImportExportRegistry &registry_;
	const quader::io::ImportService &import_service_;
	NotificationService &notifications_;
};

[[nodiscard]] QList<FileDialogFilter> import_filters_from_formats(
		const std::vector<quader::io::FileFormatDescriptor> &formats);

} // namespace quader::ui
