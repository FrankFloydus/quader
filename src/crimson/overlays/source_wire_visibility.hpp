/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include "crimson/overlays/overlay_system.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace crimson {

/// Camera-aware visibility/depth policy derived from source-wire depth metadata.
class SourceWireDepthStampVisibilityFilter final {
public:
	/// Build per-view/per-object stamp sets from prepared non-rendering metadata.
	explicit SourceWireDepthStampVisibilityFilter(std::span<const SourceWireDepthStampMetadata> stamps);

	/// Return true when no stamp sets were supplied.
	[[nodiscard]] bool empty() const noexcept;

	/// Return true when an editable point should remain visible for the camera.
	[[nodiscard]] bool point_visible(
			std::uint32_t view_index,
			const OverlayElementRef &element,
			quader::math::Vec3 point,
			const RenderCamera &camera) const noexcept;

	/// Return the effective depth mode for a visible editable point.
	[[nodiscard]] OverlayDepthMode point_depth_mode(
			std::uint32_t view_index,
			const OverlayElementRef &element,
			quader::math::Vec3 point,
			const RenderCamera &camera,
			OverlayDepthMode fallback) const noexcept;

private:
	struct StampTriangle {
		TriangleOverlayPrimitive triangle;
		quader::math::Vec3 centroid;
		quader::math::Vec3 normal;
	};

	struct StampSet {
		std::uint32_t view_index = 0;
		RenderObjectId object_id = 0;
		std::vector<StampTriangle> triangles;
		quader::math::Vec3 centroid;
		bool inside_out = false;
	};

	[[nodiscard]] const StampSet *stamp_set_for(
			std::uint32_t view_index,
			RenderObjectId object_id) const noexcept;
	[[nodiscard]] bool camera_inside(const StampSet &set, const RenderCamera &camera) const noexcept;

	std::vector<StampSet> sets_;
};

} // namespace crimson
