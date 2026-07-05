#pragma once

#include "crimson/gpu/gpu_handles.hpp"
#include "crimson/gpu/gpu_resources.hpp"

namespace crimson::gpu {

void destroy_gpu_mesh_resource(GpuMeshResource &resource) noexcept;
void destroy_gpu_program_resource(GpuProgramResource &resource) noexcept;
void destroy_gpu_shader_resource(GpuShaderResource &resource) noexcept;

using GpuMeshTable = GpuResourceTable<GpuMeshResource, RenderMeshHandle>;
using GpuProgramTable = GpuResourceTable<GpuProgramResource, RenderProgramHandle>;

} // namespace crimson::gpu
