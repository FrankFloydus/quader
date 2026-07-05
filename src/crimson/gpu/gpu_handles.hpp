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

#include "crimson/gpu/gpu_resources.hpp"

#include <bgfx/bgfx.h>

namespace crimson::gpu {

/// Destruction traits for `bgfx::VertexBufferHandle`.
struct VertexBufferHandleTraits {
	/// Return the invalid bgfx vertex-buffer handle.
	static bgfx::VertexBufferHandle invalid() noexcept {
		return BGFX_INVALID_HANDLE;
	}

	/**
	 * Check whether a handle is valid.
	 *
	 * @param handle Handle to test.
	 * @return True when bgfx considers it valid.
	 */
	static bool is_valid(bgfx::VertexBufferHandle handle) noexcept {
		return bgfx::isValid(handle);
	}

	/**
	 * Destroy a handle.
	 *
	 * @param handle Handle to destroy.
	 */
	static void destroy(bgfx::VertexBufferHandle handle) noexcept {
		bgfx::destroy(handle);
	}
};

/// Destruction traits for `bgfx::IndexBufferHandle`.
struct IndexBufferHandleTraits {
	/// Return the invalid bgfx index-buffer handle.
	static bgfx::IndexBufferHandle invalid() noexcept {
		return BGFX_INVALID_HANDLE;
	}

	/**
	 * Check whether a handle is valid.
	 *
	 * @param handle Handle to test.
	 * @return True when bgfx considers it valid.
	 */
	static bool is_valid(bgfx::IndexBufferHandle handle) noexcept {
		return bgfx::isValid(handle);
	}

	/**
	 * Destroy a handle.
	 *
	 * @param handle Handle to destroy.
	 */
	static void destroy(bgfx::IndexBufferHandle handle) noexcept {
		bgfx::destroy(handle);
	}
};

/// Destruction traits for `bgfx::ProgramHandle`.
struct ProgramHandleTraits {
	/// Return the invalid bgfx program handle.
	static bgfx::ProgramHandle invalid() noexcept {
		return BGFX_INVALID_HANDLE;
	}

	/**
	 * Check whether a handle is valid.
	 *
	 * @param handle Handle to test.
	 * @return True when bgfx considers it valid.
	 */
	static bool is_valid(bgfx::ProgramHandle handle) noexcept {
		return bgfx::isValid(handle);
	}

	/**
	 * Destroy a handle.
	 *
	 * @param handle Handle to destroy.
	 */
	static void destroy(bgfx::ProgramHandle handle) noexcept {
		bgfx::destroy(handle);
	}
};

/// Destruction traits for `bgfx::ShaderHandle`.
struct ShaderHandleTraits {
	/// Return the invalid bgfx shader handle.
	static bgfx::ShaderHandle invalid() noexcept {
		return BGFX_INVALID_HANDLE;
	}

	/**
	 * Check whether a handle is valid.
	 *
	 * @param handle Handle to test.
	 * @return True when bgfx considers it valid.
	 */
	static bool is_valid(bgfx::ShaderHandle handle) noexcept {
		return bgfx::isValid(handle);
	}

	/**
	 * Destroy a handle.
	 *
	 * @param handle Handle to destroy.
	 */
	static void destroy(bgfx::ShaderHandle handle) noexcept {
		bgfx::destroy(handle);
	}
};

/// Destruction traits for `bgfx::TextureHandle`.
struct TextureHandleTraits {
	/// Return the invalid bgfx texture handle.
	static bgfx::TextureHandle invalid() noexcept {
		return BGFX_INVALID_HANDLE;
	}

	/**
	 * Check whether a handle is valid.
	 *
	 * @param handle Handle to test.
	 * @return True when bgfx considers it valid.
	 */
	static bool is_valid(bgfx::TextureHandle handle) noexcept {
		return bgfx::isValid(handle);
	}

	/**
	 * Destroy a handle.
	 *
	 * @param handle Handle to destroy.
	 */
	static void destroy(bgfx::TextureHandle handle) noexcept {
		bgfx::destroy(handle);
	}
};

/// Destruction traits for `bgfx::UniformHandle`.
struct UniformHandleTraits {
	/// Return the invalid bgfx uniform handle.
	static bgfx::UniformHandle invalid() noexcept {
		return BGFX_INVALID_HANDLE;
	}

