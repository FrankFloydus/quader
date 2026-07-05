#pragma once

#include "document/object_id.hpp"

#include <QMetaType>
#include <Qt>

#include <cstdint>

namespace quader::ui {

struct UiNodeId {
  std::uint64_t value = 0U;

  [[nodiscard]] bool is_valid() const noexcept { return value != 0U; }

  friend bool operator==(UiNodeId, UiNodeId) = default;
};

enum class DocumentItemKind : std::uint8_t {
  MeshObject,
};

enum class DocumentItemColumn : int {
  Name = 0,
  Kind = 1,
};

namespace DocumentItemRoles {
constexpr int UiNodeIdRole = Qt::UserRole + 1;
constexpr int ObjectIdRole = Qt::UserRole + 2;
constexpr int KindRole = Qt::UserRole + 3;
constexpr int SelectedRole = Qt::UserRole + 4;
} // namespace DocumentItemRoles

void register_ui_model_metatypes();

} // namespace quader::ui

Q_DECLARE_METATYPE(quader::document::ObjectId)
Q_DECLARE_METATYPE(quader::ui::UiNodeId)
Q_DECLARE_METATYPE(quader::ui::DocumentItemKind)
