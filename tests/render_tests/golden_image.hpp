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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace quader::render_tests {

/// In-memory RGBA8 sRGB image used by golden-image tests.
struct GoldenImage {
	/// Image width in pixels.
	std::uint32_t width = 0;
	/// Image height in pixels.
	std::uint32_t height = 0;
	/// Pixel bytes in RGBA8 sRGB row-major order.
	std::vector<std::uint8_t> rgba8_srgb;
};

/// Pixel-difference tolerance for golden-image comparison.
struct GoldenTolerance {
	/// Maximum absolute channel difference allowed for any channel.
	std::uint8_t max_channel_delta = 2;
	/// Maximum mean absolute channel difference.
	double max_mean_channel_delta = 0.25;
	/// Maximum number of pixels allowed to differ.
	std::uint32_t max_changed_pixels = 0;
};

/// Difference report returned by golden-image comparison.
struct GoldenImageDiff {
	/// Maximum channel difference observed.
	std::uint8_t max_channel_delta = 0;
	/// Mean channel difference observed.
	double mean_channel_delta = 0.0;
	/// Number of pixels with at least one changed channel.
	std::uint32_t changed_pixels = 0;
	/// True when image dimensions match.
	bool dimensions_match = true;
	/// True when pixel buffer sizes match expected dimensions.
	bool pixel_buffer_sizes_match = true;
	/// True when the diff satisfies the requested tolerance.
	bool passed = false;
	/// Human-readable comparison message.
	std::string message;
};

/// Compares two in-memory golden images with a tolerance.
class GoldenImageComparator final {
public:
	/**
	 * Compare actual and expected images.
	 *
	 * @param actual Image produced by the renderer/test.
	 * @param expected Golden reference image.
	 * @param tolerance Accepted difference thresholds.
	 * @return Difference report and pass/fail state.
	 */
	[[nodiscard]] GoldenImageDiff compare(
			const GoldenImage &actual,
			const GoldenImage &expected,
			GoldenTolerance tolerance) const;
};

/**
 * Compute expected RGBA8 byte count for an image.
 *
 * @param image Image whose dimensions are inspected.
 * @return `width * height * 4`.
 */
[[nodiscard]] std::size_t expected_rgba8_byte_count(const GoldenImage &image) noexcept;
/**
 * Check whether an image's pixel buffer matches its dimensions.
 *
 * @param image Image to validate.
 * @return True when `rgba8_srgb.size()` equals the expected byte count.
 */
[[nodiscard]] bool has_expected_rgba8_byte_count(const GoldenImage &image) noexcept;

} // namespace quader::render_tests
