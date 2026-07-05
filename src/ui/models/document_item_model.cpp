#include "ui/models/document_item_model.hpp"

#include "commands/command_result.hpp"
#include "commands/document_commands.hpp"
#include "document/document.hpp"
#include "document/scene_object.hpp"
#include "ui/services/document_ui_controller.hpp"

#include <QVariant>

#include <algorithm>
#include <memory>
#include <string>

namespace quader::ui {
namespace {

constexpr int kDocumentItemColumnCount = 2;

[[nodiscard]] bool is_name_column(int column) noexcept {
  return column == static_cast<int>(DocumentItemColumn::Name);
}

[[nodiscard]] bool is_kind_column(int column) noexcept {
  return column == static_cast<int>(DocumentItemColumn::Kind);
}

} // namespace

DocumentItemModel::DocumentItemModel(DocumentUiController &documents,
                                     QObject *parent)
    : QAbstractItemModel(parent), documents_(documents) {
  register_ui_model_metatypes();
  rebuild_from_document();

  connect(&documents_, &DocumentUiController::object_list_changed, this,
          &DocumentItemModel::rebuild_from_document);
  connect(&documents_, &DocumentUiController::object_changed, this,
          &DocumentItemModel::refresh_object);
  connect(&documents_, &DocumentUiController::selection_changed, this,
          &DocumentItemModel::refresh_selection);
}

QModelIndex DocumentItemModel::index(int row, int column,
                                     const QModelIndex &parent_index) const {
  if (parent_index.isValid() || row < 0 || column < 0 ||
      column >= columnCount()) {
    return {};
  }

  const auto row_index = static_cast<std::size_t>(row);
  if (row_index >= roots_.size()) {
    return {};
  }

  return createIndex(row, column, roots_[row_index].get());
}

QModelIndex DocumentItemModel::parent(const QModelIndex &child) const {
  (void)child;
  return {};
}

int DocumentItemModel::rowCount(const QModelIndex &parent_index) const {
  if (parent_index.isValid()) {
    return 0;
  }

  return static_cast<int>(roots_.size());
}

int DocumentItemModel::columnCount(const QModelIndex &parent_index) const {
  (void)parent_index;
  return kDocumentItemColumnCount;
}

QVariant DocumentItemModel::data(const QModelIndex &model_index,
                                 int role) const {
  const auto *node = node_from_index(model_index);
  if (node == nullptr) {
    return {};
  }

  const auto *object = object_for_node(*node);
  if (object == nullptr) {
    return {};
  }

  switch (role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    if (is_name_column(model_index.column())) {
      return QString::fromStdString(object->name);
    }
    if (is_kind_column(model_index.column())) {
      return QStringLiteral("Mesh");
    }
    return {};
  case DocumentItemRoles::UiNodeIdRole:
    return QVariant::fromValue(node->ui_node_id);
  case DocumentItemRoles::ObjectIdRole:
    return QVariant::fromValue(node->object);
  case DocumentItemRoles::KindRole:
    return QVariant::fromValue(DocumentItemKind::MeshObject);
  case DocumentItemRoles::SelectedRole:
    return documents_.document().selection().references_object(node->object);
  default:
    return {};
  }
}

bool DocumentItemModel::setData(const QModelIndex &model_index,
                                const QVariant &value, int role) {
  if (role != Qt::EditRole || !model_index.isValid() ||
      !is_name_column(model_index.column())) {
    return false;
  }

  const auto *node = node_from_index(model_index);
  if (node == nullptr) {
    return false;
  }

  const auto *object = object_for_node(*node);
  if (object == nullptr) {
    return false;
  }

  const QString new_name = value.toString();
  if (new_name.isEmpty() || new_name.toStdString() == object->name) {
    return false;
  }

  auto result = documents_.execute_command(
      std::make_unique<quader::commands::RenameObjectCommand>(
          node->object, new_name.toStdString()));
  return result.is_applied();
}

Qt::ItemFlags DocumentItemModel::flags(const QModelIndex &model_index) const {
  if (!model_index.isValid()) {
    return Qt::NoItemFlags;
  }

  auto item_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if (is_name_column(model_index.column())) {
    item_flags |= Qt::ItemIsEditable;
  }

  return item_flags;
}

QVariant DocumentItemModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
    return {};
  }

  if (section == static_cast<int>(DocumentItemColumn::Name)) {
    return QStringLiteral("Name");
  }

  if (section == static_cast<int>(DocumentItemColumn::Kind)) {
    return QStringLiteral("Kind");
  }

  return {};
}

QModelIndex
DocumentItemModel::index_for_object(quader::document::ObjectId object,
                                    int column) const {
  const int row = row_for_object(object);
  if (row < 0) {
    return {};
  }

  return index(row, column);
}

void DocumentItemModel::rebuild_from_document() {
  beginResetModel();
  roots_.clear();

  const auto object_ids = documents_.document().object_ids();
  roots_.reserve(object_ids.size());
  for (int row = 0; row < static_cast<int>(object_ids.size()); ++row) {
    auto node = std::make_unique<Node>();
    node->object = object_ids[static_cast<std::size_t>(row)];
    node->ui_node_id = ui_node_id_for_object(node->object);
    node->row = row;
    roots_.push_back(std::move(node));
  }

  remove_stale_ui_node_ids();
  endResetModel();
}

void DocumentItemModel::refresh_object(quader::document::ObjectId object) {
  const int row = row_for_object(object);
  if (row < 0) {
    return;
  }

  Q_EMIT dataChanged(
      index(row, 0), index(row, kDocumentItemColumnCount - 1),
      {Qt::DisplayRole, Qt::EditRole, DocumentItemRoles::ObjectIdRole,
       DocumentItemRoles::KindRole, DocumentItemRoles::SelectedRole});
}

void DocumentItemModel::refresh_selection() {
  if (roots_.empty()) {
    return;
  }

  Q_EMIT dataChanged(
      index(0, 0),
      index(static_cast<int>(roots_.size()) - 1, kDocumentItemColumnCount - 1),
      {DocumentItemRoles::SelectedRole});
}

DocumentItemModel::Node *DocumentItemModel::node_from_index(
    const QModelIndex &model_index) const noexcept {
  if (!model_index.isValid() || model_index.model() != this) {
    return nullptr;
  }

  return static_cast<Node *>(model_index.internalPointer());
}

const quader::document::MeshObject *
DocumentItemModel::object_for_node(const Node &node) const noexcept {
  return documents_.document().find_mesh_object(node.object);
}

int DocumentItemModel::row_for_object(
    quader::document::ObjectId object) const noexcept {
  const auto it =
      std::find_if(roots_.begin(), roots_.end(), [object](const auto &node) {
        return node->object == object;
      });

  if (it == roots_.end()) {
    return -1;
  }

  return (*it)->row;
}

UiNodeId
DocumentItemModel::ui_node_id_for_object(quader::document::ObjectId object) {
  const auto it = ui_node_ids_.find(object);
  if (it != ui_node_ids_.end()) {
    return it->second;
  }

  UiNodeId id{next_ui_node_id_++};
  ui_node_ids_.emplace(object, id);
  return id;
}

void DocumentItemModel::remove_stale_ui_node_ids() {
  for (auto it = ui_node_ids_.begin(); it != ui_node_ids_.end();) {
    if (row_for_object(it->first) < 0) {
      it = ui_node_ids_.erase(it);
    } else {
      ++it;
    }
  }
}

} // namespace quader::ui
