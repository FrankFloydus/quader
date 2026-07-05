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

#include <cstdint>
#include <string>
#include <vector>

namespace quader::render_tests {

/// Metadata for a planned render-test scene.
struct RenderTestScene {
	/// Stable scene name.
	std::string name;
	/// Expected capture width in pixels.
	std::uint32_t width = 0;
	/// Expected capture height in pixels.
	std::uint32_t height = 0;
	/// Color space expected for capture comparison.
	std::string capture_color_space;
	/// True when the scene needs a live renderer capture.
	bool requires_runtime_capture = false;
};

/**
 * Return the planned V1 golden-image scene metadata.
 *
 * @return Scene descriptors used by render-test scaffolding.
 */
[[nodiscard]] std::vector<RenderTestScene> planned_v1_golden_scenes();

} // namespace quader::render_tests
