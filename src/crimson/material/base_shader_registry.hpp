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

#include <span>
#include <vector>

namespace crimson {

/// Immutable lookup table for Crimson base shader definitions.
class BaseShaderRegistry final {
public:
	/// Construct an empty registry.
	BaseShaderRegistry() = default;
	/// Construct a registry from owned definitions.
	explicit BaseShaderRegistry(std::vector<BaseShaderDefinition> definitions);

	/// Find a base shader definition by id, or return `nullptr`.
	[[nodiscard]] const BaseShaderDefinition *find(BaseShaderId id) const noexcept;
	/// Borrow all definitions owned by this registry.
	[[nodiscard]] std::span<const BaseShaderDefinition> definitions() const noexcept;
	/// Return true when the registry contains no definitions.
	[[nodiscard]] bool empty() const noexcept;

private:
	std::vector<BaseShaderDefinition> definitions_;
};

/// Build the built-in V1 base shader registry.
[[nodiscard]] BaseShaderRegistry make_v1_base_shader_registry();

} // namespace crimson
