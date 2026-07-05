#pragma once

#include "document/object_id.hpp"

#include <cstdint>

namespace quader::document {

enum class DocumentChangeKind {
	ObjectCreated,
	ObjectDeleted,
	ObjectRenamed,
	TransformChanged,
	MeshTopologyChanged,
	MeshGeometryChanged,
	SelectionChanged,
	DirtyStateChanged,
};

enum class DocumentDirtyFlag : std::uint32_t {
	None = 0U,
	Objects = 1U << 0U,
	ObjectProperties = 1U << 1U,
	Transforms = 1U << 2U,
	MeshTopology = 1U << 3U,
	MeshGeometry = 1U << 4U,
	Selection = 1U << 5U,
};

using DocumentDirtyFlags = std::uint32_t;

constexpr DocumentDirtyFlags kDocumentDirtyNone = 0U;

[[nodiscard]] constexpr DocumentDirtyFlags to_dirty_flags(DocumentDirtyFlag flag) noexcept {
	return static_cast<DocumentDirtyFlags>(flag);
}

[[nodiscard]] constexpr DocumentDirtyFlags operator|(DocumentDirtyFlag left,
		DocumentDirtyFlag right) noexcept {
	return to_dirty_flags(left) | to_dirty_flags(right);
}

[[nodiscard]] constexpr DocumentDirtyFlags operator|(DocumentDirtyFlags left,
		DocumentDirtyFlag right) noexcept {
	return left | to_dirty_flags(right);
}

[[nodiscard]] constexpr bool has_dirty_flag(DocumentDirtyFlags flags,
		DocumentDirtyFlag flag) noexcept {
	return (flags & to_dirty_flags(flag)) != 0U;
}

struct DocumentChange {
	DocumentChangeKind kind = DocumentChangeKind::DirtyStateChanged;
	ObjectId object = ObjectId::invalid();
	DocumentDirtyFlags dirty_flags = kDocumentDirtyNone;
};

} // namespace quader::document
