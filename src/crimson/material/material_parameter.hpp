#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace crimson {

struct MaterialVec2 {
	float x = 0.0F;
	float y = 0.0F;

	friend bool operator==(const MaterialVec2 &, const MaterialVec2 &) = default;
};

struct MaterialVec3 {
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;

	friend bool operator==(const MaterialVec3 &, const MaterialVec3 &) = default;
};

struct MaterialVec4 {
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
	float w = 0.0F;

	friend bool operator==(const MaterialVec4 &, const MaterialVec4 &) = default;
};

struct MaterialColorSrgb {
	float r = 1.0F;
	float g = 1.0F;
	float b = 1.0F;
	float a = 1.0F;

	friend bool operator==(const MaterialColorSrgb &, const MaterialColorSrgb &) = default;
};

struct MaterialEnumValue {
	std::int32_t value = 0;

	friend bool operator==(const MaterialEnumValue &, const MaterialEnumValue &) = default;
};

enum class MaterialParameterKind : std::uint8_t {
	Float,
	Vec2,
	Vec3,
	Vec4,
	ColorSrgb,
	Bool,
	Enum,
};

using MaterialParameterValue = std::variant<
		float,
		MaterialVec2,
		MaterialVec3,
		MaterialVec4,
		MaterialColorSrgb,
		bool,
		MaterialEnumValue>;

struct MaterialParameterDesc {
	std::string name;
	MaterialParameterKind kind = MaterialParameterKind::Float;
	MaterialParameterValue default_value = 0.0F;
	MaterialParameterValue min_value = 0.0F;
	MaterialParameterValue max_value = 1.0F;
};

struct MaterialParameter {
	std::string name;
	MaterialParameterValue value = 0.0F;
};

[[nodiscard]] MaterialParameterKind material_parameter_value_kind(const MaterialParameterValue &value) noexcept;
[[nodiscard]] const char *material_parameter_kind_name(MaterialParameterKind kind) noexcept;

} // namespace crimson
