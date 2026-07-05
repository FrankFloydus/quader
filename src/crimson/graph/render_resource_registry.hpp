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

#include "crimson/graph/render_resource_desc.hpp"
#include "foundation/result.hpp"

#include <span>
#include <string>
#include <vector>

namespace crimson {

/// Owns render graph resource records and resize generations.
class RenderResourceRegistry final {
public:
	/**
	 * Add a render resource.
	 *
	 * @param desc Resource descriptor to store.
	 * @return Success, or a validation error string.
	 */
	[[nodiscard]] quader::foundation::Result<void, std::string> add(RenderResourceDesc desc);
	/**
	 * Find an immutable resource by name.
	 *
	 * @param name Resource name.
	 * @return Borrowed record, or `nullptr` when absent.
	 */
	[[nodiscard]] const RenderResourceRecord *find(std::string_view name) const noexcept;
	/**
	 * Find a mutable resource by name.
	 *
	 * @param name Resource name.
	 * @return Borrowed record, or `nullptr` when absent.
	 */
	[[nodiscard]] RenderResourceRecord *find(std::string_view name) noexcept;
	/**
	 * Return registered resources.
	 *
	 * @return Borrowed span valid until registry mutation.
	 */
	[[nodiscard]] std::span<const RenderResourceRecord> resources() const noexcept;
	/**
	 * Return the current resize generation.
	 *
	 * @return Monotonic resize generation.
	 */
	[[nodiscard]] std::uint64_t resize_generation() const noexcept;

	/**
	 * Resize all resize-dependent resources.
	 *
	 * @param extent New extent applied to resize-dependent resources.
	 */
	void resize(ViewportExtent extent);
	/// Remove all resources and reset resize generation.
	void clear() noexcept;

private:
	std::vector<RenderResourceRecord> resources_;
	std::uint64_t resize_generation_ = 1;
};

} // namespace crimson
