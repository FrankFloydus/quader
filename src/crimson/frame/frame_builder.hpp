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

#include "crimson/frame/frame_snapshot.hpp"
#include "crimson/viewport_frame.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "crimson/scene/render_world.hpp"
#include "foundation/result.hpp"

namespace crimson {

/// Builds immutable frame snapshots from viewport-facing input data.
class FrameBuilder final {
public:
	/**
	 * Build a renderer snapshot from a viewport frame input.
	 *
	 * @param frame Viewport frame data to validate and copy.
	 * @return Snapshot ready for Crimson submission, or a renderer diagnostic.
	 */
	[[nodiscard]] quader::foundation::Result<FrameSnapshot, RendererDiagnostic> build_snapshot(
			const ViewportFrameInput &frame) const;
};

} // namespace crimson
