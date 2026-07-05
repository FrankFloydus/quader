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
class QPlainTextEdit;
class QPushButton;
class QShowEvent;
class QTreeView;

namespace quader::ui {

class DiagnosticsItemModel;

/// Dockable panel that displays renderer diagnostics and frame stats.
class DiagnosticsPanel final : public QWidget {
	Q_OBJECT

public:
	/// Construct the diagnostics panel with non-owning UI service context.
	explicit DiagnosticsPanel(PanelContext context, QWidget *parent = nullptr);

	/// Return the diagnostics model owned by the panel.
	[[nodiscard]] DiagnosticsItemModel &model() noexcept;
	/// Return the tree view owned by the panel.
	[[nodiscard]] QTreeView &tree_view() noexcept;

protected:
	/// Refresh diagnostics when the panel is shown.
	void showEvent(QShowEvent *event) override;

private Q_SLOTS:
	void refresh_from_service();
	void copy_selected_rows();
	void copy_all_rows();

private:
	PanelContext context_;
	DiagnosticsItemModel *model_ = nullptr;
	QTreeView *tree_view_ = nullptr;
	QLabel *summary_label_ = nullptr;
	QPlainTextEdit *frame_graph_view_ = nullptr;
	QPushButton *refresh_button_ = nullptr;
	QPushButton *copy_selected_button_ = nullptr;
	QPushButton *copy_all_button_ = nullptr;
};

} // namespace quader::ui
