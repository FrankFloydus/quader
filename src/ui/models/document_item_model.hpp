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

#include "document/object_id.hpp"
#include "ui/models/item_model_roles.hpp"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QObject>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace quader::document {
struct MeshObject;
}

namespace quader::ui {

class DocumentUiController;

/// Qt item model presenting document objects without owning document state.
class DocumentItemModel final : public QAbstractItemModel {
	Q_OBJECT

public:
	/// Construct a model backed by a non-owning document UI controller reference.
	explicit DocumentItemModel(DocumentUiController &documents,
			QObject *parent = nullptr);

	/// Return a model index for a row, column, and parent.
	[[nodiscard]] QModelIndex
	index(int row, int column, const QModelIndex &parent = {}) const override;
	/// Return the parent index for `child`.
	[[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
	/// Return row count for `parent`.
	[[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
	/// Return column count for `parent`.
	[[nodiscard]] int columnCount(const QModelIndex &parent = {}) const override;
	/// Return model data for `index` and `role`.
	[[nodiscard]] QVariant data(const QModelIndex &index,
			int role) const override;
	/// Dispatch editable changes through document commands.
	[[nodiscard]] bool setData(const QModelIndex &index, const QVariant &value,
			int role) override;
	/// Return item flags for `index`.
	[[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
	/// Return horizontal header text.
	[[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
			int role) const override;

	/**
	 * Return the model index for a document object.
	 *
	 * @param object Object id to locate.
	 * @param column Column to address.
	 * @return Matching index, or an invalid index when not visible.
	 */
	[[nodiscard]] QModelIndex index_for_object(
			quader::document::ObjectId object,
			int column = static_cast<int>(DocumentItemColumn::Name)) const;

public Q_SLOTS:
	/// Rebuild presentation rows from the current document.
	void rebuild_from_document();
	/// Refresh one object row if it is visible.
	void refresh_object(quader::document::ObjectId object);
	/// Refresh selection roles for visible rows.
	void refresh_selection();

private:
	struct Node {
		UiNodeId ui_node_id;
		quader::document::ObjectId object;
		int row = -1;
	};

	[[nodiscard]] Node *node_from_index(const QModelIndex &index) const noexcept;
	[[nodiscard]] const quader::document::MeshObject *
	object_for_node(const Node &node) const noexcept;
	[[nodiscard]] int
	row_for_object(quader::document::ObjectId object) const noexcept;
	[[nodiscard]] UiNodeId
	ui_node_id_for_object(quader::document::ObjectId object);
	void remove_stale_ui_node_ids();

	DocumentUiController &documents_;
	std::vector<std::unique_ptr<Node>> roots_;
	std::unordered_map<quader::document::ObjectId, UiNodeId> ui_node_ids_;
	std::uint64_t next_ui_node_id_ = 1U;
};

} // namespace quader::ui
