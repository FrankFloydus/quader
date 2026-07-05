#include "golden_image.hpp"

#include <limits>

namespace quader::render_tests {
namespace {

[[nodiscard]] std::uint8_t channel_delta(std::uint8_t actual, std::uint8_t expected) noexcept
{
    return actual > expected
        ? static_cast<std::uint8_t>(actual - expected)
        : static_cast<std::uint8_t>(expected - actual);
}

} // namespace

std::size_t expected_rgba8_byte_count(const GoldenImage& image) noexcept
{
    if (image.width == 0 || image.height == 0) {
        return 0;
    }

    const std::uint64_t pixel_count = static_cast<std::uint64_t>(image.width) * image.height;
    const std::uint64_t byte_count = pixel_count * 4ULL;
    if (byte_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return 0;
    }

    return static_cast<std::size_t>(byte_count);
}

bool has_expected_rgba8_byte_count(const GoldenImage& image) noexcept
{
    const std::size_t expected_size = expected_rgba8_byte_count(image);
    return expected_size != 0 && image.rgba8_srgb.size() == expected_size;
}

GoldenImageDiff GoldenImageComparator::compare(
    const GoldenImage& actual,
    const GoldenImage& expected,
    GoldenTolerance tolerance) const
{
    GoldenImageDiff diff;

    if (actual.width != expected.width || actual.height != expected.height) {
        diff.dimensions_match = false;
        diff.passed = false;
        diff.message = "Image dimensions differ.";
        return diff;
    }

    if (!has_expected_rgba8_byte_count(actual) || !has_expected_rgba8_byte_count(expected)) {
        diff.pixel_buffer_sizes_match = false;
        diff.passed = false;
        diff.message = "Image RGBA8 buffers do not match their declared dimensions.";
        return diff;
    }

    std::uint64_t channel_delta_sum = 0;
    for (std::size_t pixel_offset = 0; pixel_offset < actual.rgba8_srgb.size(); pixel_offset += 4) {
        bool pixel_changed = false;
        for (std::size_t channel = 0; channel < 4; ++channel) {
            const std::uint8_t delta = channel_delta(
                actual.rgba8_srgb[pixel_offset + channel],
                expected.rgba8_srgb[pixel_offset + channel]);
            diff.max_channel_delta = delta > diff.max_channel_delta ? delta : diff.max_channel_delta;
            channel_delta_sum += delta;
            pixel_changed = pixel_changed || delta != 0;
        }
        if (pixel_changed) {
            ++diff.changed_pixels;
        }
    }

    diff.mean_channel_delta = static_cast<double>(channel_delta_sum)
        / static_cast<double>(actual.rgba8_srgb.size());
    diff.passed = diff.max_channel_delta <= tolerance.max_channel_delta
        && diff.mean_channel_delta <= tolerance.max_mean_channel_delta
        && diff.changed_pixels <= tolerance.max_changed_pixels;
    diff.message = diff.passed ? "Images are within tolerance." : "Images differ beyond tolerance.";
    return diff;
}

} // namespace quader::render_tests
