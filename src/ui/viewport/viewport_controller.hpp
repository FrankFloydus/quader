#pragma once

#include "ui/viewport/viewport_camera_controller.hpp"
#include "ui/viewport/viewport_layout_state.hpp"
#include "ui/viewport/viewport_render_host.hpp"

#include <array>

#include <QObject>
#include <QString>

namespace quader::ui {

class ViewportController final : public QObject {
    Q_OBJECT

public:
    explicit ViewportController(IViewportRenderHost& render_host, QObject* parent = nullptr);
    ~ViewportController() override = default;

    [[nodiscard]] bool surface_initialized() const noexcept;
    [[nodiscard]] NavigationMode navigation_mode() const noexcept;

    void initialize_surface(const NativeViewportSurface& surface, ViewportPixelSize size, double device_pixel_ratio);
    void resize_surface(ViewportPixelSize size, double device_pixel_ratio);
    void shutdown_surface();

    void set_quad_viewports_enabled(bool enabled);
    [[nodiscard]] bool quad_viewports_enabled() const noexcept;

    void set_prototype_animation_enabled(bool enabled);
    [[nodiscard]] bool prototype_animation_enabled() const noexcept;

    void render_frame(double elapsed_seconds, float delta_seconds);
    void request_frame();

    [[nodiscard]] ViewportLayoutState& layout_state() noexcept;
    [[nodiscard]] const ViewportLayoutState& layout_state() const noexcept;
    [[nodiscard]] const ViewportCameraController& camera_controller() const noexcept;

    void handle_mouse_press(ViewportMouseButton button, ViewportPoint point, bool shift_modifier, ViewportPixelSize size);
    void handle_mouse_move(ViewportPoint point, ViewportPixelSize size);
    void handle_mouse_release(ViewportMouseButton button);
    void handle_wheel(float wheel_steps, ViewportPoint point, ViewportPixelSize size);
    void handle_key(ViewportKey key, bool pressed, bool auto_repeat);

Q_SIGNALS:
    void renderer_ready(QString renderer_name);
    void viewport_layout_changed(bool quad_enabled, QString layout_name);
    void frame_stats_changed(quader::ui::ViewportFrameStats stats);
    void render_error(QString summary, QString detail);
    void frame_requested();

private:
    [[nodiscard]] ViewportPixelSize safe_size(ViewportPixelSize size) const noexcept;
    [[nodiscard]] ViewportPixelSize pane_size_for_camera(ViewportPixelSize size, int camera_index) const;
    void emit_result_error(const ViewportRenderResult& result);

    IViewportRenderHost& render_host_;
    ViewportLayoutState layout_;
    ViewportCameraController cameras_;
    ViewportPixelSize surface_size_;
    double device_pixel_ratio_ = 1.0;
    bool prototype_animation_enabled_ = true;
    bool surface_initialized_ = false;
};

} // namespace quader::ui
