#include "ui/panels/diagnostics_panel.hpp"

#include "ui/models/diagnostics_item_model.hpp"
#include "ui/services/notification_service.hpp"
#include "ui/services/viewport_diagnostics_service.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QModelIndex>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShowEvent>
#include <QStringList>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>

namespace quader::ui {

DiagnosticsPanel::DiagnosticsPanel(PanelContext context, QWidget *parent) : QWidget(parent), context_(context) {
	model_ = new DiagnosticsItemModel(context_.ui.viewport_diagnostics, this);

	summary_label_ = new QLabel(this);
	summary_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);

	tree_view_ = new QTreeView(this);
	tree_view_->setModel(model_);
	tree_view_->setAlternatingRowColors(true);
	tree_view_->setUniformRowHeights(true);
	tree_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
	tree_view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
	tree_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tree_view_->header()->setStretchLastSection(true);
	tree_view_->expand(model_->index(0, 0));

	frame_graph_view_ = new QPlainTextEdit(this);
	frame_graph_view_->setReadOnly(true);
	frame_graph_view_->setLineWrapMode(QPlainTextEdit::NoWrap);

	auto *tabs = new QTabWidget(this);
	tabs->addTab(tree_view_, QStringLiteral("Items"));
	tabs->addTab(frame_graph_view_, QStringLiteral("Frame Graph"));

	refresh_button_ = new QPushButton(QStringLiteral("Refresh"), this);
	copy_selected_button_ = new QPushButton(QStringLiteral("Copy Selected"), this);
	copy_all_button_ = new QPushButton(QStringLiteral("Copy All"), this);

	auto *buttons = new QHBoxLayout;
	buttons->addWidget(refresh_button_);
	buttons->addStretch(1);
	buttons->addWidget(copy_selected_button_);
	buttons->addWidget(copy_all_button_);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->addWidget(summary_label_);
	layout->addWidget(tabs, 1);
	layout->addLayout(buttons);

	connect(refresh_button_, &QPushButton::clicked, &context_.ui.viewport_diagnostics, &ViewportDiagnosticsService::refresh);
	connect(copy_selected_button_, &QPushButton::clicked, this, &DiagnosticsPanel::copy_selected_rows);
	connect(copy_all_button_, &QPushButton::clicked, this, &DiagnosticsPanel::copy_all_rows);
	connect(&context_.ui.viewport_diagnostics,
			&ViewportDiagnosticsService::summary_changed,
			this,
			&DiagnosticsPanel::refresh_from_service);
	connect(&context_.ui.viewport_diagnostics,
			&ViewportDiagnosticsService::diagnostics_unavailable,
			this,
			&DiagnosticsPanel::refresh_from_service);

	refresh_from_service();
}

DiagnosticsItemModel &DiagnosticsPanel::model() noexcept {
	return *model_;
}

QTreeView &DiagnosticsPanel::tree_view() noexcept {
	return *tree_view_;
}

void DiagnosticsPanel::showEvent(QShowEvent *event) {
	QWidget::showEvent(event);
	model_->reload_from_service();
	refresh_from_service();
	tree_view_->expand(model_->index(0, 0));
}

void DiagnosticsPanel::refresh_from_service() {
	const ViewportDiagnosticsSummary kSummary = context_.ui.viewport_diagnostics.summary();
	summary_label_->setText(kSummary.status_text);
	summary_label_->setToolTip(kSummary.tooltip_text);

	const auto &snapshot = context_.ui.viewport_diagnostics.latest_snapshot();
	frame_graph_view_->setPlainText(snapshot.has_value() ? snapshot->frame_graph_dump : QString());
}

void DiagnosticsPanel::copy_selected_rows() {
	const QModelIndexList kRows = tree_view_->selectionModel()->selectedRows(static_cast<int>(DiagnosticsItemColumn::Name));
	if (kRows.isEmpty()) {
		context_.ui.notifications.show_status(QStringLiteral("No diagnostics rows selected"), 2000);
		return;
	}

	QStringList lines;
	for (const QModelIndex &row : kRows) {
		lines.push_back(model_->copy_text_for_index(row));
	}

	QApplication::clipboard()->setText(lines.join(QStringLiteral("\n")));
	context_.ui.notifications.show_status(QStringLiteral("Copied selected diagnostics"), 2000);
}

void DiagnosticsPanel::copy_all_rows() {
	const QString kText = model_->copy_text_for_all();
	if (kText.isEmpty()) {
		context_.ui.notifications.show_status(QStringLiteral("No diagnostics to copy"), 2000);
		return;
	}

	QApplication::clipboard()->setText(kText);
	context_.ui.notifications.show_status(QStringLiteral("Copied diagnostics"), 2000);
}

} // namespace quader::ui
