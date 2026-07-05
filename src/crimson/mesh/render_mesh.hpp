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

#include "crimson/material/base_shader.hpp"
#include "crimson/renderer_types.hpp"
#include "math/aabb.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace crimson {

/// Revision tuple used to decide whether a render mesh upload is stale.
struct RenderMeshRevision {
	/// Geometry buffer revision.
	std::uint64_t geometry_version = 0;
	/// Topology/index buffer revision.
	std::uint64_t topology_version = 0;
	/// Vertex attribute revision.
	std::uint64_t attribute_version = 0;

	/// Compare revisions by all version fields.
	friend bool operator==(const RenderMeshRevision &, const RenderMeshRevision &) = default;
};

/// CPU-side render mesh upload descriptor.
struct RenderMeshUploadDesc {
	/// Target render mesh handle.
	RenderMeshHandle handle;
	/// Source revision being uploaded.
	RenderMeshRevision revision;
	/// Interleaved position/normal/UV0 vertex data borrowed for the upload call.
	std::span<const float> position_normal_uv_interleaved;
	/// Index data borrowed for the upload call.
	std::span<const std::uint32_t> indices;
	/// Vertex attributes present in the upload.
	VertexAttributeMask attributes = VertexAttributePosition | VertexAttributeNormal | VertexAttributeUv0;
	/// Bounds represented by the uploaded mesh.
	quader::math::Aabb bounds;
};

/// Snapshot-owned render mesh upload payload.
struct RenderMeshUpload {
	/// Target render mesh handle.
	RenderMeshHandle handle;
	/// Source revision being uploaded.
	RenderMeshRevision revision;
	/// Interleaved position/normal/UV0 vertex data.
	std::vector<float> position_normal_uv_interleaved;
	/// Index data for triangle submission.
	std::vector<std::uint32_t> indices;
	/// Vertex attributes present in the upload.
	VertexAttributeMask attributes = VertexAttributePosition | VertexAttributeNormal | VertexAttributeUv0;
	/// Bounds represented by the uploaded mesh.
	quader::math::Aabb bounds;

	/// Borrow this owned payload as an upload descriptor.
	[[nodiscard]] RenderMeshUploadDesc desc() const noexcept {
		return RenderMeshUploadDesc{
			.handle = handle,
			.revision = revision,
			.position_normal_uv_interleaved = position_normal_uv_interleaved,
			.indices = indices,
			.attributes = attributes,
			.bounds = bounds,
		};
	}
};

/// Record of the revision and byte counts uploaded for one render mesh.
struct RenderMeshUploadRecord {
	/// Uploaded render mesh handle.
	RenderMeshHandle handle;
	/// Revision stored by the GPU/cache side.
	RenderMeshRevision uploaded_revision;
	/// Number of vertex bytes uploaded.
	std::uint64_t vertex_bytes = 0;
	/// Number of index bytes uploaded.
	std::uint64_t index_bytes = 0;
};

} // namespace crimson
