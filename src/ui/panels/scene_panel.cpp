#include "ui/panels/scene_panel.hpp"

#include "ui/models/document_item_model.hpp"
#include "ui/models/document_selection_adapter.hpp"
#include "ui/services/document_ui_controller.hpp"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>

namespace quader::ui {

ScenePanel::ScenePanel(PanelContext context, QWidget *parent) : QWidget(parent), context_(context) {
	model_ = new DocumentItemModel(context_.ui.documents, this);

	tree_view_ = new QTreeView(this);
	tree_view_->setModel(model_);
	tree_view_->setRootIsDecorated(false);
	tree_view_->setUniformRowHeights(true);
	tree_view_->setAlternatingRowColors(true);
	tree_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
	tree_view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
	tree_view_->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
	tree_view_->header()->setStretchLastSection(true);

	selection_adapter_ = new DocumentSelectionAdapter(*model_, *tree_view_->selectionModel(), context_.ui.documents, this);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(tree_view_);
}

DocumentItemModel &ScenePanel::model() noexcept {
	return *model_;
}

QTreeView &ScenePanel::tree_view() noexcept {
	return *tree_view_;
}

} // namespace quader::ui
