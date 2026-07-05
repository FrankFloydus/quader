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
#include <Qt>

#include <cstdint>

namespace quader::ui {

/// Stable presentation-node id used inside Qt item models.
struct UiNodeId {
	/// Non-zero id value.
	std::uint64_t value = 0U;

	/// Return true when `value` is non-zero.
	[[nodiscard]] bool is_valid() const noexcept { return value != 0U; }

	friend bool operator==(UiNodeId, UiNodeId) = default;
};

/// Row kind exposed by the document item model.
enum class DocumentItemKind : std::uint8_t {
	MeshObject, ///< Mesh object row.
};

/// Columns exposed by the document item model.
enum class DocumentItemColumn : int {
	Name = 0, ///< Object name column.
	Kind = 1, ///< Object kind column.
};

namespace document_item_roles {
/// Role containing `UiNodeId`.
constexpr int kUiNodeIdRole = Qt::UserRole + 1;
/// Role containing `quader::document::ObjectId`.
constexpr int kObjectIdRole = Qt::UserRole + 2;
/// Role containing `DocumentItemKind`.
constexpr int kKindRole = Qt::UserRole + 3;
/// Role containing selection state.
constexpr int kSelectedRole = Qt::UserRole + 4;
} //namespace document_item_roles

/// Register Qt metatypes used by UI item models.
void register_ui_model_metatypes();

} // namespace quader::ui

Q_DECLARE_METATYPE(quader::document::ObjectId)
Q_DECLARE_METATYPE(quader::ui::UiNodeId)
Q_DECLARE_METATYPE(quader::ui::DocumentItemKind)
