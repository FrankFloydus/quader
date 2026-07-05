#include "ui/viewport/viewport_layout_state.hpp"

#include <algorithm>
#include <cmath>

namespace quader::ui {
namespace {

[[nodiscard]] int safe_extent(int value) noexcept {
	return std::max(1, value);
}

[[nodiscard]] int active_splitter_width(ViewportPixelSize size) noexcept {
	return size.width > 8 && size.height > 8 ? kViewportSplitterWidth : 0;
}

[[nodiscard]] float clamped_fraction(float value) noexcept {
	return std::clamp(value, 0.0F, 1.0F);
}

} // namespace

ViewportLayoutMode ViewportLayoutState::mode() const noexcept {
	return mode_;
}

void ViewportLayoutState::set_mode(ViewportLayoutMode mode) noexcept {
	mode_ = mode;
}

bool ViewportLayoutState::quad_enabled() const noexcept {
	return mode_ == ViewportLayoutMode::Quad;
}

void ViewportLayoutState::set_quad_enabled(bool enabled) noexcept {
	mode_ = enabled ? ViewportLayoutMode::Quad : ViewportLayoutMode::Single;
}

float ViewportLayoutState::vertical_split() const noexcept {
	return vertical_split_;
}

float ViewportLayoutState::horizontal_split() const noexcept {
	return horizontal_split_;
}

void ViewportLayoutState::set_vertical_split(float value) noexcept {
	vertical_split_ = clamped_fraction(value);
}

void ViewportLayoutState::set_horizontal_split(float value) noexcept {
	horizontal_split_ = clamped_fraction(value);
}

ViewportPaneArray ViewportLayoutState::panes_for(ViewportPixelSize size) const {
	const int kWidth = safe_extent(size.width);
	const int kHeight = safe_extent(size.height);

	if (mode_ != ViewportLayoutMode::Quad) {
		return { {
				ViewportPane{ { 0, 0, kWidth, kHeight }, 0, QStringLiteral("Perspective") },
				ViewportPane{},
				ViewportPane{},
				ViewportPane{},
		} };
	}

	const int kGap = active_splitter_width(size);
	const int kSplitX = clamped_split_start(kWidth, vertical_split_, kGap);
	const int kSplitY = clamped_split_start(kHeight, horizontal_split_, kGap);
	const int kLeftWidth = std::max(1, kSplitX);
	const int kRightX = std::min(kWidth - 1, kSplitX + kGap);
	const int kRightWidth = std::max(1, kWidth - kRightX);
	const int kTopHeight = std::max(1, kSplitY);
	const int kBottomY = std::min(kHeight - 1, kSplitY + kGap);
	const int kBottomHeight = std::max(1, kHeight - kBottomY);

	return { {
			ViewportPane{ { 0, 0, kLeftWidth, kTopHeight }, 0, QStringLiteral("Perspective") },
			ViewportPane{ { kRightX, 0, kRightWidth, kTopHeight }, 1, QStringLiteral("Top") },
			ViewportPane{ { 0, kBottomY, kLeftWidth, kBottomHeight }, 2, QStringLiteral("Front") },
			ViewportPane{ { kRightX, kBottomY, kRightWidth, kBottomHeight }, 3, QStringLiteral("Right") },
	} };
}

int ViewportLayoutState::pane_count() const noexcept {
	return pane_count_for_layout(mode_);
}

int ViewportLayoutState::pane_at(ViewportPixelSize size, ViewportPoint point) const {
	const ViewportPaneArray kPanes = panes_for(size);
	for (int index = 0; index < pane_count(); ++index) {
		const ViewportRect &rect = kPanes[static_cast<std::size_t>(index)].rect;
		if (point.x >= rect.x && point.y >= rect.y && point.x < rect.x + rect.width && point.y < rect.y + rect.height) {
			return kPanes[static_cast<std::size_t>(index)].camera_index;
		}
	}

	return -1;
}

ViewportSplitHandle ViewportLayoutState::splitter_at(ViewportPixelSize size, ViewportPoint point) const {
	if (mode_ != ViewportLayoutMode::Quad) {
		return ViewportSplitHandle::None;
	}

	const int kGap = active_splitter_width(size);
	const int kVerticalStart = clamped_split_start(safe_extent(size.width), vertical_split_, kGap);
	const int kHorizontalStart = clamped_split_start(safe_extent(size.height), horizontal_split_, kGap);
	const int kVerticalCenter = kVerticalStart + kGap / 2;
	const int kHorizontalCenter = kHorizontalStart + kGap / 2;
	const int kVerticalDistance = std::abs(static_cast<int>(std::round(point.x)) - kVerticalCenter);
	const int kHorizontalDistance = std::abs(static_cast<int>(std::round(point.y)) - kHorizontalCenter);

	if (kVerticalDistance <= kViewportSplitterHitTolerance && kVerticalDistance <= kHorizontalDistance) {
		return ViewportSplitHandle::Vertical;
	}
	if (kHorizontalDistance <= kViewportSplitterHitTolerance) {
		return ViewportSplitHandle::Horizontal;
	}

	return ViewportSplitHandle::None;
}

void ViewportLayoutState::set_split_from_position(
		ViewportPixelSize size,
		ViewportSplitHandle handle,
		ViewportPoint point) noexcept {
	const int kGap = active_splitter_width(size);

	if (handle == ViewportSplitHandle::Vertical) {
		const float kFraction = static_cast<float>(point.x) / static_cast<float>(safe_extent(size.width));
		const int kStart = clamped_split_start(safe_extent(size.width), kFraction, kGap);
		vertical_split_ = static_cast<float>(kStart) / static_cast<float>(std::max(1, safe_extent(size.width) - kGap));
	} else if (handle == ViewportSplitHandle::Horizontal) {
		const float kFraction = static_cast<float>(point.y) / static_cast<float>(safe_extent(size.height));
		const int kStart = clamped_split_start(safe_extent(size.height), kFraction, kGap);
		horizontal_split_ = static_cast<float>(kStart) / static_cast<float>(std::max(1, safe_extent(size.height) - kGap));
	}
}

ViewportRect ViewportLayoutState::vertical_splitter_rect(ViewportPixelSize size) const noexcept {
	const int kGap = active_splitter_width(size);
	const int kSplitX = clamped_split_start(safe_extent(size.width), vertical_split_, kGap);
	return ViewportRect{ kSplitX, 0, kGap, safe_extent(size.height) };
}

ViewportRect ViewportLayoutState::horizontal_splitter_rect(ViewportPixelSize size) const noexcept {
	const int kGap = active_splitter_width(size);
	const int kSplitY = clamped_split_start(safe_extent(size.height), horizontal_split_, kGap);
	return ViewportRect{ 0, kSplitY, safe_extent(size.width), kGap };
}

int ViewportLayoutState::clamped_split_start(int size, float fraction, int splitter_width) const noexcept {
	const int kAvailable = std::max(1, size - splitter_width);
	const int kMinExtent = std::min(kMinimumViewportExtent, std::max(1, kAvailable / 2));
	const int kMaxStart = std::max(kMinExtent, kAvailable - kMinExtent);
	const int kRequested = static_cast<int>(std::round(kAvailable * clamped_fraction(fraction)));
	return std::clamp(kRequested, kMinExtent, kMaxStart);
}

} // namespace quader::ui
