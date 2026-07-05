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

#include "crimson/renderer_types.hpp"

#include <cstdint>
#include <string>

namespace crimson {

/// Frame resource format used by the render graph.
enum class RenderResourceFormat {
	/// External backbuffer color target.
	BackbufferColor,
	/// External backbuffer depth target.
	BackbufferDepth,
	/// 8-bit normalized RGBA target.
	Rgba8Unorm,
	/// 16-bit floating-point RGBA target.
	Rgba16Float,
	/// 24-bit depth plus 8-bit stencil target.
	D24S8,
	/// 32-bit floating-point depth target.
	D32Float,
	/// 32-bit unsigned integer target.
	R32Uint,
};

/// Access mode declared by a pass for one resource.
enum class RenderResourceAccess {
	/// Pass reads the resource.
	Read,
	/// Pass writes the resource.
	Write,
	/// Pass reads and writes the resource.
	ReadWrite,
};

/// Description of one render graph resource.
struct RenderResourceDesc {
	/// Unique graph resource name.
	std::string name;
	/// Resource format.
	RenderResourceFormat format = RenderResourceFormat::Rgba8Unorm;
	/// Resource extent.
	ViewportExtent extent;
	/// True when the resource is owned outside the graph.
	bool external = false;
	/// True when resize should update this resource extent and generation.
	bool resize_dependent = true;
	/// Multisample count requested for the resource.
	std::uint8_t sample_count = 1;
};

/// Live render graph resource record.
struct RenderResourceRecord {
	/// Resource descriptor.
	RenderResourceDesc desc;
	/// Generation incremented when resize-dependent resources are resized.
	std::uint64_t generation = 1;
};

/**
 * Return the stable resource-format name.
 *
 * @param format Format to name.
 * @return Static format name.
 */
[[nodiscard]] const char *render_resource_format_name(RenderResourceFormat format) noexcept;
/**
 * Return the stable resource-access name.
 *
 * @param access Access mode to name.
 * @return Static access name.
 */
[[nodiscard]] const char *render_resource_access_name(RenderResourceAccess access) noexcept;

} // namespace crimson
