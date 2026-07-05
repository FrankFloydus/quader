#include "crimson/material/material_parameter.hpp"

namespace crimson {

MaterialParameterKind material_parameter_value_kind(const MaterialParameterValue& value) noexcept
{
    return static_cast<MaterialParameterKind>(value.index());
}

const char* material_parameter_kind_name(MaterialParameterKind kind) noexcept
{
    switch (kind) {
    case MaterialParameterKind::Float:
        return "Float";
    case MaterialParameterKind::Vec2:
        return "Vec2";
    case MaterialParameterKind::Vec3:
        return "Vec3";
    case MaterialParameterKind::Vec4:
        return "Vec4";
    case MaterialParameterKind::ColorSrgb:
        return "ColorSrgb";
    case MaterialParameterKind::Bool:
        return "Bool";
    case MaterialParameterKind::Enum:
        return "Enum";
    }

    return "Unknown";
}

} // namespace crimson