	/**
	 * Check whether a handle is valid.
	 *
	 * @param handle Handle to test.
	 * @return True when bgfx considers it valid.
	 */
	static bool is_valid(bgfx::UniformHandle handle) noexcept {
		return bgfx::isValid(handle);
	}

	/**
	 * Destroy a handle.
	 *
	 * @param handle Handle to destroy.
	 */
	static void destroy(bgfx::UniformHandle handle) noexcept {
		bgfx::destroy(handle);
	}
};

/// Destruction traits for `bgfx::FrameBufferHandle`.
struct FrameBufferHandleTraits {
	/// Return the invalid bgfx framebuffer handle.
	static bgfx::FrameBufferHandle invalid() noexcept {
		return BGFX_INVALID_HANDLE;
	}

	/**
	 * Check whether a handle is valid.
	 *
	 * @param handle Handle to test.
	 * @return True when bgfx considers it valid.
	 */
	static bool is_valid(bgfx::FrameBufferHandle handle) noexcept {
		return bgfx::isValid(handle);
	}

	/**
	 * Destroy a handle.
	 *
	 * @param handle Handle to destroy.
	 */
	static void destroy(bgfx::FrameBufferHandle handle) noexcept {
		bgfx::destroy(handle);
	}
};

/// Unique RAII wrapper for a bgfx vertex buffer.
using UniqueVertexBufferHandle = UniqueGpuHandle<bgfx::VertexBufferHandle, VertexBufferHandleTraits>;
/// Unique RAII wrapper for a bgfx index buffer.
using UniqueIndexBufferHandle = UniqueGpuHandle<bgfx::IndexBufferHandle, IndexBufferHandleTraits>;
/// Unique RAII wrapper for a bgfx program.
using UniqueProgramHandle = UniqueGpuHandle<bgfx::ProgramHandle, ProgramHandleTraits>;
/// Unique RAII wrapper for a bgfx shader.
using UniqueShaderHandle = UniqueGpuHandle<bgfx::ShaderHandle, ShaderHandleTraits>;
/// Unique RAII wrapper for a bgfx texture.
using UniqueTextureHandle = UniqueGpuHandle<bgfx::TextureHandle, TextureHandleTraits>;
/// Unique RAII wrapper for a bgfx uniform.
using UniqueUniformHandle = UniqueGpuHandle<bgfx::UniformHandle, UniformHandleTraits>;
/// Unique RAII wrapper for a bgfx framebuffer.
using UniqueFrameBufferHandle = UniqueGpuHandle<bgfx::FrameBufferHandle, FrameBufferHandleTraits>;

/// GPU vertex/index buffer resource.
struct GpuMeshResource {
	/// bgfx vertex layout used by the vertex buffer.
	bgfx::VertexLayout vertex_layout{};
	/// Owned vertex buffer.
	UniqueVertexBufferHandle vertex_buffer;
	/// Owned index buffer.
	UniqueIndexBufferHandle index_buffer;

	/// Return true when both vertex and index buffers are valid.
	bool valid() const noexcept;
	/// Release ownership without destroying handles.
	void invalidate() noexcept;
	/// Destroy owned handles and reset to invalid.
	void reset() noexcept;
};

/// GPU shader program resource.
struct GpuProgramResource {
	/// Owned shader program.
	UniqueProgramHandle program;

	/// Return true when the program handle is valid.
	bool valid() const noexcept;
	/// Release ownership without destroying the handle.
	void invalidate() noexcept;
	/// Destroy the owned handle and reset to invalid.
	void reset() noexcept;
};

/// GPU shader binary resource.
struct GpuShaderResource {
	/// Owned shader handle.
	UniqueShaderHandle shader;

	/// Return true when the shader handle is valid.
	bool valid() const noexcept;
	/// Release ownership without destroying the handle.
	void invalidate() noexcept;
	/// Destroy the owned handle and reset to invalid.
	void reset() noexcept;
};

} // namespace crimson::gpu
