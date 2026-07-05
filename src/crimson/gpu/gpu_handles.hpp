#pragma once

#include "crimson/gpu/gpu_resources.hpp"

#include <bgfx/bgfx.h>

namespace crimson::gpu {

struct VertexBufferHandleTraits {
    static bgfx::VertexBufferHandle invalid() noexcept
    {
        return BGFX_INVALID_HANDLE;
    }

    static bool is_valid(bgfx::VertexBufferHandle handle) noexcept
    {
        return bgfx::isValid(handle);
    }

    static void destroy(bgfx::VertexBufferHandle handle) noexcept
    {
        bgfx::destroy(handle);
    }
};

struct IndexBufferHandleTraits {
    static bgfx::IndexBufferHandle invalid() noexcept
    {
        return BGFX_INVALID_HANDLE;
    }

    static bool is_valid(bgfx::IndexBufferHandle handle) noexcept
    {
        return bgfx::isValid(handle);
    }

    static void destroy(bgfx::IndexBufferHandle handle) noexcept
    {
        bgfx::destroy(handle);
    }
};

struct ProgramHandleTraits {
    static bgfx::ProgramHandle invalid() noexcept
    {
        return BGFX_INVALID_HANDLE;
    }

    static bool is_valid(bgfx::ProgramHandle handle) noexcept
    {
        return bgfx::isValid(handle);
    }

    static void destroy(bgfx::ProgramHandle handle) noexcept
    {
        bgfx::destroy(handle);
    }
};

struct ShaderHandleTraits {
    static bgfx::ShaderHandle invalid() noexcept
    {
        return BGFX_INVALID_HANDLE;
    }

    static bool is_valid(bgfx::ShaderHandle handle) noexcept
    {
        return bgfx::isValid(handle);
    }

    static void destroy(bgfx::ShaderHandle handle) noexcept
    {
        bgfx::destroy(handle);
    }
};

struct TextureHandleTraits {
    static bgfx::TextureHandle invalid() noexcept
    {
        return BGFX_INVALID_HANDLE;
    }

    static bool is_valid(bgfx::TextureHandle handle) noexcept
    {
        return bgfx::isValid(handle);
    }

    static void destroy(bgfx::TextureHandle handle) noexcept
    {
        bgfx::destroy(handle);
    }
};

struct UniformHandleTraits {
    static bgfx::UniformHandle invalid() noexcept
    {
        return BGFX_INVALID_HANDLE;
    }

    static bool is_valid(bgfx::UniformHandle handle) noexcept
    {
        return bgfx::isValid(handle);
    }

    static void destroy(bgfx::UniformHandle handle) noexcept
    {
        bgfx::destroy(handle);
    }
};

struct FrameBufferHandleTraits {
    static bgfx::FrameBufferHandle invalid() noexcept
    {
        return BGFX_INVALID_HANDLE;
    }

    static bool is_valid(bgfx::FrameBufferHandle handle) noexcept
    {
        return bgfx::isValid(handle);
    }

    static void destroy(bgfx::FrameBufferHandle handle) noexcept
    {
        bgfx::destroy(handle);
    }
};

using UniqueVertexBufferHandle = UniqueGpuHandle<bgfx::VertexBufferHandle, VertexBufferHandleTraits>;
using UniqueIndexBufferHandle = UniqueGpuHandle<bgfx::IndexBufferHandle, IndexBufferHandleTraits>;
using UniqueProgramHandle = UniqueGpuHandle<bgfx::ProgramHandle, ProgramHandleTraits>;
using UniqueShaderHandle = UniqueGpuHandle<bgfx::ShaderHandle, ShaderHandleTraits>;
using UniqueTextureHandle = UniqueGpuHandle<bgfx::TextureHandle, TextureHandleTraits>;
using UniqueUniformHandle = UniqueGpuHandle<bgfx::UniformHandle, UniformHandleTraits>;
using UniqueFrameBufferHandle = UniqueGpuHandle<bgfx::FrameBufferHandle, FrameBufferHandleTraits>;

struct GpuMeshResource {
    bgfx::VertexLayout vertex_layout{};
    UniqueVertexBufferHandle vertex_buffer;
    UniqueIndexBufferHandle index_buffer;

    bool valid() const noexcept;
    void invalidate() noexcept;
    void reset() noexcept;
};

struct GpuProgramResource {
    UniqueProgramHandle program;

    bool valid() const noexcept;
    void invalidate() noexcept;
    void reset() noexcept;
};

struct GpuShaderResource {
    UniqueShaderHandle shader;

    bool valid() const noexcept;
    void invalidate() noexcept;
    void reset() noexcept;
};

} // namespace crimson::gpu
