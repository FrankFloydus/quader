#pragma once

#include "ui/viewport/viewport_types.hpp"

namespace quader::ui {

class ViewportLayoutState final {
public:
    [[nodiscard]] ViewportLayoutMode mode() const noexcept;
    void set_mode(ViewportLayoutMode mode) noexcept;

    [[nodiscard]] bool quad_enabled() const noexcept;
    void set_quad_enabled(bool enabled) noexcept;

    [[nodiscard]] float vertical_split() const noexcept;
    [[nodiscard]] float horizontal_split() const noexcept;
    void set_vertical_split(float value) noexcept;
    void set_horizontal_split(float value) noexcept;

    [[nodiscard]] ViewportPaneArray panes_for(ViewportPixelSize size) const;
    [[nodiscard]] int pane_count() const noexcept;
    [[nodiscard]] int pane_at(ViewportPixelSize size, ViewportPoint point) const;

    [[nodiscard]] ViewportSplitHandle splitter_at(ViewportPixelSize size, ViewportPoint point) const;
    void set_split_from_position(ViewportPixelSize size, ViewportSplitHandle handle, ViewportPoint point) noexcept;
    [[nodiscard]] ViewportRect vertical_splitter_rect(ViewportPixelSize size) const noexcept;
    [[nodiscard]] ViewportRect horizontal_splitter_rect(ViewportPixelSize size) const noexcept;

private:
    [[nodiscard]] int clamped_split_start(int size, float fraction, int splitter_width) const noexcept;

    ViewportLayoutMode mode_ = ViewportLayoutMode::Single;
    float vertical_split_ = 0.5F;
    float horizontal_split_ = 0.5F;
};

} // namespace quader::ui
