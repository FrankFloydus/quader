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

#include "math/scalar.hpp"
#include "math/vec3.hpp"

namespace quader::document {

/// Document-owned PBR material values for mesh objects.
struct PbrMaterial {
	/// Linear base color in normalized RGB channels.
	quader::math::Vec3 base_color{ 1.0F, 1.0F, 1.0F };
	/// Roughness in the normalized range `[0, 1]`.
	float roughness = 1.0F;
	/// Metallic factor in the normalized range `[0, 1]`.
	float metallic = 0.0F;

	/// Compare material values exactly for command and document state tests.
	friend bool operator==(PbrMaterial left, PbrMaterial right) noexcept {
		return left.base_color.x == right.base_color.x && left.base_color.y == right.base_color.y && left.base_color.z == right.base_color.z && left.roughness == right.roughness && left.metallic == right.metallic;
	}
};

/**
 * Return the default material assigned to newly created mesh objects.
 *
 * @return White, fully rough, non-metallic PBR material values.
 */
[[nodiscard]] constexpr PbrMaterial default_mesh_material() noexcept {
	return PbrMaterial{};
}

/**
 * Return the default material assigned to newly committed Box tool meshes.
 *
 * @return White, fully rough, non-metallic PBR material values.
 */
[[nodiscard]] constexpr PbrMaterial default_box_material() noexcept {
	return default_mesh_material();
}

/**
 * Check whether all material scalar values are finite.
 *
 * @param material Material to validate.
 * @return True when every stored value is finite.
 */
[[nodiscard]] inline bool is_finite(PbrMaterial material) noexcept {
	return quader::math::is_finite(material.base_color) && quader::math::is_finite(material.roughness) && quader::math::is_finite(material.metallic);
}

/**
 * Check whether a material is finite and within normalized PBR ranges.
 *
 * @param material Material to validate.
 * @return True when the material is valid for document storage and rendering adapters.
 */
[[nodiscard]] inline bool is_valid(PbrMaterial material) noexcept {
	return is_finite(material) && material.base_color.x >= 0.0F && material.base_color.x <= 1.0F && material.base_color.y >= 0.0F && material.base_color.y <= 1.0F && material.base_color.z >= 0.0F && material.base_color.z <= 1.0F && material.roughness >= 0.0F && material.roughness <= 1.0F && material.metallic >= 0.0F && material.metallic <= 1.0F;
}

} // namespace quader::document
