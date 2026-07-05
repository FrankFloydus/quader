#include "crimson/gpu/gpu_mesh_cache.hpp"

#include <array>

#include <bgfx/bgfx.h>

namespace crimson::gpu {
namespace {

struct PosNormalVertex {
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
};

const PosNormalVertex kCubeVertices[] = {
	{ -1.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F },
	{ 1.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F },
	{ -1.0F, -1.0F, 1.0F, 0.0F, 0.0F, 1.0F },
	{ 1.0F, -1.0F, 1.0F, 0.0F, 0.0F, 1.0F },
	{ -1.0F, 1.0F, -1.0F, 0.0F, 0.0F, -1.0F },
	{ 1.0F, 1.0F, -1.0F, 0.0F, 0.0F, -1.0F },
	{ -1.0F, -1.0F, -1.0F, 0.0F, 0.0F, -1.0F },
	{ 1.0F, -1.0F, -1.0F, 0.0F, 0.0F, -1.0F },
	{ -1.0F, 1.0F, -1.0F, 0.0F, 1.0F, 0.0F },
	{ 1.0F, 1.0F, -1.0F, 0.0F, 1.0F, 0.0F },
	{ -1.0F, 1.0F, 1.0F, 0.0F, 1.0F, 0.0F },
	{ 1.0F, 1.0F, 1.0F, 0.0F, 1.0F, 0.0F },
	{ -1.0F, -1.0F, -1.0F, 0.0F, -1.0F, 0.0F },
	{ 1.0F, -1.0F, -1.0F, 0.0F, -1.0F, 0.0F },
	{ -1.0F, -1.0F, 1.0F, 0.0F, -1.0F, 0.0F },
	{ 1.0F, -1.0F, 1.0F, 0.0F, -1.0F, 0.0F },
	{ 1.0F, -1.0F, -1.0F, 1.0F, 0.0F, 0.0F },
	{ 1.0F, 1.0F, -1.0F, 1.0F, 0.0F, 0.0F },
	{ 1.0F, -1.0F, 1.0F, 1.0F, 0.0F, 0.0F },
	{ 1.0F, 1.0F, 1.0F, 1.0F, 0.0F, 0.0F },
	{ -1.0F, -1.0F, -1.0F, -1.0F, 0.0F, 0.0F },
	{ -1.0F, 1.0F, -1.0F, -1.0F, 0.0F, 0.0F },
	{ -1.0F, -1.0F, 1.0F, -1.0F, 0.0F, 0.0F },
	{ -1.0F, 1.0F, 1.0F, -1.0F, 0.0F, 0.0F },
};

const std::uint16_t kCubeIndices[] = {
	0,
	2,
	1,
	1,
	2,
	3,
	5,
	7,
	4,
	4,
	7,
	6,
	8,
	10,
	9,
	9,
	10,
	11,
	13,
	15,
	12,
	12,
	15,
	14,
	16,
	18,
	17,
	17,
	18,
	19,
	21,
	23,
	20,
	20,
	23,
	22,
};

void push_mesh_creation_error(RendererStatus &status) {
	status.diagnostics.push_back(RendererDiagnostic{
			.severity = RendererDiagnosticSeverity::Error,
			.code = RendererDiagnosticCode::ResourceCreationFailed,
			.message = "Crimson failed to create the prototype cube render mesh.",
			.subsystem = "gpu.mesh",
			.resource_name = "PrototypeCube",
	});
}

} // namespace

RenderMeshHandle GpuMeshCache::create_prototype_cube(RendererStatus &status) {
	GpuMeshResource resource;
	resource.vertex_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.end();
	resource.vertex_buffer.reset(bgfx::createVertexBuffer(
			bgfx::makeRef(kCubeVertices, sizeof(kCubeVertices)),
			resource.vertex_layout));
	resource.index_buffer.reset(bgfx::createIndexBuffer(bgfx::makeRef(kCubeIndices, sizeof(kCubeIndices))));

	if (!resource.valid()) {
		push_mesh_creation_error(status);
		return {};
	}

	const RenderMeshHandle kHandle = meshes_.create(std::move(resource));
	++upload_stats_.mesh_create_count;
	upload_stats_.uploaded_vertex_bytes += sizeof(kCubeVertices);
	upload_stats_.uploaded_index_bytes += sizeof(kCubeIndices);
	return kHandle;
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
