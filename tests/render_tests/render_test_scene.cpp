#include "render_test_scene.hpp"

namespace quader::render_tests {

std::vector<RenderTestScene> planned_v1_golden_scenes()
{
    return {
        RenderTestScene{
            .name = "01_color_space_tone_map_grid",
            .width = 640,
            .height = 360,
            .capture_color_space = "display_srgb_rgba8",
        },
        RenderTestScene{
            .name = "02_overlay_depth_modes",
            .width = 640,
            .height = 360,
            .capture_color_space = "display_srgb_rgba8",
        },
        RenderTestScene{
            .name = "03_picking_id_debug",
            .width = 640,
            .height = 360,
            .capture_color_space = "display_srgb_rgba8",
            .requires_runtime_capture = true,
        },
    };
}

} // namespace quader::render_tests
