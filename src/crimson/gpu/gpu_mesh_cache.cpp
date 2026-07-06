/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/gpu_mesh_cache.hpp"

#include "crimson/mesh/render_mesh_upload.hpp"

#include <cstdint>
#include <string>
#include <utility>

#include <bgfx/bgfx.h>

namespace crimson::gpu {
namespace {

void push_mesh_upload_error(RendererStatus &status, RendererDiagnostic diagnostic) {
	status.diagnostics.push_back(std::move(diagnostic));
}

void push_mesh_upload_creation_error(RendererStatus &status, RenderMeshHandle handle) {
	status.diagnostics.push_back(RendererDiagnostic{
			.severity = RendererDiagnosticSeverity::Error,
			.code = RendererDiagnosticCode::ResourceCreationFailed,
			.message = "Crimson failed to create a document render mesh.",
			.subsystem = "gpu.mesh",
			.resource_name = "RenderMesh[" + std::to_string(handle.index) + "]",
	});
}

bool is_clean_mesh_upload(const GpuMeshResource *resource, RenderMeshRevision revision) noexcept {
	return resource != nullptr && resource->uploaded_revision == revision;
}

} // namespace

bool GpuMeshCache::upload_mesh(const RenderMeshUploadDesc &desc, RendererStatus &status) {
	auto validated = validate_render_mesh_upload_desc(desc);
	if (!validated) {
		push_mesh_upload_error(status, std::move(validated).error());
		return false;
	}

	const GpuMeshResource *existing = get(desc.handle);
	if (is_clean_mesh_upload(existing, desc.revision)) {
		++upload_stats_.skipped_clean_resource_count;
		return true;
	}

	GpuMeshResource resource;
	resource.uploaded_revision = desc.revision;
	resource.vertex_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	resource.vertex_buffer.reset(bgfx::createVertexBuffer(
			bgfx::copy(
					desc.position_normal_uv_interleaved.data(),
					static_cast<std::uint32_t>(desc.position_normal_uv_interleaved.size_bytes())),
			resource.vertex_layout));
	resource.index_buffer.reset(bgfx::createIndexBuffer(
			bgfx::copy(desc.indices.data(), static_cast<std::uint32_t>(desc.indices.size_bytes())),
			BGFX_BUFFER_INDEX32));

	if (!resource.valid()) {
		push_mesh_upload_creation_error(status, desc.handle);
		return false;
	}

	const bool kUpdated = existing != nullptr;
	if (!meshes_.upsert(desc.handle, std::move(resource))) {
		push_mesh_upload_creation_error(status, desc.handle);
		return false;
	}

	const RenderMeshUploadRecord kRecord = validated.value();
	if (kUpdated) {
		++upload_stats_.mesh_update_count;
	} else {
		++upload_stats_.mesh_create_count;
	}
	upload_stats_.uploaded_vertex_bytes += kRecord.vertex_bytes;
	upload_stats_.uploaded_index_bytes += kRecord.index_bytes;
	return true;
}

const GpuMeshResource *GpuMeshCache::get(RenderMeshHandle handle) const noexcept {
	return meshes_.get(handle);
}

std::size_t GpuMeshCache::live_mesh_count() const noexcept {
	return meshes_.live_count();
}

FrameUploadStats GpuMeshCache::upload_stats() const noexcept {
	return upload_stats_;
}

void GpuMeshCache::clear() noexcept {
	meshes_.clear();
	upload_stats_ = {};
}

} // namespace crimson::gpu
