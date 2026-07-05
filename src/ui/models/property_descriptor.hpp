#pragma once

#include "document/object_id.hpp"

#include <QMetaType>
#include <QString>
#include <QVariant>
#include <Qt>

#include <cstdint>
#include <vector>

namespace quader::document {
class Document;
}

namespace quader::ui {

enum class PropertyValueKind : std::uint8_t {
  String,
  Float,
  Int,
  Vector3,
  ReadOnlyText,
};

enum class PropertyField : std::uint8_t {
  ObjectName,
  ObjectKind,
  VertexCount,
  EdgeCount,
  FaceCount,
  TranslationX,
  TranslationY,
  TranslationZ,
  RotationX,
  RotationY,
  RotationZ,
  ScaleX,
  ScaleY,
  ScaleZ,
  SelectionSummary,
};

struct PropertyPath {
  quader::document::ObjectId object;
  PropertyField field = PropertyField::SelectionSummary;

  friend bool operator==(const PropertyPath &, const PropertyPath &) = default;
};

struct PropertyDescriptor {
  PropertyPath path;
  QString label;
  PropertyValueKind kind = PropertyValueKind::ReadOnlyText;
  QVariant value;
  QVariant minimum;
  QVariant maximum;
  int decimals = 3;
  bool editable = false;
  QString tooltip;
};

namespace PropertyItemRoles {
constexpr int PropertyPathRole = Qt::UserRole + 1;
constexpr int ValueKindRole = Qt::UserRole + 2;
constexpr int EditableRole = Qt::UserRole + 3;
constexpr int DescriptorRole = Qt::UserRole + 4;
} // namespace PropertyItemRoles

std::vector<PropertyDescriptor>
build_property_descriptors(const quader::document::Document &document);

} // namespace quader::ui

Q_DECLARE_METATYPE(quader::ui::PropertyPath)
Q_DECLARE_METATYPE(quader::ui::PropertyValueKind)
Q_DECLARE_METATYPE(quader::ui::PropertyDescriptor)
