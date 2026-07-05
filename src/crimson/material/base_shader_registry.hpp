#pragma once

#include "crimson/material/base_shader.hpp"

#include <span>
#include <vector>

namespace crimson {

class BaseShaderRegistry final {
public:
    BaseShaderRegistry() = default;
    explicit BaseShaderRegistry(std::vector<BaseShaderDefinition> definitions);

    [[nodiscard]] const BaseShaderDefinition* find(BaseShaderId id) const noexcept;
    [[nodiscard]] std::span<const BaseShaderDefinition> definitions() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    std::vector<BaseShaderDefinition> definitions_;
};

[[nodiscard]] BaseShaderRegistry make_v1_base_shader_registry();

} // namespace crimson
