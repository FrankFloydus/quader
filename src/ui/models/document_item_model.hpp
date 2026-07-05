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

class DocumentItemModel final : public QAbstractItemModel {
	Q_OBJECT

public:
	explicit DocumentItemModel(DocumentUiController &documents,
			QObject *parent = nullptr);

	[[nodiscard]] QModelIndex
	index(int row, int column, const QModelIndex &parent = {}) const override;
	[[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
	[[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
	[[nodiscard]] int columnCount(const QModelIndex &parent = {}) const override;
	[[nodiscard]] QVariant data(const QModelIndex &index,
			int role) const override;
	[[nodiscard]] bool setData(const QModelIndex &index, const QVariant &value,
			int role) override;
	[[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
	[[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
			int role) const override;

	[[nodiscard]] QModelIndex index_for_object(
			quader::document::ObjectId object,
			int column = static_cast<int>(DocumentItemColumn::Name)) const;

public Q_SLOTS:
	void rebuild_from_document();
	void refresh_object(quader::document::ObjectId object);
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
