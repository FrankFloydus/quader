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

#include <cstdint>

namespace quader::document {

/// Kinds of document changes emitted into the pending change queue.
enum class DocumentChangeKind {
	ObjectCreated,       ///< A scene object was added or restored.
	ObjectDeleted,       ///< A scene object was removed.
	ObjectRenamed,       ///< An object's display name changed.
	TransformChanged,    ///< An object's transform changed.
	MeshTopologyChanged, ///< Mesh connectivity or element identity changed.
	MeshGeometryChanged, ///< Mesh positions or geometric attributes changed.
	SelectionChanged,    ///< Document selection changed.
	DirtyStateChanged,   ///< Dirty flags changed.
};

/// Bit values that identify which document subsystems are dirty.
enum class DocumentDirtyFlag : std::uint32_t {
	None = 0U,                 ///< No dirty subsystem.
	Objects = 1U << 0U,        ///< Object list changed.
	ObjectProperties = 1U << 1U,///< Object metadata or material changed.
	Transforms = 1U << 2U,     ///< Object transform changed.
	MeshTopology = 1U << 3U,   ///< Mesh connectivity changed.
	MeshGeometry = 1U << 4U,   ///< Mesh geometry changed.
	Selection = 1U << 5U,      ///< Selection changed.
};

/// Packed bitmask of `DocumentDirtyFlag` values.
using DocumentDirtyFlags = std::uint32_t;

/// Empty dirty flag mask.
constexpr DocumentDirtyFlags kDocumentDirtyNone = 0U;

/**
 * Convert one dirty flag into its bitmask representation.
 *
 * @param flag Flag to convert.
 * @return Bitmask containing `flag`.
 */
[[nodiscard]] constexpr DocumentDirtyFlags to_dirty_flags(DocumentDirtyFlag flag) noexcept {
	return static_cast<DocumentDirtyFlags>(flag);
}

/**
 * Combine two dirty flags into a bitmask.
 *
 * @param left, right Flags to combine.
 * @return Bitmask containing both flags.
 */
[[nodiscard]] constexpr DocumentDirtyFlags operator|(DocumentDirtyFlag left,
		DocumentDirtyFlag right) noexcept {
	return to_dirty_flags(left) | to_dirty_flags(right);
}

/**
 * Add a dirty flag to an existing bitmask.
 *
 * @param left Existing bitmask.
 * @param right Flag to add.
 * @return Combined dirty flag bitmask.
 */
[[nodiscard]] constexpr DocumentDirtyFlags operator|(DocumentDirtyFlags left,
		DocumentDirtyFlag right) noexcept {
	return left | to_dirty_flags(right);
}

/**
 * Check whether a dirty flag is present in a bitmask.
 *
 * @param flags Dirty flag bitmask.
 * @param flag Flag to test.
 * @return True when `flag` is present.
 */
[[nodiscard]] constexpr bool has_dirty_flag(DocumentDirtyFlags flags,
		DocumentDirtyFlag flag) noexcept {
	return (flags & to_dirty_flags(flag)) != 0U;
}

/// One queued document change notification.
struct DocumentChange {
	/// Change category.
	DocumentChangeKind kind = DocumentChangeKind::DirtyStateChanged;
	/// Object affected by the change, or invalid when not object-specific.
	ObjectId object = ObjectId::invalid();
	/// Dirty bits associated with dirty-state changes.
	DocumentDirtyFlags dirty_flags = kDocumentDirtyNone;
};

} // namespace quader::document
