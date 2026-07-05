#pragma once

#include "crimson/material/base_shader.hpp"
#include "crimson/renderer_types.hpp"
#include "math/aabb.hpp"

#include <cstdint>
#include <span>

namespace crimson {

struct RenderMeshRevision {
	std::uint64_t geometry_version = 0;
	std::uint64_t topology_version = 0;
	std::uint64_t attribute_version = 0;

	friend bool operator==(const RenderMeshRevision &, const RenderMeshRevision &) = default;
};

struct RenderMeshUploadDesc {
	RenderMeshHandle handle;
	RenderMeshRevision revision;
	std::span<const float> position_normal_interleaved;
	std::span<const std::uint32_t> indices;
	VertexAttributeMask attributes = VertexAttributePosition | VertexAttributeNormal;
	quader::math::Aabb bounds;
};

struct RenderMeshUploadRecord {
	RenderMeshHandle handle;
	RenderMeshRevision uploaded_revision;
	std::uint64_t vertex_bytes = 0;
	std::uint64_t index_bytes = 0;
};

} // namespace crimson
