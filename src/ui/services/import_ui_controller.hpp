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

/// Result of the UI import flow.
enum class ImportUiResult {
	NoImporterRegistered, ///< No import formats are registered.
	Canceled,             ///< User canceled the dialog.
	ImportFailed,         ///< Import service returned an error.
	ParsedNotApplied,     ///< File parsed but document application is not wired yet.
};

/// Coordinates import file dialogs, import service calls, and notifications.
class ImportUiController final {
public:
	/// Construct over non-owning dialog, I/O, and notification services.
	ImportUiController(IFileDialogService &dialogs,
			const quader::io::ImportExportRegistry &registry,
			const quader::io::ImportService &import_service,
			NotificationService &notifications) noexcept;

	/// Return file-dialog filters derived from registered import formats.
	[[nodiscard]] QList<FileDialogFilter> import_filters() const;
	/// Run the open-scene dialog and parse the selected file.
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

/// Convert import file format descriptors into Qt file-dialog filters.
[[nodiscard]] QList<FileDialogFilter> import_filters_from_formats(
		const std::vector<quader::io::FileFormatDescriptor> &formats);

} // namespace quader::ui
