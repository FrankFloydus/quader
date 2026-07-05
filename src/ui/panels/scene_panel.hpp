#pragma once

#include "ui/panels/panel_context.hpp"

#include <QWidget>

class QTreeView;

namespace quader::ui {

class DocumentItemModel;
class DocumentSelectionAdapter;

class ScenePanel final : public QWidget {
	Q_OBJECT

public:
	explicit ScenePanel(PanelContext context, QWidget *parent = nullptr);

	[[nodiscard]] DocumentItemModel &model() noexcept;
	[[nodiscard]] QTreeView &tree_view() noexcept;

private:
	PanelContext context_;
	DocumentItemModel *model_ = nullptr;
	QTreeView *tree_view_ = nullptr;
	DocumentSelectionAdapter *selection_adapter_ = nullptr;
};

} // namespace quader::ui
