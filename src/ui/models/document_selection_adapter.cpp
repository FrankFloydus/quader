/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/models/document_selection_adapter.hpp"

#include "commands/command_result.hpp"
#include "commands/selection_commands.hpp"
#include "document/document.hpp"
#include "document/selection.hpp"
#include "ui/models/document_item_model.hpp"
#include "ui/models/item_model_roles.hpp"
#include "ui/services/document_ui_controller.hpp"

#include <QItemSelection>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QSignalBlocker>
#include <QVariant>

#include <algorithm>
#include <memory>

namespace quader::ui {

DocumentSelectionAdapter::DocumentSelectionAdapter(
		DocumentItemModel &model, QItemSelectionModel &selection_model,
		DocumentUiController &documents, QObject *parent) : QObject(parent),
															model_(model),
															selection_model_(selection_model),
															documents_(documents) {
	connect(&selection_model_, &QItemSelectionModel::selectionChanged, this,
			&DocumentSelectionAdapter::on_view_selection_changed);
	connect(&documents_, &DocumentUiController::selection_changed, this,
			&DocumentSelectionAdapter::sync_from_document);

	sync_from_document();
}

void DocumentSelectionAdapter::sync_from_document() {
	syncing_ = true;
	const QSignalBlocker kBlocker(&selection_model_);
	selection_model_.clearSelection();

	const auto &document_selection = documents_.document().selection();
	if (document_selection.mode() != quader::document::SelectionMode::Object) {
		syncing_ = false;
		return;
	}

	QItemSelection selection;
	for (const auto kObject : document_selection.selected_objects()) {
		const QModelIndex kLeft = model_.index_for_object(
				kObject, static_cast<int>(DocumentItemColumn::Name));
		if (!kLeft.isValid()) {
			continue;
		}

		const QModelIndex kRight = model_.index_for_object(
				kObject, static_cast<int>(DocumentItemColumn::Kind));
		selection.select(kLeft, kRight.isValid() ? kRight : kLeft);
	}

	if (!selection.isEmpty()) {
		selection_model_.select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
		selection_model_.setCurrentIndex(selection.indexes().front(),
				QItemSelectionModel::NoUpdate);
	}

	syncing_ = false;
}

void DocumentSelectionAdapter::on_view_selection_changed(
		const QItemSelection &selected, const QItemSelection &deselected) {
	(void)selected;
	(void)deselected;
	if (syncing_) {
		return;
	}

	auto selected_ids = selected_object_ids_from_view();
	quader::document::Selection selection;
	auto selected_result = selection.set_objects(std::move(selected_ids));
	if (!selected_result) {
		return;
	}

	(void)documents_.execute_command(
			std::make_unique<quader::commands::SetSelectionCommand>(
					std::move(selection)),
			CommandFeedback::Silent);
}

std::vector<quader::document::ObjectId>
DocumentSelectionAdapter::selected_object_ids_from_view() const {
	std::vector<quader::document::ObjectId> object_ids;
	const auto kIndexes = selection_model_.selectedIndexes();
	object_ids.reserve(static_cast<std::size_t>(kIndexes.size()));

	for (const auto &index : kIndexes) {
		const auto kObject = index.data(document_item_roles::kObjectIdRole)
									 .value<quader::document::ObjectId>();
		if (kObject.is_valid()) {
			object_ids.push_back(kObject);
		}
	}

	std::sort(object_ids.begin(), object_ids.end());
	object_ids.erase(std::unique(object_ids.begin(), object_ids.end()),
			object_ids.end());
	return object_ids;
}

} // namespace quader::ui
