/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include <gtest/gtest.h>

#include "geometry/normals.hpp"
#include "mesh/core/mesh_traversal.hpp"
#include "mesh/core/mesh_validation.hpp"
#include "mesh/ops/box_builder.hpp"

#include <array>
#include <limits>
#include <span>
#include <vector>

namespace {

[[nodiscard]] quader::math::Vec3 scale(quader::math::Vec3 value, float factor) noexcept {
	return value * factor;
}

[[nodiscard]] quader::math::Vec3 average_points(std::span<const quader::math::Vec3> points) noexcept {
	quader::math::Vec3 average;
	for (quader::math::Vec3 point : points) {
		average = average + point;
	}
	return points.empty() ? average : average / static_cast<float>(points.size());
}

[[nodiscard]] std::vector<quader::math::Vec3> face_points(
		const quader::mesh::Polyhedron &mesh,
		quader::mesh::FaceId face) {
	auto vertices = quader::mesh::face_vertices(mesh, face);
	EXPECT_TRUE(vertices);
	std::vector<quader::math::Vec3> points;
	if (!vertices) {
		return points;
	}

	points.reserve(vertices.value().size());
	for (const quader::mesh::VertexId kVertex : vertices.value()) {
		auto position = mesh.vertex_position(kVertex);
		EXPECT_TRUE(position);
		if (position) {
			points.push_back(position.value());
		}
	}
	return points;
}

[[nodiscard]] quader::math::Vec3 mesh_center(const quader::mesh::Polyhedron &mesh) {
	std::vector<quader::math::Vec3> points;
	points.reserve(mesh.vertex_count());
	for (const quader::mesh::VertexId kVertex : mesh.vertex_ids()) {
		auto position = mesh.vertex_position(kVertex);
		EXPECT_TRUE(position);
		if (position) {
			points.push_back(position.value());
		}
	}
	return average_points(points);
}

void expect_faces_oriented_outward(const quader::mesh::Polyhedron &mesh) {
	const quader::math::Vec3 kCenter = mesh_center(mesh);
	for (const quader::mesh::FaceId kFace : mesh.face_ids()) {
		const std::vector<quader::math::Vec3> kPoints = face_points(mesh, kFace);
		ASSERT_GE(kPoints.size(), 3U);
		const quader::math::Vec3 kNormal = quader::geometry::polygon_normal(kPoints);
		const quader::math::Vec3 kFaceCenter = average_points(kPoints);
		EXPECT_GT(quader::math::dot(kNormal, kFaceCenter - kCenter), 0.0F);
	}
}

TEST(MeshOps, BoxFromBoundsCreatesClosedSixFaceMesh) {
	auto mesh = quader::mesh::ops::make_box_from_bounds(quader::mesh::ops::BoxDimensions{
			.min = { -1.0F, -2.0F, -3.0F },
			.max = { 4.0F, 5.0F, 6.0F },
	});
	ASSERT_TRUE(mesh);

	EXPECT_EQ(mesh.value().vertex_count(), 8U);
	EXPECT_EQ(mesh.value().edge_count(), 12U);
	EXPECT_EQ(mesh.value().face_count(), 6U);
	EXPECT_TRUE(quader::mesh::validate_mesh(mesh.value()).ok());
}

TEST(MeshOps, BoxFromBoundsNormalizesReversedBounds) {
	auto mesh = quader::mesh::ops::make_box_from_bounds(quader::mesh::ops::BoxDimensions{
			.min = { 1.0F, 1.0F, 1.0F },
			.max = { -1.0F, -2.0F, -3.0F },
	});
	ASSERT_TRUE(mesh);

	EXPECT_EQ(mesh.value().vertex_count(), 8U);
	EXPECT_EQ(mesh.value().face_count(), 6U);
	EXPECT_TRUE(quader::mesh::validate_mesh(mesh.value()).ok());
}

TEST(MeshOps, BoxFromBoundsRejectsDegenerateAndNonFiniteBounds) {
	auto degenerate = quader::mesh::ops::make_box_from_bounds(quader::mesh::ops::BoxDimensions{
			.min = { 0.0F, 0.0F, 0.0F },
			.max = { 0.0F, 1.0F, 1.0F },
	});
	EXPECT_FALSE(degenerate);

	auto non_finite = quader::mesh::ops::make_box_from_bounds(quader::mesh::ops::BoxDimensions{
			.min = { 0.0F, 0.0F, 0.0F },
			.max = { std::numeric_limits<float>::infinity(), 1.0F, 1.0F },
	});
	EXPECT_FALSE(non_finite);
}

TEST(MeshOps, BoxFromCornersSupportsOrientedFootprintAndHeight) {
	const quader::math::Vec3 kBase0{ 1.0F, 2.0F, 3.0F };
	const quader::math::Vec3 kAxisU{ 2.0F, 0.0F, 0.0F };
	const quader::math::Vec3 kAxisV{ 0.0F, 0.0F, 3.0F };
	const quader::math::Vec3 kHeight{ 0.0F, 4.0F, 0.0F };
	auto mesh = quader::mesh::ops::make_box_from_corners(quader::mesh::ops::BoxCorners{
			.points = {
					kBase0,
					kBase0 + kAxisU,
					kBase0 + kAxisU + kAxisV,
					kBase0 + kAxisV,
					kBase0 + kHeight,
					kBase0 + kAxisU + kHeight,
					kBase0 + kAxisU + kAxisV + kHeight,
					kBase0 + kAxisV + kHeight,
			},
	});
	ASSERT_TRUE(mesh);

	EXPECT_EQ(mesh.value().vertex_count(), 8U);
	EXPECT_EQ(mesh.value().edge_count(), 12U);
	EXPECT_EQ(mesh.value().face_count(), 6U);
	EXPECT_TRUE(quader::mesh::validate_mesh(mesh.value()).ok());
}

TEST(MeshOps, BoxFromCornersOrientsFacesOutwardForSignedFootprintDrags) {
	const quader::math::Vec3 kStart{ 1.0F, 0.0F, 1.0F };
	const quader::math::Vec3 kAxisU{ 2.0F, 0.0F, 0.0F };
	const quader::math::Vec3 kAxisV{ 0.0F, 0.0F, -3.0F };
	const quader::math::Vec3 kHeight{ 0.0F, 1.0F, 0.0F };
	const std::array<float, 2> kSigns{ -1.0F, 1.0F };

	for (const float kUSign : kSigns) {
		for (const float kVSign : kSigns) {
			SCOPED_TRACE(::testing::Message() << "u sign " << kUSign << ", v sign " << kVSign);
			const quader::math::Vec3 kU = scale(kAxisU, kUSign);
			const quader::math::Vec3 kV = scale(kAxisV, kVSign);
			auto mesh = quader::mesh::ops::make_box_from_corners(quader::mesh::ops::BoxCorners{
					.points = {
							kStart,
							kStart + kV,
							kStart + kU + kV,
							kStart + kU,
							kStart + kHeight,
							kStart + kV + kHeight,
							kStart + kU + kV + kHeight,
							kStart + kU + kHeight,
					},
			});
			ASSERT_TRUE(mesh);
			EXPECT_TRUE(quader::mesh::validate_mesh(mesh.value()).ok());
			expect_faces_oriented_outward(mesh.value());
		}
	}
}

} // namespace
