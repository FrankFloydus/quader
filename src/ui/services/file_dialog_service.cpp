/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/services/file_dialog_service.hpp"

#include <QFileDialog>

namespace quader::ui {
namespace {

[[nodiscard]] QString name_filter_from_filter(const FileDialogFilter &filter) {
	if (filter.patterns.empty()) {
		return filter.label;
	}

	return QStringLiteral("%1 (%2)").arg(filter.label, filter.patterns.join(QStringLiteral(" ")));
}

[[nodiscard]] QStringList name_filters_from_filters(const QList<FileDialogFilter> &filters) {
	QStringList name_filters;
	name_filters.reserve(filters.size());
	for (const auto &filter : filters) {
		name_filters.push_back(name_filter_from_filter(filter));
	}

	return name_filters;
}

} // namespace

std::optional<QString> QtFileDialogService::choose_open_file(
		QWidget *parent,
		const OpenFileDialogRequest &request) {
	QFileDialog dialog(parent, request.title, request.initial_directory);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	dialog.setFileMode(QFileDialog::ExistingFile);

	const QStringList kNameFilters = name_filters_from_filters(request.filters);
	if (!kNameFilters.empty()) {
		dialog.setNameFilters(kNameFilters);
	}

	if (dialog.exec() != QDialog::Accepted) {
		return std::nullopt;
	}

	const QStringList kFiles = dialog.selectedFiles();
	if (kFiles.empty()) {
		return std::nullopt;
	}

	return kFiles.front();
}

std::optional<QString> QtFileDialogService::choose_save_file(
		QWidget *parent,
		const SaveFileDialogRequest &request) {
	QFileDialog dialog(parent, request.title, request.initial_directory);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setFileMode(QFileDialog::AnyFile);
	if (!request.default_suffix.isEmpty()) {
		dialog.setDefaultSuffix(request.default_suffix);
	}

	const QStringList kNameFilters = name_filters_from_filters(request.filters);
	if (!kNameFilters.empty()) {
		dialog.setNameFilters(kNameFilters);
	}

	if (dialog.exec() != QDialog::Accepted) {
		return std::nullopt;
	}

	const QStringList kFiles = dialog.selectedFiles();
	if (kFiles.empty()) {
		return std::nullopt;
	}

	return kFiles.front();
}

} // namespace quader::ui
