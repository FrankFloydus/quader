#include "ui/viewport/viewport_render_host.hpp"

#include <utility>

namespace quader::ui {

ViewportRenderResult ViewportRenderResult::success(
    QString renderer_name,
    std::vector<ViewportPickResult> completed_pick_results)
{
    return ViewportRenderResult{
        .ok = true,
        .renderer_name = std::move(renderer_name),
        .completed_pick_results = std::move(completed_pick_results),
    };
}

ViewportRenderResult ViewportRenderResult::failure(QString summary, QString detail)
{
    return ViewportRenderResult{
        .ok = false,
        .summary = std::move(summary),
        .detail = std::move(detail),
    };
}

} // namespace quader::ui
