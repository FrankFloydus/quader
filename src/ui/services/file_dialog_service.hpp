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

#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

class QWidget;

namespace quader::ui {

/// One file-dialog filter entry.
struct FileDialogFilter {
	/// User-visible filter label.
	QString label;
	/// Glob patterns accepted by the filter.
	QStringList patterns;
};

/// Request data for opening one file.
struct OpenFileDialogRequest {
	/// Dialog title.
	QString title;
	/// Initial directory path.
	QString initial_directory;
	/// Selectable file filters.
	QList<FileDialogFilter> filters;
};

/// Request data for saving one file.
struct SaveFileDialogRequest {
	/// Dialog title.
	QString title;
	/// Initial directory path.
	QString initial_directory;
	/// Selectable file filters.
	QList<FileDialogFilter> filters;
	/// Default suffix applied by the dialog.
	QString default_suffix;
};

/// Abstract file dialog service for tests and platform-specific UI.
class IFileDialogService {
public:
	/// Destroy the service.
	virtual ~IFileDialogService() = default;

	/// Show an open-file dialog and return the selected path, or empty on cancel.
	[[nodiscard]] virtual std::optional<QString> choose_open_file(
			QWidget *parent,
			const OpenFileDialogRequest &request) = 0;
	/// Show a save-file dialog and return the selected path, or empty on cancel.
	[[nodiscard]] virtual std::optional<QString> choose_save_file(
			QWidget *parent,
			const SaveFileDialogRequest &request) = 0;
};

/// Qt-backed file dialog service.
class QtFileDialogService final : public IFileDialogService {
public:
	/// Show Qt's open-file dialog.
	[[nodiscard]] std::optional<QString> choose_open_file(
			QWidget *parent,
			const OpenFileDialogRequest &request) override;
	/// Show Qt's save-file dialog.
	[[nodiscard]] std::optional<QString> choose_save_file(
			QWidget *parent,
			const SaveFileDialogRequest &request) override;
};

} // namespace quader::ui
