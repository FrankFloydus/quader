/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "mesh/ops/box_builder.hpp"

#include "geometry/normals.hpp"
#include "geometry/predicates.hpp"
#include "math/scalar.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <utility>

namespace quader::mesh::ops {
namespace {

constexpr float kMinimumBoxExtent = 0.000001F;

[[nodiscard]] bool extent_is_valid(float min_value, float max_value) noexcept {
	return quader::math::is_finite(min_value) && quader::math::is_finite(max_value) && max_value - min_value > kMinimumBoxExtent;
}

[[nodiscard]] quader::mesh::MeshError invalid_box_error(const char *message) {
	return quader::mesh::make_mesh_error(quader::mesh::MeshErrorCode::InvalidGeometry, message);
}

[[nodiscard]] quader::math::Vec3 component_min(quader::math::Vec3 left,
		quader::math::Vec3 right) noexcept {
	return quader::math::Vec3{
		std::min(left.x, right.x),
		std::min(left.y, right.y),
		std::min(left.z, right.z),
	};
}

[[nodiscard]] quader::math::Vec3 component_max(quader::math::Vec3 left,
		quader::math::Vec3 right) noexcept {
	return quader::math::Vec3{
		std::max(left.x, right.x),
		std::max(left.y, right.y),
		std::max(left.z, right.z),
	};
}

[[nodiscard]] std::array<std::array<std::size_t, 4>, 6> box_face_indices() noexcept {
	return {
		std::array<std::size_t, 4>{ 0U, 1U, 2U, 3U },
		std::array<std::size_t, 4>{ 4U, 5U, 6U, 7U },
		std::array<std::size_t, 4>{ 0U, 4U, 5U, 1U },
		std::array<std::size_t, 4>{ 1U, 5U, 6U, 2U },
		std::array<std::size_t, 4>{ 2U, 6U, 7U, 3U },
		std::array<std::size_t, 4>{ 3U, 7U, 4U, 0U },
	};
}

[[nodiscard]] quader::math::Vec3 average_points(std::span<const quader::math::Vec3> points) noexcept {
	quader::math::Vec3 average;
	for (quader::math::Vec3 point : points) {
		average = average + point;
	}
	const float kScale = points.empty() ? 0.0F : 1.0F / static_cast<float>(points.size());
	return average * kScale;
}

[[nodiscard]] std::array<quader::math::Vec3, 4> face_points(
		const BoxCorners &corners,
		const std::array<std::size_t, 4> &face) noexcept {
	return {
		corners.points[face[0]],
		corners.points[face[1]],
		corners.points[face[2]],
		corners.points[face[3]],
	};
}

[[nodiscard]] quader::foundation::Result<std::array<std::size_t, 4>, quader::mesh::MeshError>
orient_face_outward(
		const BoxCorners &corners,
		std::array<std::size_t, 4> face,
		quader::math::Vec3 box_center) {
	const std::array<quader::math::Vec3, 4> kPoints = face_points(corners, face);
	const quader::math::Vec3 kFaceCenter = average_points(kPoints);
	const quader::math::Vec3 kOutward = kFaceCenter - box_center;
	const quader::math::Vec3 kArea = quader::geometry::polygon_normal(kPoints);
	if (quader::math::length_squared(kArea) <= kMinimumBoxExtent * kMinimumBoxExtent ||
			quader::math::length_squared(kOutward) <= kMinimumBoxExtent * kMinimumBoxExtent) {
		return quader::foundation::Result<std::array<std::size_t, 4>, quader::mesh::MeshError>::failure(
				invalid_box_error("cannot create a box with a degenerate face"));
	}

	if (quader::math::dot(kArea, kOutward) < 0.0F) {
		std::reverse(face.begin(), face.end());
	}
	return quader::foundation::Result<std::array<std::size_t, 4>, quader::mesh::MeshError>::success(face);
}

[[nodiscard]] quader::foundation::Result<void, quader::mesh::MeshError> validate_corners(
		const BoxCorners &corners) {
	if (!quader::geometry::all_points_finite(corners.points)) {
		return quader::foundation::Result<void, quader::mesh::MeshError>::failure(
				quader::mesh::make_mesh_error(quader::mesh::MeshErrorCode::NonFinitePosition,
						"cannot create a box from non-finite corner positions"));
	}

	for (const auto &face : box_face_indices()) {
		const std::array<quader::math::Vec3, 4> kPoints{
			corners.points[face[0]],
			corners.points[face[1]],
			corners.points[face[2]],
			corners.points[face[3]],
		};
		if (quader::geometry::has_degenerate_area(kPoints, kMinimumBoxExtent)) {
			return quader::foundation::Result<void, quader::mesh::MeshError>::failure(
					quader::mesh::make_mesh_error(quader::mesh::MeshErrorCode::DegenerateFace,
							"cannot create a box with a degenerate face"));
		}
	}

	return quader::foundation::Result<void, quader::mesh::MeshError>::success();
}

} // namespace

quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError> make_box_from_bounds(
		BoxDimensions bounds) {
	const quader::math::Vec3 kMin = component_min(bounds.min, bounds.max);
	const quader::math::Vec3 kMax = component_max(bounds.min, bounds.max);
	if (!extent_is_valid(kMin.x, kMax.x) || !extent_is_valid(kMin.y, kMax.y) || !extent_is_valid(kMin.z, kMax.z)) {
		return quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError>::failure(
				invalid_box_error("cannot create a box with non-finite or degenerate bounds"));
	}

	return make_box_from_corners(BoxCorners{
			.points = {
					quader::math::Vec3{ kMin.x, kMin.y, kMin.z },
					quader::math::Vec3{ kMax.x, kMin.y, kMin.z },
					quader::math::Vec3{ kMax.x, kMax.y, kMin.z },
					quader::math::Vec3{ kMin.x, kMax.y, kMin.z },
					quader::math::Vec3{ kMin.x, kMin.y, kMax.z },
					quader::math::Vec3{ kMax.x, kMin.y, kMax.z },
					quader::math::Vec3{ kMax.x, kMax.y, kMax.z },
					quader::math::Vec3{ kMin.x, kMax.y, kMax.z },
			},
	});
}

quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError> make_box_from_bounds(
		quader::math::Aabb bounds) {
	return make_box_from_bounds(BoxDimensions{
			.min = bounds.min,
			.max = bounds.max,
	});
}

quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError> make_box_from_corners(
		const BoxCorners &corners) {
	auto validation = validate_corners(corners);
	if (!validation) {
		return quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError>::failure(
				std::move(validation).error());
	}

	quader::mesh::Polyhedron mesh;
	std::array<quader::mesh::VertexId, 8> vertices{};
	for (std::size_t index = 0; index < corners.points.size(); ++index) {
		vertices[index] = mesh.create_vertex(corners.points[index]);
	}

	const quader::math::Vec3 kBoxCenter = average_points(corners.points);
	for (const auto &face : box_face_indices()) {
		auto oriented_face = orient_face_outward(corners, face, kBoxCenter);
		if (!oriented_face) {
			return quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError>::failure(
					std::move(oriented_face).error());
		}

		const std::array<quader::mesh::VertexId, 4> kFaceVertices{
			vertices[oriented_face.value()[0]],
			vertices[oriented_face.value()[1]],
			vertices[oriented_face.value()[2]],
			vertices[oriented_face.value()[3]],
		};
		auto created_face = mesh.create_face(kFaceVertices);
		if (!created_face) {
			return quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError>::failure(
					std::move(created_face).error());
		}
	}

	return quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError>::success(
			std::move(mesh));
}

} // namespace quader::mesh::ops
