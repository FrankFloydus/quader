/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/panels/properties_panel.hpp"

#include "ui/delegates/property_value_delegate.hpp"
#include "ui/models/property_item_model.hpp"
#include "ui/services/document_ui_controller.hpp"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>

namespace quader::ui {

PropertiesPanel::PropertiesPanel(PanelContext context, QWidget *parent) : QWidget(parent), context_(context) {
	model_ = new PropertyItemModel(context_.ui.documents, this);
	delegate_ = new PropertyValueDelegate(this);

	empty_label_ = new QLabel(QStringLiteral("No selection"), this);
	empty_label_->setAlignment(Qt::AlignCenter);

	table_view_ = new QTableView(this);
	table_view_->setModel(model_);
	table_view_->setItemDelegateForColumn(static_cast<int>(PropertyItemColumn::Value), delegate_);
	table_view_->setAlternatingRowColors(true);
	table_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
	table_view_->setSelectionMode(QAbstractItemView::SingleSelection);
	table_view_->verticalHeader()->setVisible(false);
	table_view_->horizontalHeader()->setStretchLastSection(true);
	table_view_->horizontalHeader()->setMinimumSectionSize(72);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(empty_label_);
	layout->addWidget(table_view_);

	connect(model_, &QAbstractItemModel::modelReset, this, &PropertiesPanel::update_empty_state);
	connect(model_, &QAbstractItemModel::rowsInserted, this, &PropertiesPanel::update_empty_state);
	connect(model_, &QAbstractItemModel::rowsRemoved, this, &PropertiesPanel::update_empty_state);
	update_empty_state();
}

PropertyItemModel &PropertiesPanel::model() noexcept {
	return *model_;
}

QTableView &PropertiesPanel::table_view() noexcept {
	return *table_view_;
}

void PropertiesPanel::update_empty_state() {
	const bool kHasRows = model_ != nullptr && model_->rowCount() > 0;
	empty_label_->setVisible(!kHasRows);
	table_view_->setVisible(kHasRows);
}

} // namespace quader::ui
