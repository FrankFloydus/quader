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
		std::vector<std::uint8_t> rgba8_srgb) {
	return quader::render_tests::GoldenImage{
		.width = width,
		.height = height,
		.rgba8_srgb = std::move(rgba8_srgb),
	};
}

TEST(GoldenImageCompare, ExactMatchPassesWithZeroDifferences) {
	const auto kExpected = make_image(2, 1, { 10, 20, 30, 255, 100, 110, 120, 255 });
	const auto &actual = kExpected;

	const quader::render_tests::GoldenImageDiff kDiff =
			quader::render_tests::GoldenImageComparator{}.compare(
					actual,
					kExpected,
					quader::render_tests::GoldenTolerance{});

	EXPECT_TRUE(kDiff.passed);
	EXPECT_TRUE(kDiff.dimensions_match);
	EXPECT_TRUE(kDiff.pixel_buffer_sizes_match);
	EXPECT_EQ(kDiff.max_channel_delta, 0);
	EXPECT_EQ(kDiff.changed_pixels, 0U);
	EXPECT_TRUE(kDiff.mean_channel_delta == 0.0);
}

TEST(GoldenImageCompare, ControlledDeltaReportsMaxMeanAndChangedPixels) {
	const auto kExpected = make_image(2, 1, { 10, 20, 30, 255, 100, 110, 120, 255 });
	const auto kActual = make_image(2, 1, { 12, 21, 30, 255, 100, 110, 119, 255 });

	const quader::render_tests::GoldenImageDiff kStrictDiff =
			quader::render_tests::GoldenImageComparator{}.compare(
					kActual,
					kExpected,
					quader::render_tests::GoldenTolerance{});
	EXPECT_FALSE(kStrictDiff.passed);
	EXPECT_EQ(kStrictDiff.max_channel_delta, 2);
	EXPECT_EQ(kStrictDiff.changed_pixels, 2U);
	EXPECT_TRUE(kStrictDiff.mean_channel_delta > 0.0);

	const quader::render_tests::GoldenImageDiff kTolerantDiff =
			quader::render_tests::GoldenImageComparator{}.compare(
					kActual,
					kExpected,
					quader::render_tests::GoldenTolerance{
							.max_channel_delta = 2,
							.max_mean_channel_delta = 0.5,
							.max_changed_pixels = 2,
					});
	EXPECT_TRUE(kTolerantDiff.passed);
}

TEST(GoldenImageCompare, DimensionMismatchFailsBeforePixelCompare) {
	const auto kExpected = make_image(2, 1, { 0, 0, 0, 255, 255, 255, 255, 255 });
	const auto kActual = make_image(1, 2, { 0, 0, 0, 255, 255, 255, 255, 255 });

	const quader::render_tests::GoldenImageDiff kDiff =
			quader::render_tests::GoldenImageComparator{}.compare(
					kActual,
					kExpected,
					quader::render_tests::GoldenTolerance{});

	EXPECT_FALSE(kDiff.passed);
	EXPECT_FALSE(kDiff.dimensions_match);
	EXPECT_TRUE(kDiff.pixel_buffer_sizes_match);
	EXPECT_EQ(kDiff.changed_pixels, 0U);
}

TEST(GoldenImageCompare, BufferSizeMismatchFailsWithReadableState) {
	const auto kExpected = make_image(1, 1, { 1, 2, 3, 4 });
	const auto kActual = make_image(1, 1, { 1, 2, 3 });

	const quader::render_tests::GoldenImageDiff kDiff =
			quader::render_tests::GoldenImageComparator{}.compare(
					kActual,
					kExpected,
					quader::render_tests::GoldenTolerance{});

	EXPECT_FALSE(kDiff.passed);
	EXPECT_TRUE(kDiff.dimensions_match);
	EXPECT_FALSE(kDiff.pixel_buffer_sizes_match);
	EXPECT_TRUE(!kDiff.message.empty());
}

TEST(GoldenImageCompare, PlannedSceneScaffoldHasNoBaselineCaptureSideEffects) {
	const std::vector<quader::render_tests::RenderTestScene> kScenes =
			quader::render_tests::planned_v1_golden_scenes();

	EXPECT_EQ(kScenes.size(), 3U);
	for (const quader::render_tests::RenderTestScene &scene : kScenes) {
		EXPECT_TRUE(!scene.name.empty());
		EXPECT_TRUE(scene.width > 0);
		EXPECT_TRUE(scene.height > 0);
		EXPECT_EQ(scene.capture_color_space, "display_srgb_rgba8");
	}
	EXPECT_FALSE(kScenes[0].requires_runtime_capture);
	EXPECT_FALSE(kScenes[1].requires_runtime_capture);
	EXPECT_TRUE(kScenes[2].requires_runtime_capture);
}

} // namespace
