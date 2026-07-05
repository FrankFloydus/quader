#pragma once

#include <cstdint>
#include <string>

namespace crimson {

enum class MaterialUiFieldKind : std::uint8_t {
    Parameter,
    TextureSlot,
};

enum class MaterialUiControl : std::uint8_t {
    Float,
    Vec2,
    Color,
    Checkbox,
    Combo,
    Texture,
};

struct MaterialUiFieldDesc {
    std::string label;
    MaterialUiFieldKind kind = MaterialUiFieldKind::Parameter;
    MaterialUiControl control = MaterialUiControl::Float;
    std::string binding_name;
};

} // namespace crimson
