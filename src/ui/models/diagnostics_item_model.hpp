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

#include "ui/services/viewport_diagnostics_service.hpp"

#include <QAbstractItemModel>
#include <QMetaType>
#include <QModelIndex>
#include <QObject>
#include <QStringList>
#include <Qt>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace quader::ui {

/// Row kinds exposed by the viewport diagnostics model.
enum class DiagnosticsNodeKind : std::uint8_t {
	Section,      ///< Top-level grouping row.
	SummaryField, ///< Summary key/value row.
	RenderPass,   ///< Render pass statistics row.
	Counter,      ///< Renderer counter row.
	Diagnostic,   ///< Renderer diagnostic row.
};

/// Columns exposed by the viewport diagnostics model.
enum class DiagnosticsItemColumn : int {
	Name = 0,  ///< Name or category column.
	Value = 1, ///< Value or severity column.
	Detail = 2,///< Detail column.
};

namespace diagnostics_item_roles {
/// Role containing `DiagnosticsNodeKind`.
constexpr int kNodeKindRole = Qt::UserRole + 40;
/// Role containing severity text.
constexpr int kSeverityRole = Qt::UserRole + 41;
/// Role containing category text.
constexpr int kCategoryRole = Qt::UserRole + 42;
/// Role containing frame index.
constexpr int kFrameIndexRole = Qt::UserRole + 43;
/// Role containing copy-ready row text.
constexpr int kCopyTextRole = Qt::UserRole + 44;
} //namespace diagnostics_item_roles

/// Qt tree model presenting the latest viewport diagnostics snapshot.
class DiagnosticsItemModel final : public QAbstractItemModel {
	Q_OBJECT

public:
	/// Construct a diagnostics model backed by a diagnostics service.
	explicit DiagnosticsItemModel(ViewportDiagnosticsService &diagnostics, QObject *parent = nullptr);

	/// Return a model index for a diagnostics row.
	[[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
	/// Return the parent index for `child`.
	[[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
	/// Return row count for `parent`.
	[[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
	/// Return column count for `parent`.
	[[nodiscard]] int columnCount(const QModelIndex &parent = {}) const override;
	/// Return diagnostics data for `index` and `role`.
	[[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
	/// Return item flags for diagnostics rows.
	[[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
	/// Return horizontal header text.
	[[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	/// Return copy-ready text for one index.
	[[nodiscard]] QString copy_text_for_index(const QModelIndex &index) const;
	/// Return copy-ready text for all rows.
	[[nodiscard]] QString copy_text_for_all() const;

public Q_SLOTS:
	/// Rebuild rows from the attached diagnostics service.
	void reload_from_service();

private:
	struct Node {
		DiagnosticsNodeKind kind = DiagnosticsNodeKind::Section;
		QString name;
		QString value;
		QString detail;
		QString severity;
		QString category;
		std::optional<quint64> frame_index;
		QString copy_text;
		Node *parent = nullptr;
		int row = -1;
		std::vector<std::unique_ptr<Node>> children;
	};

	[[nodiscard]] Node *node_from_index(const QModelIndex &index) const noexcept;
	void rebuild_from_snapshot(const std::optional<ViewportDiagnosticsSnapshot> &snapshot);
	void add_summary_rows(Node &section, const std::optional<ViewportDiagnosticsSnapshot> &snapshot);
	[[nodiscard]] static std::unique_ptr<Node> make_node(
			DiagnosticsNodeKind kind,
			QString name,
			QString value = {},
			QString detail = {});
	void append_root(std::unique_ptr<Node> node);
	void append_node(Node &parent, std::unique_ptr<Node> child);
	[[nodiscard]] QString copy_text_for_node(const Node &node) const;
	void append_copy_text_recursive(const Node &node, QStringList &lines) const;

	ViewportDiagnosticsService &diagnostics_;
	std::vector<std::unique_ptr<Node>> roots_;
};

} // namespace quader::ui

Q_DECLARE_METATYPE(quader::ui::DiagnosticsNodeKind)
