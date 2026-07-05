#include "crimson/gpu/gpu_runtime_resources.hpp"

namespace crimson::gpu {

bool GpuMeshResource::valid() const noexcept
{
    return vertex_buffer.valid() && index_buffer.valid();
}

void GpuMeshResource::invalidate() noexcept
{
    (void)vertex_buffer.release();
    (void)index_buffer.release();
}

void GpuMeshResource::reset() noexcept
{
    vertex_buffer.reset();
    index_buffer.reset();
}

bool GpuProgramResource::valid() const noexcept
{
    return program.valid();
}

void GpuProgramResource::invalidate() noexcept
{
    (void)program.release();
}

void GpuProgramResource::reset() noexcept
{
    program.reset();
}

bool GpuShaderResource::valid() const noexcept
{
    return shader.valid();
}

void GpuShaderResource::invalidate() noexcept
{
    (void)shader.release();
}

void GpuShaderResource::reset() noexcept
{
    shader.reset();
}

void destroy_gpu_mesh_resource(GpuMeshResource& resource) noexcept
{
    resource.reset();
}

void destroy_gpu_program_resource(GpuProgramResource& resource) noexcept
{
    resource.reset();
}

void destroy_gpu_shader_resource(GpuShaderResource& resource) noexcept
{
    resource.reset();
}

} // namespace crimson::gpu
