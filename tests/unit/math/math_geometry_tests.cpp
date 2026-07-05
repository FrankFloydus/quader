/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "foundation/logging.hpp"
#include "geometry/normals.hpp"
#include "math/aabb.hpp"
#include "math/vec2.hpp"
#include "math/vec3.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <type_traits>

namespace {

TEST(MathGeometry, MathTypesRemainAggregateStandardLayoutValueTypes) {
	using quader::math::Aabb;
	using quader::math::Vec2;
	using quader::math::Vec3;

	EXPECT_TRUE(std::is_aggregate_v<Vec2>);
	EXPECT_TRUE(std::is_standard_layout_v<Vec2>);
	EXPECT_TRUE(std::is_trivially_copyable_v<Vec2>);
	EXPECT_EQ(offsetof(Vec2, x), 0U);
	EXPECT_EQ(offsetof(Vec2, y), sizeof(float));
	EXPECT_EQ(sizeof(Vec2), sizeof(float) * 2U);

	EXPECT_TRUE(std::is_aggregate_v<Vec3>);
	EXPECT_TRUE(std::is_standard_layout_v<Vec3>);
	EXPECT_TRUE(std::is_trivially_copyable_v<Vec3>);
	EXPECT_EQ(offsetof(Vec3, x), 0U);
	EXPECT_EQ(offsetof(Vec3, y), sizeof(float));
	EXPECT_EQ(offsetof(Vec3, z), sizeof(float) * 2U);
	EXPECT_EQ(sizeof(Vec3), sizeof(float) * 3U);

	EXPECT_TRUE(std::is_aggregate_v<Aabb>);
	EXPECT_TRUE(std::is_standard_layout_v<Aabb>);
	EXPECT_TRUE(std::is_trivially_copyable_v<Aabb>);
}

TEST(MathGeometry, ScalarComparisonsUseInclusiveDefaultEpsilon) {
	const float kJustOutsideEpsilon = std::nextafter(quader::math::kDefaultEpsilon, 1.0F);

	EXPECT_TRUE(quader::math::nearly_equal(0.0F, quader::math::kDefaultEpsilon));
	EXPECT_FALSE(quader::math::nearly_equal(0.0F, kJustOutsideEpsilon));
	EXPECT_TRUE(quader::math::is_near_zero(quader::math::kDefaultEpsilon));
	EXPECT_FALSE(quader::math::is_near_zero(kJustOutsideEpsilon));
}

TEST(MathGeometry, FiniteChecksRejectNanAndInfinityComponentWise) {
	const float kInfinity = std::numeric_limits<float>::infinity();
	const float kNan = std::numeric_limits<float>::quiet_NaN();

	EXPECT_TRUE(quader::math::is_finite(1.0F));
	EXPECT_FALSE(quader::math::is_finite(kInfinity));
	EXPECT_FALSE(quader::math::is_finite(-kInfinity));
	EXPECT_FALSE(quader::math::is_finite(kNan));

	EXPECT_TRUE(quader::math::is_finite(quader::math::Vec2{ 1.0F, 2.0F }));
	EXPECT_FALSE(quader::math::is_finite(quader::math::Vec2{ 1.0F, kInfinity }));
	EXPECT_TRUE(quader::math::is_finite(quader::math::Vec3{ 1.0F, 2.0F, 3.0F }));
	EXPECT_FALSE(quader::math::is_finite(quader::math::Vec3{ 1.0F, kNan, 3.0F }));
}

TEST(MathGeometry, Vec2OperationsPreserveQuaderContract) {
	using quader::math::Vec2;

	EXPECT_TRUE(quader::math::nearly_equal(Vec2{ 1.0F, 2.0F } + Vec2{ 3.0F, -4.0F },
			Vec2{ 4.0F, -2.0F }));
	EXPECT_TRUE(quader::math::nearly_equal(Vec2{ 1.0F, 2.0F } - Vec2{ 3.0F, -4.0F },
			Vec2{ -2.0F, 6.0F }));
	EXPECT_TRUE(quader::math::nearly_equal(Vec2{ 2.0F, -3.0F } * 2.5F, Vec2{ 5.0F, -7.5F }));
	EXPECT_FLOAT_EQ(quader::math::dot(Vec2{ 2.0F, 3.0F }, Vec2{ 4.0F, -5.0F }), -7.0F);
	EXPECT_FLOAT_EQ(quader::math::length_squared(Vec2{ 3.0F, 4.0F }), 25.0F);
}

TEST(MathGeometry, Vec3OperationsPreserveQuaderContract) {
	using quader::math::Vec3;

	const Vec3 kXAxis{ 1.0F, 0.0F, 0.0F };
	const Vec3 kYAxis{ 0.0F, 1.0F, 0.0F };
	const Vec3 kZAxis = quader::math::cross(kXAxis, kYAxis);

	EXPECT_TRUE(quader::math::nearly_equal(Vec3{ 1.0F, 2.0F, 3.0F } + Vec3{ 4.0F, -5.0F, 6.0F },
			Vec3{ 5.0F, -3.0F, 9.0F }));
	EXPECT_TRUE(quader::math::nearly_equal(Vec3{ 1.0F, 2.0F, 3.0F } - Vec3{ 4.0F, -5.0F, 6.0F },
			Vec3{ -3.0F, 7.0F, -3.0F }));
	EXPECT_TRUE(quader::math::nearly_equal(Vec3{ 2.0F, -3.0F, 4.0F } * 2.5F,
			Vec3{ 5.0F, -7.5F, 10.0F }));
	EXPECT_TRUE(quader::math::nearly_equal(Vec3{ 6.0F, -8.0F, 10.0F } / 2.0F,
			Vec3{ 3.0F, -4.0F, 5.0F }));
	EXPECT_TRUE(quader::math::nearly_equal(kZAxis, Vec3{ 0.0F, 0.0F, 1.0F }));
	EXPECT_EQ(quader::math::dot(kXAxis, kYAxis), 0.0F);
	EXPECT_FLOAT_EQ(quader::math::dot(Vec3{ 2.0F, 3.0F, 4.0F }, Vec3{ 5.0F, -6.0F, 7.0F }), 20.0F);
	EXPECT_FLOAT_EQ(quader::math::length_squared(Vec3{ 2.0F, 3.0F, 6.0F }), 49.0F);
	EXPECT_FLOAT_EQ(quader::math::length(Vec3{ 2.0F, 3.0F, 6.0F }), 7.0F);
	EXPECT_TRUE(quader::math::nearly_equal(quader::math::normalized(Vec3{ 0.0F, 0.0F, 5.0F }),
			Vec3{ 0.0F, 0.0F, 1.0F }));
}

TEST(MathGeometry, NormalizedReturnsZeroForZeroAndNearZeroVectors) {
	using quader::math::Vec3;

	EXPECT_TRUE(quader::math::nearly_equal(quader::math::normalized(Vec3{}), Vec3{}));
	EXPECT_TRUE(quader::math::nearly_equal(quader::math::normalized(Vec3{ quader::math::kDefaultEpsilon, 0.0F, 0.0F }),
			Vec3{}));
	EXPECT_TRUE(quader::math::nearly_equal(quader::math::normalized(Vec3{ quader::math::kDefaultEpsilon * 0.5F,
												   0.0F,
												   0.0F }),
			Vec3{}));
	EXPECT_TRUE(quader::math::nearly_equal(quader::math::normalized(Vec3{ quader::math::kDefaultEpsilon * 2.0F,
												   0.0F,
												   0.0F }),
			Vec3{ 1.0F, 0.0F, 0.0F }));
}

TEST(MathGeometry, CrossProductUsesRightHandedOrientation) {
	using quader::math::Vec3;

	EXPECT_TRUE(quader::math::nearly_equal(quader::math::cross(Vec3{ 1.0F, 0.0F, 0.0F },
												   Vec3{ 0.0F, 1.0F, 0.0F }),
			Vec3{ 0.0F, 0.0F, 1.0F }));
	EXPECT_TRUE(quader::math::nearly_equal(quader::math::cross(Vec3{ 0.0F, 1.0F, 0.0F },
												   Vec3{ 1.0F, 0.0F, 0.0F }),
			Vec3{ 0.0F, 0.0F, -1.0F }));
}

TEST(MathGeometry, PolygonNormalAndDegenerateDetection) {
	using quader::math::Vec3;

	const std::array<Vec3, 3> kTriangle{
		Vec3{ 0.0F, 0.0F, 0.0F },
		Vec3{ 1.0F, 0.0F, 0.0F },
		Vec3{ 0.0F, 1.0F, 0.0F },
	};
	const std::array<Vec3, 3> kCollinear{
		Vec3{ 0.0F, 0.0F, 0.0F },
		Vec3{ 1.0F, 0.0F, 0.0F },
		Vec3{ 2.0F, 0.0F, 0.0F },
	};

	EXPECT_TRUE(quader::math::nearly_equal(quader::geometry::polygon_normal(kTriangle),
			Vec3{ 0.0F, 0.0F, 1.0F }));
	EXPECT_FALSE(quader::geometry::has_degenerate_area(kTriangle));
	EXPECT_TRUE(quader::geometry::has_degenerate_area(kCollinear));
}

TEST(MathGeometry, AabbDefaultsEmptyAndExpandsToPoints) {
	quader::math::Aabb bounds;
	EXPECT_TRUE(quader::math::empty(bounds));
	EXPECT_TRUE(std::isinf(bounds.min.x));
	EXPECT_TRUE(std::isinf(bounds.min.y));
	EXPECT_TRUE(std::isinf(bounds.min.z));
	EXPECT_TRUE(std::isinf(bounds.max.x));
	EXPECT_TRUE(std::isinf(bounds.max.y));
	EXPECT_TRUE(std::isinf(bounds.max.z));
	EXPECT_GT(bounds.min.x, bounds.max.x);

	quader::math::expand(bounds, quader::math::Vec3{ 2.0F, -1.0F, 3.0F });
	quader::math::expand(bounds, quader::math::Vec3{ -2.0F, 4.0F, 1.0F });

	EXPECT_FALSE(quader::math::empty(bounds));
	EXPECT_TRUE(quader::math::nearly_equal(bounds.min, quader::math::Vec3{ -2.0F, -1.0F, 1.0F }));
	EXPECT_TRUE(quader::math::nearly_equal(bounds.max, quader::math::Vec3{ 2.0F, 4.0F, 3.0F }));
	EXPECT_TRUE(quader::math::nearly_equal(quader::math::center(bounds),
			quader::math::Vec3{ 0.0F, 1.5F, 2.0F }));
}

} // namespace
