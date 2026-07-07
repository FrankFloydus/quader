/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/overlays/source_wire_visibility.hpp"

#include "math/vec3.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <vector>

namespace crimson {
namespace {

constexpr float kRayEpsilon = 0.000001F;
constexpr float kInsideOutEpsilon = 0.00001F;
constexpr float kMinimumOcclusionSlackM = 0.015F;
constexpr float kRelativeOcclusionSlack = 0.0005F;
constexpr float kDistinctHitEpsilonM = 0.001F;

struct RayTriangleHit {
	bool hit = false;
	float distance = std::numeric_limits<float>::infinity();
};

[[nodiscard]] quader::math::Vec3 scale(quader::math::Vec3 value, float factor) noexcept {
	return quader::math::Vec3{ value.x * factor, value.y * factor, value.z * factor };
}

[[nodiscard]] quader::math::Vec3 normalized_or(
		quader::math::Vec3 value,
		quader::math::Vec3 fallback) noexcept {
	const quader::math::Vec3 kNormalized = quader::math::normalized(value, kRayEpsilon);
	return quader::math::length_squared(kNormalized) <= kRayEpsilon * kRayEpsilon ? fallback : kNormalized;
}

[[nodiscard]] quader::math::Vec3 triangle_centroid(const TriangleOverlayPrimitive &triangle) noexcept {
	return (triangle.a + triangle.b + triangle.c) / 3.0F;
}

[[nodiscard]] quader::math::Vec3 triangle_normal(const TriangleOverlayPrimitive &triangle) noexcept {
	return quader::math::cross(triangle.b - triangle.a, triangle.c - triangle.a);
}

[[nodiscard]] RayTriangleHit intersect_triangle(
		quader::math::Vec3 origin,
		quader::math::Vec3 direction,
		const TriangleOverlayPrimitive &triangle) noexcept {
	const quader::math::Vec3 kEdge1 = triangle.b - triangle.a;
	const quader::math::Vec3 kEdge2 = triangle.c - triangle.a;
	const quader::math::Vec3 kP = quader::math::cross(direction, kEdge2);
	const float kDet = quader::math::dot(kEdge1, kP);
	if (std::abs(kDet) <= kRayEpsilon) {
		return {};
	}

	const float kInvDet = 1.0F / kDet;
	const quader::math::Vec3 kT = origin - triangle.a;
	const float kU = quader::math::dot(kT, kP) * kInvDet;
	if (kU < -kRayEpsilon || kU > 1.0F + kRayEpsilon) {
		return {};
	}

	const quader::math::Vec3 kQ = quader::math::cross(kT, kEdge1);
	const float kV = quader::math::dot(direction, kQ) * kInvDet;
	if (kV < -kRayEpsilon || kU + kV > 1.0F + kRayEpsilon) {
		return {};
	}

	const float kDistance = quader::math::dot(kEdge2, kQ) * kInvDet;
	if (kDistance <= kRayEpsilon) {
		return {};
	}

	return RayTriangleHit{
		.hit = true,
		.distance = kDistance,
	};
}

[[nodiscard]] quader::math::Vec3 camera_forward(const RenderCamera &camera) noexcept {
	return normalized_or(camera.forward, normalized_or(camera.target - camera.eye, { 0.0F, 0.0F, -1.0F }));
}

} // namespace

SourceWireDepthStampVisibilityFilter::SourceWireDepthStampVisibilityFilter(
		std::span<const SourceWireDepthStampMetadata> stamps) {
	for (const SourceWireDepthStampMetadata &stamp : stamps) {
		const RenderObjectId kObjectId = stamp.element.object_id != 0
				? stamp.element.object_id
				: stamp.triangle.element.object_id;
		if (kObjectId == 0) {
			continue;
		}

		auto set = std::find_if(sets_.begin(), sets_.end(), [&](const StampSet &candidate) {
			return candidate.view_index == stamp.view_index && candidate.object_id == kObjectId;
		});
		if (set == sets_.end()) {
			sets_.push_back(StampSet{
					.view_index = stamp.view_index,
					.object_id = kObjectId,
			});
			set = std::prev(sets_.end());
		}

		set->triangles.push_back(StampTriangle{
				.triangle = stamp.triangle,
				.centroid = triangle_centroid(stamp.triangle),
				.normal = triangle_normal(stamp.triangle),
		});
	}

	for (StampSet &set : sets_) {
		if (set.triangles.empty()) {
			continue;
		}

		quader::math::Vec3 centroid{};
		for (const StampTriangle &triangle : set.triangles) {
			centroid = centroid + triangle.centroid;
		}
		set.centroid = centroid / static_cast<float>(set.triangles.size());

		float signed_orientation = 0.0F;
		for (const StampTriangle &triangle : set.triangles) {
			signed_orientation += quader::math::dot(triangle.normal, triangle.centroid - set.centroid);
		}
		set.inside_out = signed_orientation < -kInsideOutEpsilon;
	}
}

bool SourceWireDepthStampVisibilityFilter::empty() const noexcept {
	return sets_.empty();
}

bool SourceWireDepthStampVisibilityFilter::point_visible(
		std::uint32_t view_index,
		const OverlayElementRef &element,
		quader::math::Vec3 point,
		const RenderCamera &camera) const noexcept {
	const StampSet *set = stamp_set_for(view_index, element.object_id);
	if (set == nullptr || set->triangles.empty() || set->inside_out) {
		return true;
	}

	quader::math::Vec3 origin = camera.eye;
	quader::math::Vec3 direction = normalized_or(point - origin, camera_forward(camera));
	float target_distance = quader::math::length(point - origin);
	if (camera.projection == CameraProjection::Orthographic) {
		direction = camera_forward(camera);
		target_distance = quader::math::dot(point - camera.eye, direction);
		origin = point - scale(direction, target_distance);
	}

	if (target_distance <= kRayEpsilon) {
		return true;
	}

	const float kCullDistance = target_distance - std::max(kMinimumOcclusionSlackM, target_distance * kRelativeOcclusionSlack);
	if (kCullDistance <= kRayEpsilon) {
		return true;
	}

	for (const StampTriangle &triangle : set->triangles) {
		const RayTriangleHit kHit = intersect_triangle(origin, direction, triangle.triangle);
		if (kHit.hit && kHit.distance < kCullDistance) {
			return false;
		}
	}
	return true;
}

OverlayDepthMode SourceWireDepthStampVisibilityFilter::point_depth_mode(
		std::uint32_t view_index,
		const OverlayElementRef &element,
		quader::math::Vec3 point,
		const RenderCamera &camera,
		OverlayDepthMode fallback) const noexcept {
	(void)point;
	const StampSet *set = stamp_set_for(view_index, element.object_id);
	if (set == nullptr || set->triangles.empty()) {
		return fallback;
	}
	if (set->inside_out && camera_inside(*set, camera)) {
		return OverlayDepthMode::AlwaysOnTop;
	}
	return fallback;
}

const SourceWireDepthStampVisibilityFilter::StampSet *SourceWireDepthStampVisibilityFilter::stamp_set_for(
		std::uint32_t view_index,
		RenderObjectId object_id) const noexcept {
	if (object_id == 0) {
		return nullptr;
	}
	const auto set = std::find_if(sets_.begin(), sets_.end(), [&](const StampSet &candidate) {
		return candidate.view_index == view_index && candidate.object_id == object_id;
	});
	return set != sets_.end() ? &*set : nullptr;
}

bool SourceWireDepthStampVisibilityFilter::camera_inside(
		const StampSet &set,
		const RenderCamera &camera) const noexcept {
	static constexpr std::array<quader::math::Vec3, 5> kRayDirections = {
		quader::math::Vec3{ 1.0F, 0.37F, 0.19F },
		quader::math::Vec3{ -0.41F, 1.0F, 0.23F },
		quader::math::Vec3{ 0.29F, -0.53F, 1.0F },
		quader::math::Vec3{ -0.77F, -0.31F, 0.54F },
		quader::math::Vec3{ 0.45F, 0.62F, -0.64F },
	};

	std::uint32_t odd_rays = 0;
	for (const quader::math::Vec3 kCandidateDirection : kRayDirections) {
		const quader::math::Vec3 kDirection = normalized_or(kCandidateDirection, { 1.0F, 0.0F, 0.0F });
		std::vector<float> distances;
		distances.reserve(set.triangles.size());
		for (const StampTriangle &triangle : set.triangles) {
			const RayTriangleHit kHit = intersect_triangle(camera.eye, kDirection, triangle.triangle);
			if (kHit.hit) {
				distances.push_back(kHit.distance);
			}
		}

		std::sort(distances.begin(), distances.end());
		std::uint32_t distinct_hits = 0;
		float previous = -std::numeric_limits<float>::infinity();
		for (const float distance : distances) {
			if (distance - previous <= kDistinctHitEpsilonM) {
				continue;
			}
			previous = distance;
			++distinct_hits;
		}
		if ((distinct_hits % 2U) == 1U) {
			++odd_rays;
		}
	}

	return odd_rays > kRayDirections.size() / 2U;
}

} // namespace crimson
