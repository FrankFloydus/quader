#pragma once

#include "document/object_id.hpp"

#include <QObject>

#include <vector>

class QItemSelection;
class QItemSelectionModel;

namespace quader::ui {

class DocumentItemModel;
class DocumentUiController;

class DocumentSelectionAdapter final : public QObject {
	Q_OBJECT

public:
	DocumentSelectionAdapter(DocumentItemModel &model,
			QItemSelectionModel &selection_model,
			DocumentUiController &documents,
			QObject *parent = nullptr);

public Q_SLOTS:
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
