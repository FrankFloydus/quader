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

#include "document/object_id.hpp"

#include <QObject>

#include <vector>

class QItemSelection;
class QItemSelectionModel;

namespace quader::ui {

class DocumentItemModel;
class DocumentUiController;

/// Synchronizes document object selection with a Qt item selection model.
class DocumentSelectionAdapter final : public QObject {
	Q_OBJECT

public:
	/// Construct an adapter over model, view selection, and document services.
	DocumentSelectionAdapter(DocumentItemModel &model,
			QItemSelectionModel &selection_model,
			DocumentUiController &documents,
			QObject *parent = nullptr);

public Q_SLOTS:
	/// Project document selection into the Qt selection model.
	void sync_from_document();

private:
	void on_view_selection_changed(const QItemSelection &selected,
			const QItemSelection &deselected);
	[[nodiscard]] std::vector<quader::document::ObjectId>
	selected_object_ids_from_view() const;

	DocumentItemModel &model_;
	QItemSelectionModel &selection_model_;
	DocumentUiController &documents_;
	bool syncing_ = false;
};

} // namespace quader::ui
