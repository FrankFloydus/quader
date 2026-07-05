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

#include "crimson/gpu/gpu_handles.hpp"
#include "crimson/gpu/gpu_resources.hpp"

namespace crimson::gpu {

/**
 * Destroy handles owned by a mesh resource.
 *
 * @param resource Mesh resource whose owned handles are destroyed.
 */
void destroy_gpu_mesh_resource(GpuMeshResource &resource) noexcept;
/**
 * Destroy handles owned by a program resource.
 *
 * @param resource Program resource whose owned handle is destroyed.
 */
void destroy_gpu_program_resource(GpuProgramResource &resource) noexcept;
/**
 * Destroy handles owned by a shader resource.
 *
 * @param resource Shader resource whose owned handle is destroyed.
 */
void destroy_gpu_shader_resource(GpuShaderResource &resource) noexcept;

/// Resource table for GPU mesh resources keyed by public mesh handles.
using GpuMeshTable = GpuResourceTable<GpuMeshResource, RenderMeshHandle>;
/// Resource table for GPU program resources keyed by public program handles.
using GpuProgramTable = GpuResourceTable<GpuProgramResource, RenderProgramHandle>;

} // namespace crimson::gpu
