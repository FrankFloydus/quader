#include "golden_image.hpp"
#include "render_test_scene.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <utility>
#include <vector>

namespace {

quader::render_tests::GoldenImage make_image(
    std::uint32_t width,
    std::uint32_t height,
    std::vector<std::uint8_t> rgba8_srgb)
{
    return quader::render_tests::GoldenImage{
        .width = width,
        .height = height,
        .rgba8_srgb = std::move(rgba8_srgb),
    };
}

TEST(GoldenImageCompare, ExactMatchPassesWithZeroDifferences)
{
    const auto expected = make_image(2, 1, {10, 20, 30, 255, 100, 110, 120, 255});
    const auto actual = expected;

    const quader::render_tests::GoldenImageDiff diff =
        quader::render_tests::GoldenImageComparator{}.compare(
            actual,
            expected,
            quader::render_tests::GoldenTolerance{});

    EXPECT_TRUE(diff.passed);
    EXPECT_TRUE(diff.dimensions_match);
    EXPECT_TRUE(diff.pixel_buffer_sizes_match);
    EXPECT_EQ(diff.max_channel_delta, 0);
    EXPECT_EQ(diff.changed_pixels, 0U);
    EXPECT_TRUE(diff.mean_channel_delta == 0.0);
}

TEST(GoldenImageCompare, ControlledDeltaReportsMaxMeanAndChangedPixels)
{
    const auto expected = make_image(2, 1, {10, 20, 30, 255, 100, 110, 120, 255});
    const auto actual = make_image(2, 1, {12, 21, 30, 255, 100, 110, 119, 255});

    const quader::render_tests::GoldenImageDiff strict_diff =
        quader::render_tests::GoldenImageComparator{}.compare(
            actual,
            expected,
            quader::render_tests::GoldenTolerance{});
    EXPECT_FALSE(strict_diff.passed);
    EXPECT_EQ(strict_diff.max_channel_delta, 2);
    EXPECT_EQ(strict_diff.changed_pixels, 2U);
    EXPECT_TRUE(strict_diff.mean_channel_delta > 0.0);

    const quader::render_tests::GoldenImageDiff tolerant_diff =
        quader::render_tests::GoldenImageComparator{}.compare(
            actual,
            expected,
            quader::render_tests::GoldenTolerance{
                .max_channel_delta = 2,
                .max_mean_channel_delta = 0.5,
                .max_changed_pixels = 2,
            });
    EXPECT_TRUE(tolerant_diff.passed);
}

TEST(GoldenImageCompare, DimensionMismatchFailsBeforePixelCompare)
{
    const auto expected = make_image(2, 1, {0, 0, 0, 255, 255, 255, 255, 255});
    const auto actual = make_image(1, 2, {0, 0, 0, 255, 255, 255, 255, 255});

    const quader::render_tests::GoldenImageDiff diff =
        quader::render_tests::GoldenImageComparator{}.compare(
            actual,
            expected,
            quader::render_tests::GoldenTolerance{});

    EXPECT_FALSE(diff.passed);
    EXPECT_FALSE(diff.dimensions_match);
    EXPECT_TRUE(diff.pixel_buffer_sizes_match);
    EXPECT_EQ(diff.changed_pixels, 0U);
}

TEST(GoldenImageCompare, BufferSizeMismatchFailsWithReadableState)
{
    const auto expected = make_image(1, 1, {1, 2, 3, 4});
    const auto actual = make_image(1, 1, {1, 2, 3});

    const quader::render_tests::GoldenImageDiff diff =
        quader::render_tests::GoldenImageComparator{}.compare(
            actual,
            expected,
            quader::render_tests::GoldenTolerance{});

    EXPECT_FALSE(diff.passed);
    EXPECT_TRUE(diff.dimensions_match);
    EXPECT_FALSE(diff.pixel_buffer_sizes_match);
    EXPECT_TRUE(!diff.message.empty());
}

TEST(GoldenImageCompare, PlannedSceneScaffoldHasNoBaselineCaptureSideEffects)
{
    const std::vector<quader::render_tests::RenderTestScene> scenes =
        quader::render_tests::planned_v1_golden_scenes();

    EXPECT_EQ(scenes.size(), 3U);
    for (const quader::render_tests::RenderTestScene& scene : scenes) {
        EXPECT_TRUE(!scene.name.empty());
        EXPECT_TRUE(scene.width > 0);
        EXPECT_TRUE(scene.height > 0);
        EXPECT_EQ(scene.capture_color_space, "display_srgb_rgba8");
    }
    EXPECT_FALSE(scenes[0].requires_runtime_capture);
    EXPECT_FALSE(scenes[1].requires_runtime_capture);
    EXPECT_TRUE(scenes[2].requires_runtime_capture);
}

} // namespace
