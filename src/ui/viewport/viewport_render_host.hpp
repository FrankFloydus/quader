#pragma once

#include "ui/viewport/viewport_types.hpp"

#include <optional>
#include <vector>

#include <QString>

namespace quader::ui {

struct ViewportRenderResult {
    bool ok = true;
    QString summary;
    QString detail;
    QString renderer_name;
    std::vector<ViewportPickResult> completed_pick_results;

    [[nodiscard]] static ViewportRenderResult success(
        QString renderer_name = {},
        std::vector<ViewportPickResult> completed_pick_results = {});
    [[nodiscard]] static ViewportRenderResult failure(QString summary, QString detail = {});
};

class IViewportRenderHost {
public:
    virtual ~IViewportRenderHost() = default;

    [[nodiscard]] virtual ViewportRenderResult initialize_surface(
        const NativeViewportSurface& surface,
        ViewportPixelSize size,
        double device_pixel_ratio) = 0;
    [[nodiscard]] virtual ViewportRenderResult resize_surface(
        ViewportPixelSize size,
        double device_pixel_ratio) = 0;
    [[nodiscard]] virtual ViewportRenderResult render_frame(const ViewportRenderRequest& request) = 0;
    virtual void shutdown_surface() = 0;

    [[nodiscard]] virtual std::optional<ViewportFrameStats> latest_frame_stats() const = 0;
    [[nodiscard]] virtual std::optional<ViewportDiagnosticsSnapshot> latest_diagnostics() const = 0;
    [[nodiscard]] virtual QString renderer_name() const = 0;
};

} // namespace quader::ui
