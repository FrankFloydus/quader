#include "ui/viewport/viewport_layout_state.hpp"

#include <algorithm>
#include <cmath>

namespace quader::ui {
namespace {

[[nodiscard]] int safe_extent(int value) noexcept
{
    return std::max(1, value);
}

[[nodiscard]] int active_splitter_width(ViewportPixelSize size) noexcept
{
    return size.width > 8 && size.height > 8 ? kViewportSplitterWidth : 0;
}

[[nodiscard]] float clamped_fraction(float value) noexcept
{
    return std::clamp(value, 0.0F, 1.0F);
}

} // namespace

ViewportLayoutMode ViewportLayoutState::mode() const noexcept
{
    return mode_;
}

void ViewportLayoutState::set_mode(ViewportLayoutMode mode) noexcept
{
    mode_ = mode;
}

bool ViewportLayoutState::quad_enabled() const noexcept
{
    return mode_ == ViewportLayoutMode::Quad;
}

void ViewportLayoutState::set_quad_enabled(bool enabled) noexcept
{
    mode_ = enabled ? ViewportLayoutMode::Quad : ViewportLayoutMode::Single;
}

float ViewportLayoutState::vertical_split() const noexcept
{
    return vertical_split_;
}

float ViewportLayoutState::horizontal_split() const noexcept
{
    return horizontal_split_;
}

void ViewportLayoutState::set_vertical_split(float value) noexcept
{
    vertical_split_ = clamped_fraction(value);
}

void ViewportLayoutState::set_horizontal_split(float value) noexcept
{
    horizontal_split_ = clamped_fraction(value);
}

ViewportPaneArray ViewportLayoutState::panes_for(ViewportPixelSize size) const
{
    const int width = safe_extent(size.width);
    const int height = safe_extent(size.height);

    if (mode_ != ViewportLayoutMode::Quad) {
        return {{
            ViewportPane{{0, 0, width, height}, 0, QStringLiteral("Perspective")},
            ViewportPane{},
            ViewportPane{},
            ViewportPane{},
        }};
    }

    const int gap = active_splitter_width(size);
    const int split_x = clamped_split_start(width, vertical_split_, gap);
    const int split_y = clamped_split_start(height, horizontal_split_, gap);
    const int left_width = std::max(1, split_x);
    const int right_x = std::min(width - 1, split_x + gap);
    const int right_width = std::max(1, width - right_x);
    const int top_height = std::max(1, split_y);
    const int bottom_y = std::min(height - 1, split_y + gap);
    const int bottom_height = std::max(1, height - bottom_y);

    return {{
        ViewportPane{{0, 0, left_width, top_height}, 0, QStringLiteral("Perspective")},
        ViewportPane{{right_x, 0, right_width, top_height}, 1, QStringLiteral("Top")},
        ViewportPane{{0, bottom_y, left_width, bottom_height}, 2, QStringLiteral("Front")},
        ViewportPane{{right_x, bottom_y, right_width, bottom_height}, 3, QStringLiteral("Right")},
    }};
}

int ViewportLayoutState::pane_count() const noexcept
{
    return pane_count_for_layout(mode_);
}

int ViewportLayoutState::pane_at(ViewportPixelSize size, ViewportPoint point) const
{
    const ViewportPaneArray panes = panes_for(size);
    for (int index = 0; index < pane_count(); ++index) {
        const ViewportRect& rect = panes[static_cast<std::size_t>(index)].rect;
        if (point.x >= rect.x && point.y >= rect.y
            && point.x < rect.x + rect.width
            && point.y < rect.y + rect.height) {
            return panes[static_cast<std::size_t>(index)].camera_index;
        }
    }

    return -1;
}

ViewportSplitHandle ViewportLayoutState::splitter_at(ViewportPixelSize size, ViewportPoint point) const
{
    if (mode_ != ViewportLayoutMode::Quad) {
        return ViewportSplitHandle::None;
    }

    const int gap = active_splitter_width(size);
    const int vertical_start = clamped_split_start(safe_extent(size.width), vertical_split_, gap);
    const int horizontal_start = clamped_split_start(safe_extent(size.height), horizontal_split_, gap);
    const int vertical_center = vertical_start + gap / 2;
    const int horizontal_center = horizontal_start + gap / 2;
    const int vertical_distance = std::abs(static_cast<int>(std::round(point.x)) - vertical_center);
    const int horizontal_distance = std::abs(static_cast<int>(std::round(point.y)) - horizontal_center);

    if (vertical_distance <= kViewportSplitterHitTolerance && vertical_distance <= horizontal_distance) {
        return ViewportSplitHandle::Vertical;
    }
    if (horizontal_distance <= kViewportSplitterHitTolerance) {
        return ViewportSplitHandle::Horizontal;
    }

    return ViewportSplitHandle::None;
}

void ViewportLayoutState::set_split_from_position(
    ViewportPixelSize size,
    ViewportSplitHandle handle,
    ViewportPoint point) noexcept
{
    const int gap = active_splitter_width(size);

    if (handle == ViewportSplitHandle::Vertical) {
        const float fraction = static_cast<float>(point.x) / static_cast<float>(safe_extent(size.width));
        const int start = clamped_split_start(safe_extent(size.width), fraction, gap);
        vertical_split_ = static_cast<float>(start) / static_cast<float>(std::max(1, safe_extent(size.width) - gap));
    } else if (handle == ViewportSplitHandle::Horizontal) {
        const float fraction = static_cast<float>(point.y) / static_cast<float>(safe_extent(size.height));
        const int start = clamped_split_start(safe_extent(size.height), fraction, gap);
        horizontal_split_ = static_cast<float>(start) / static_cast<float>(std::max(1, safe_extent(size.height) - gap));
    }
}

ViewportRect ViewportLayoutState::vertical_splitter_rect(ViewportPixelSize size) const noexcept
{
    const int gap = active_splitter_width(size);
    const int split_x = clamped_split_start(safe_extent(size.width), vertical_split_, gap);
    return ViewportRect{split_x, 0, gap, safe_extent(size.height)};
}

ViewportRect ViewportLayoutState::horizontal_splitter_rect(ViewportPixelSize size) const noexcept
{
    const int gap = active_splitter_width(size);
    const int split_y = clamped_split_start(safe_extent(size.height), horizontal_split_, gap);
    return ViewportRect{0, split_y, safe_extent(size.width), gap};
}

int ViewportLayoutState::clamped_split_start(int size, float fraction, int splitter_width) const noexcept
{
    const int available = std::max(1, size - splitter_width);
    const int min_extent = std::min(kMinimumViewportExtent, std::max(1, available / 2));
    const int max_start = std::max(min_extent, available - min_extent);
    const int requested = static_cast<int>(std::round(available * clamped_fraction(fraction)));
    return std::clamp(requested, min_extent, max_start);
}

} // namespace quader::ui
