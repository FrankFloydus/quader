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

/// Value/editor kind exposed by a property descriptor.
enum class PropertyValueKind : std::uint8_t {
	String,      ///< Editable string value.
	Float,       ///< Editable floating-point value.
	Int,         ///< Editable integer value.
	Vector3,     ///< Three-component vector value.
	ReadOnlyText,///< Display-only text.
};

/// Stable property fields exposed by the property model.
enum class PropertyField : std::uint8_t {
	ObjectName,       ///< Object name.
	ObjectKind,       ///< Object kind label.
	VertexCount,      ///< Mesh vertex count.
	EdgeCount,        ///< Mesh edge count.
	FaceCount,        ///< Mesh face count.
	TranslationX,     ///< X translation component.
	TranslationY,     ///< Y translation component.
	TranslationZ,     ///< Z translation component.
	RotationX,        ///< X Euler rotation component.
	RotationY,        ///< Y Euler rotation component.
	RotationZ,        ///< Z Euler rotation component.
	ScaleX,           ///< X scale component.
	ScaleY,           ///< Y scale component.
	ScaleZ,           ///< Z scale component.
	SelectionSummary, ///< Read-only selection summary.
};

/// Address of one editable/displayed property.
struct PropertyPath {
	/// Object owning the property, or invalid for document-level rows.
	quader::document::ObjectId object;
	/// Field addressed by this path.
	PropertyField field = PropertyField::SelectionSummary;

	friend bool operator==(const PropertyPath &, const PropertyPath &) = default;
};

/// Presentation descriptor for one row in the property model.
struct PropertyDescriptor {
	/// Stable property address.
	PropertyPath path;
	/// User-visible label.
	QString label;
	/// Value/editor kind.
	PropertyValueKind kind = PropertyValueKind::ReadOnlyText;
	/// Current value.
	QVariant value;
	/// Optional minimum value for editors.
	QVariant minimum;
	/// Optional maximum value for editors.
	QVariant maximum;
	/// Decimal precision for numeric display/editing.
	int decimals = 3;
	/// True when the model may commit edits for this row.
	bool editable = false;
	/// Optional tooltip text.
	QString tooltip;
};

namespace property_item_roles {
/// Role containing `PropertyPath`.
constexpr int kPropertyPathRole = Qt::UserRole + 1;
/// Role containing `PropertyValueKind`.
constexpr int kValueKindRole = Qt::UserRole + 2;
/// Role containing editability.
constexpr int kEditableRole = Qt::UserRole + 3;
/// Role containing `PropertyDescriptor`.
constexpr int kDescriptorRole = Qt::UserRole + 4;
} //namespace property_item_roles

/// Build property descriptors for the current document selection.
std::vector<PropertyDescriptor>
build_property_descriptors(const quader::document::Document &document);

} // namespace quader::ui

Q_DECLARE_METATYPE(quader::ui::PropertyPath)
Q_DECLARE_METATYPE(quader::ui::PropertyValueKind)
Q_DECLARE_METATYPE(quader::ui::PropertyDescriptor)
