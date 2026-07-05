#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace quader::render_tests {

struct GoldenImage {
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	std::vector<std::uint8_t> rgba8_srgb;
};

struct GoldenTolerance {
	std::uint8_t max_channel_delta = 2;
	double max_mean_channel_delta = 0.25;
	std::uint32_t max_changed_pixels = 0;
};

struct GoldenImageDiff {
	std::uint8_t max_channel_delta = 0;
	double mean_channel_delta = 0.0;
	std::uint32_t changed_pixels = 0;
	bool dimensions_match = true;
	bool pixel_buffer_sizes_match = true;
	bool passed = false;
	std::string message;
};

class GoldenImageComparator final {
public:
	[[nodiscard]] GoldenImageDiff compare(
			const GoldenImage &actual,
			const GoldenImage &expected,
			GoldenTolerance tolerance) const;
};

[[nodiscard]] std::size_t expected_rgba8_byte_count(const GoldenImage &image) noexcept;
[[nodiscard]] bool has_expected_rgba8_byte_count(const GoldenImage &image) noexcept;

} // namespace quader::render_tests
