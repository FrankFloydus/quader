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

#include "ui/panels/panel_context.hpp"

#include <QWidget>

class QLabel;
class QTableView;

namespace quader::ui {

class PropertyItemModel;
class PropertyValueDelegate;

/// Dockable panel that displays editable properties for current selection.
class PropertiesPanel final : public QWidget {
	Q_OBJECT

public:
	/// Construct the properties panel with non-owning UI service context.
	explicit PropertiesPanel(PanelContext context, QWidget *parent = nullptr);

	/// Return the property model owned by the panel.
	[[nodiscard]] PropertyItemModel &model() noexcept;
	/// Return the table view owned by the panel.
	[[nodiscard]] QTableView &table_view() noexcept;

private:
	void update_empty_state();

	PanelContext context_;
	PropertyItemModel *model_ = nullptr;
	QTableView *table_view_ = nullptr;
	PropertyValueDelegate *delegate_ = nullptr;
	QLabel *empty_label_ = nullptr;
};

} // namespace quader::ui
