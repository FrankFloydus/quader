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

#include <cstdint>
#include <string>
#include <variant>

namespace crimson {

/// Two-component material parameter value.
struct MaterialVec2 {
	/// X component.
	float x = 0.0F;
	/// Y component.
	float y = 0.0F;

	friend bool operator==(const MaterialVec2 &, const MaterialVec2 &) = default;
};

/// Three-component material parameter value.
struct MaterialVec3 {
	/// X component.
	float x = 0.0F;
	/// Y component.
	float y = 0.0F;
	/// Z component.
	float z = 0.0F;

	friend bool operator==(const MaterialVec3 &, const MaterialVec3 &) = default;
};

/// Four-component material parameter value.
struct MaterialVec4 {
	/// X component.
	float x = 0.0F;
	/// Y component.
	float y = 0.0F;
	/// Z component.
	float z = 0.0F;
	/// W component.
	float w = 0.0F;

	friend bool operator==(const MaterialVec4 &, const MaterialVec4 &) = default;
};

/// sRGB color material parameter value.
struct MaterialColorSrgb {
	/// Red channel in normalized sRGB.
	float r = 1.0F;
	/// Green channel in normalized sRGB.
	float g = 1.0F;
	/// Blue channel in normalized sRGB.
	float b = 1.0F;
	/// Alpha channel.
	float a = 1.0F;

	friend bool operator==(const MaterialColorSrgb &, const MaterialColorSrgb &) = default;
};

/// Integer-backed enum material parameter value.
struct MaterialEnumValue {
	/// Stored enum value.
	std::int32_t value = 0;

	friend bool operator==(const MaterialEnumValue &, const MaterialEnumValue &) = default;
};

/// Runtime kind of a material parameter value.
enum class MaterialParameterKind : std::uint8_t {
	Float,     ///< Scalar float.
	Vec2,      ///< Two-component vector.
	Vec3,      ///< Three-component vector.
	Vec4,      ///< Four-component vector.
	ColorSrgb, ///< sRGB color.
	Bool,      ///< Boolean toggle.
	Enum,      ///< Integer-backed enum.
};

/// Variant storing all supported material parameter value types.
using MaterialParameterValue = std::variant<
		float,
		MaterialVec2,
		MaterialVec3,
		MaterialVec4,
		MaterialColorSrgb,
		bool,
		MaterialEnumValue>;

/// Schema entry for one material parameter.
struct MaterialParameterDesc {
	/// Parameter name used for lookup and material storage.
	std::string name;
	/// Expected value kind.
	MaterialParameterKind kind = MaterialParameterKind::Float;
	/// Default value used when material instances omit this parameter.
	MaterialParameterValue default_value = 0.0F;
	/// Minimum value for numeric UI/validation.
	MaterialParameterValue min_value = 0.0F;
	/// Maximum value for numeric UI/validation.
	MaterialParameterValue max_value = 1.0F;
};

/// Concrete material parameter value keyed by schema name.
struct MaterialParameter {
	/// Parameter name.
	std::string name;
	/// Stored value.
	MaterialParameterValue value = 0.0F;
};

/// Return the value kind held by a material parameter variant.
[[nodiscard]] MaterialParameterKind material_parameter_value_kind(const MaterialParameterValue &value) noexcept;
/// Return the stable debug name for a material parameter kind.
[[nodiscard]] const char *material_parameter_kind_name(MaterialParameterKind kind) noexcept;

} // namespace crimson
