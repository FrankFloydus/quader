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

class QTreeView;

namespace quader::ui {

class DocumentItemModel;
class DocumentSelectionAdapter;

/// Dockable panel that presents the document object hierarchy.
class ScenePanel final : public QWidget {
	Q_OBJECT

public:
	/// Construct the scene panel with non-owning UI service context.
	explicit ScenePanel(PanelContext context, QWidget *parent = nullptr);

	/// Return the document item model owned by the panel.
	[[nodiscard]] DocumentItemModel &model() noexcept;
	/// Return the tree view owned by the panel.
	[[nodiscard]] QTreeView &tree_view() noexcept;

private:
	PanelContext context_;
	DocumentItemModel *model_ = nullptr;
	QTreeView *tree_view_ = nullptr;
	DocumentSelectionAdapter *selection_adapter_ = nullptr;
};

} // namespace quader::ui
