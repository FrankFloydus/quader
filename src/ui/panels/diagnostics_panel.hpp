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

class DiagnosticsPanel final : public QWidget {
	Q_OBJECT

public:
	explicit DiagnosticsPanel(PanelContext context, QWidget *parent = nullptr);

	[[nodiscard]] DiagnosticsItemModel &model() noexcept;
	[[nodiscard]] QTreeView &tree_view() noexcept;

protected:
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
