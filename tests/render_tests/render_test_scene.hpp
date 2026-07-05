#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace quader::render_tests {

struct RenderTestScene {
    std::string name;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::string capture_color_space;
    bool requires_runtime_capture = false;
};

[[nodiscard]] std::vector<RenderTestScene> planned_v1_golden_scenes();

} // namespace quader::render_tests
