/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/gpu_overlay_renderer.hpp"

#include "crimson/gpu/gpu_handles.hpp"
#include "crimson/scene/render_camera_projection.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include <bgfx/bgfx.h>
#include <bx/math.h>

namespace crimson::gpu {
namespace {

struct GridVertex {
	float x;
	float y;
	float z;
};

struct OverlayPositionVertex {
	float x;
	float y;
	float z;
};

struct OverlayVec2 {
	float x = 0.0F;
	float y = 0.0F;
};

struct OverlayCameraBasis {
	quader::math::Vec3 right{ -1.0F, 0.0F, 0.0F };
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	quader::math::Vec3 view{ 0.0F, 0.0F, -1.0F };
};

struct OverlayCameraProjector {
	OverlayCameraBasis basis;
	float viewport_width_px = 1.0F;
	float viewport_height_px = 1.0F;
	float aspect_ratio = 1.0F;
	float near_plane_m = 0.0001F;
	float far_plane_m = 1.0F;
	float tan_half_fov_y = 1.0F;
	float orthographic_half_height_m = 0.5F;
	bool perspective = true;
	bool homogeneous_depth = false;
};

struct OverlayProjectedPoint {
	OverlayVec2 pixel;
	float device_z = 0.0F;
	bool valid = false;
};

struct OverlayClippedLine {
	quader::math::Vec3 start;
	quader::math::Vec3 end;
};

struct SourceWireSampleRay {
	quader::math::Vec3 origin;
	quader::math::Vec3 direction;
	float target_distance = 0.0F;
};

struct SourceWireStampSetClassification {
	bool has_stamps = false;
	bool inside_out = false;
};

const GridVertex kGridVertices[] = {
	{ -0.5F, 0.0F, -0.5F },
	{ 0.5F, 0.0F, -0.5F },
	{ 0.5F, 0.0F, 0.5F },
	{ -0.5F, 0.0F, 0.5F },
};

const std::uint16_t kGridIndices[] = {
	0,
	2,
	1,
	0,
	3,
	2,
};

constexpr float kOverlayEpsilon = 0.000001F;
constexpr float kPi = 3.14159265358979323846F;
constexpr std::uint32_t kVerticesPerQuad = 4U;
constexpr std::uint32_t kIndicesPerQuad = 6U;
constexpr std::uint32_t kMaxTransientQuads16 =
		std::numeric_limits<std::uint16_t>::max() / kVerticesPerQuad;
constexpr float kOverlayLineDepthBiasUnits = 1.0F;
constexpr float kOverlayPointDepthBiasUnits = 1.5F;
constexpr float kOverlayDeviceDepthBiasPerNearPlane = 0.0025F;
constexpr float kOverlayLineFeatherPixels = 1.0F;
constexpr float kSourceWireOcclusionBaseTolerance = 0.015F;
constexpr float kSourceWireOcclusionDistanceToleranceScale = 0.0005F;
constexpr std::array<float, 17> kSourceWireVisibilitySamples = {
	0.0F,
	0.0625F,
	0.125F,
	0.1875F,
	0.25F,
	0.3125F,
	0.375F,
	0.4375F,
	0.5F,
	0.5625F,
	0.625F,
	0.6875F,
	0.75F,
	0.8125F,
	0.875F,
	0.9375F,
	1.0F,
};

void push_overlay_resource_error(RendererStatus &status, std::string resource_name) {
	status.diagnostics.push_back(RendererDiagnostic{
			.severity = RendererDiagnosticSeverity::Error,
			.code = RendererDiagnosticCode::ResourceCreationFailed,
			.message = "Crimson failed to create overlay GPU resources.",
			.subsystem = "gpu.overlay",
			.resource_name = std::move(resource_name),
	});
}

[[nodiscard]] quader::math::Vec3 normalized_or(
		quader::math::Vec3 value,
		quader::math::Vec3 fallback) noexcept {
	const quader::math::Vec3 kNormalized = quader::math::normalized(value);
	return quader::math::length_squared(kNormalized) <= kOverlayEpsilon * kOverlayEpsilon
			? fallback
			: kNormalized;
}

[[nodiscard]] float degrees_to_radians(float degrees) noexcept {
	return degrees * (kPi / 180.0F);
}

[[nodiscard]] OverlayCameraBasis camera_basis(const RenderCamera &camera) noexcept {
	const quader::math::Vec3 kFallbackForward =
			normalized_or(camera.forward, { 0.0F, 0.0F, -1.0F });
	const quader::math::Vec3 kView = normalized_or(camera.target - camera.eye, kFallbackForward);
	const quader::math::Vec3 kRight = normalized_or(
			quader::math::cross(camera.up, kView),
			{ -1.0F, 0.0F, 0.0F });
	return OverlayCameraBasis{
		.right = kRight,
		.up = normalized_or(quader::math::cross(kView, kRight), { 0.0F, 1.0F, 0.0F }),
		.view = kView,
	};
}

[[nodiscard]] bool valid_overlay_object_id(RenderObjectId object_id) noexcept {
	return object_id != 0U;
}

[[nodiscard]] bool same_known_face(
		const OverlayElementRef &left,
		const OverlayElementRef &right) noexcept {
	return left.face_index != kInvalidOverlayElementIndex &&
			right.face_index != kInvalidOverlayElementIndex &&
			left.face_index == right.face_index;
}

[[nodiscard]] RenderObjectId stamp_object_id(const SourceWireDepthStampMetadata &stamp) noexcept {
	return valid_overlay_object_id(stamp.triangle.element.object_id)
			? stamp.triangle.element.object_id
			: stamp.element.object_id;
}

[[nodiscard]] const OverlayElementRef &stamp_element_ref(
		const SourceWireDepthStampMetadata &stamp) noexcept {
	return valid_overlay_object_id(stamp.element.object_id) ? stamp.element : stamp.triangle.element;
}

[[nodiscard]] bool element_references_face(
		const OverlayElementRef &element,
		std::uint32_t face_index) noexcept {
	return face_index != kInvalidOverlayElementIndex &&
			(element.face_index == face_index ||
					element.incident_face_index0 == face_index ||
					element.incident_face_index1 == face_index);
}

[[nodiscard]] bool stamp_owns_segment(
		const SourceWireDepthStampMetadata &stamp,
		const LineOverlaySegment &segment) noexcept {
	return element_references_face(segment.element, stamp_element_ref(stamp).face_index);
}

[[nodiscard]] bool stamp_matches_object(
		const SourceWireDepthStampMetadata &stamp,
		RenderObjectId object_id,
		std::uint32_t view_index) noexcept {
	return stamp.view_index == view_index &&
			stamp.source_kind == OverlaySourceKind::SourceWire &&
			stamp_object_id(stamp) == object_id &&
			valid_overlay_object_id(object_id);
}

[[nodiscard]] bool has_matching_object_stamps(
		std::span<const SourceWireDepthStampMetadata> stamps,
		RenderObjectId object_id,
		std::uint32_t view_index) noexcept {
	return std::any_of(stamps.begin(), stamps.end(), [object_id, view_index](const SourceWireDepthStampMetadata &stamp) {
		return stamp_matches_object(stamp, object_id, view_index);
	});
}

[[nodiscard]] quader::math::Vec3 triangle_centroid(const TriangleOverlayPrimitive &triangle) noexcept {
	return (triangle.a + triangle.b + triangle.c) / 3.0F;
}

[[nodiscard]] quader::math::Vec3 triangle_normal(const TriangleOverlayPrimitive &triangle) noexcept {
	return quader::math::cross(triangle.b - triangle.a, triangle.c - triangle.a);
}

[[nodiscard]] SourceWireStampSetClassification classify_source_wire_stamp_set(
		std::span<const SourceWireDepthStampMetadata> stamps,
		RenderObjectId object_id,
		std::uint32_t view_index) noexcept {
	SourceWireStampSetClassification classification;
	quader::math::Vec3 stamp_set_centroid;
	std::uint32_t stamp_vertex_count = 0U;
	for (const SourceWireDepthStampMetadata &stamp : stamps) {
		if (!stamp_matches_object(stamp, object_id, view_index)) {
			continue;
		}
		stamp_set_centroid = stamp_set_centroid + stamp.triangle.a + stamp.triangle.b + stamp.triangle.c;
		stamp_vertex_count += 3U;
	}
	if (stamp_vertex_count == 0U) {
		return classification;
	}

	classification.has_stamps = true;
	stamp_set_centroid = stamp_set_centroid / static_cast<float>(stamp_vertex_count);
	float winding_score = 0.0F;
	float negative_area = 0.0F;
	float positive_area = 0.0F;
	std::uint32_t valid_stamp_count = 0U;
	for (const SourceWireDepthStampMetadata &stamp : stamps) {
		if (!stamp_matches_object(stamp, object_id, view_index)) {
			continue;
		}
		const quader::math::Vec3 kNormal = triangle_normal(stamp.triangle);
		if (quader::math::length_squared(kNormal) <= kOverlayEpsilon * kOverlayEpsilon) {
			continue;
		}

		++valid_stamp_count;
		const float kTriangleScore = quader::math::dot(
				kNormal,
				triangle_centroid(stamp.triangle) - stamp_set_centroid);
		const float kTriangleAreaWeight = std::sqrt(quader::math::length_squared(kNormal));
		winding_score += kTriangleScore;
		if (kTriangleScore < -kOverlayEpsilon) {
			negative_area += kTriangleAreaWeight;
		} else if (kTriangleScore > kOverlayEpsilon) {
			positive_area += kTriangleAreaWeight;
		}
	}

	if (valid_stamp_count == 0U) {
		return classification;
	}

	const bool kInsideOutByScore = winding_score < -kOverlayEpsilon;
	const bool kInsideOutByArea = negative_area > kOverlayEpsilon &&
			negative_area > positive_area * 1.1F;
	classification.inside_out = kInsideOutByScore || kInsideOutByArea;
	return classification;
}

[[nodiscard]] bool source_wire_stamp_set_keeps_topology(
		std::span<const SourceWireDepthStampMetadata> stamps,
		RenderObjectId object_id,
		std::uint32_t view_index) noexcept {
	const SourceWireStampSetClassification kClassification =
			classify_source_wire_stamp_set(stamps, object_id, view_index);
	return kClassification.inside_out;
}

[[nodiscard]] std::optional<SourceWireSampleRay> source_wire_sample_ray(
		const RenderCamera &camera,
		const OverlayCameraBasis &basis,
		quader::math::Vec3 sample) noexcept {
	if (camera.projection == CameraProjection::Orthographic) {
		const float kTargetDistance = quader::math::dot(sample - camera.eye, basis.view);
		if (kTargetDistance <= kOverlayEpsilon) {
			return std::nullopt;
		}
		return SourceWireSampleRay{
			.origin = sample - basis.view * kTargetDistance,
			.direction = basis.view,
			.target_distance = kTargetDistance,
		};
	}

	const quader::math::Vec3 kToSample = sample - camera.eye;
	const float kTargetDistance = quader::math::length(kToSample);
	if (kTargetDistance <= kOverlayEpsilon) {
		return std::nullopt;
	}
	return SourceWireSampleRay{
		.origin = camera.eye,
		.direction = kToSample / kTargetDistance,
		.target_distance = kTargetDistance,
	};
}

[[nodiscard]] std::optional<float> ray_triangle_intersection_distance(
		const SourceWireSampleRay &ray,
		const TriangleOverlayPrimitive &triangle) noexcept {
	const quader::math::Vec3 kEdge1 = triangle.b - triangle.a;
	const quader::math::Vec3 kEdge2 = triangle.c - triangle.a;
	const quader::math::Vec3 kP = quader::math::cross(ray.direction, kEdge2);
	const float kDeterminant = quader::math::dot(kEdge1, kP);
	if (std::abs(kDeterminant) <= kOverlayEpsilon) {
		return std::nullopt;
	}

	const float kInverseDeterminant = 1.0F / kDeterminant;
	const quader::math::Vec3 kT = ray.origin - triangle.a;
	const float kU = quader::math::dot(kT, kP) * kInverseDeterminant;
	if (kU < -kOverlayEpsilon || kU > 1.0F + kOverlayEpsilon) {
		return std::nullopt;
	}

	const quader::math::Vec3 kQ = quader::math::cross(kT, kEdge1);
	const float kV = quader::math::dot(ray.direction, kQ) * kInverseDeterminant;
	if (kV < -kOverlayEpsilon || kU + kV > 1.0F + kOverlayEpsilon) {
		return std::nullopt;
	}

	const float kDistance = quader::math::dot(kEdge2, kQ) * kInverseDeterminant;
	if (kDistance <= kOverlayEpsilon) {
		return std::nullopt;
	}
	return kDistance;
}

[[nodiscard]] bool source_wire_sample_visible(
		const RenderCamera &camera,
		const OverlayCameraBasis &basis,
		std::span<const SourceWireDepthStampMetadata> stamps,
		RenderObjectId object_id,
		const OverlayElementRef &element,
		const LineOverlaySegment *segment,
		quader::math::Vec3 sample,
		std::uint32_t view_index) noexcept {
	const std::optional<SourceWireSampleRay> kRay = source_wire_sample_ray(camera, basis, sample);
	if (!kRay.has_value()) {
		return true;
	}

	const float kTolerance = std::max(
			kSourceWireOcclusionBaseTolerance,
			kRay->target_distance * kSourceWireOcclusionDistanceToleranceScale);
	const float kOccludedDistance = kRay->target_distance - kTolerance;
	for (const SourceWireDepthStampMetadata &stamp : stamps) {
		if (!stamp_matches_object(stamp, object_id, view_index) ||
				same_known_face(stamp_element_ref(stamp), element) ||
				(segment != nullptr && stamp_owns_segment(stamp, *segment))) {
			continue;
		}

		const std::optional<float> kHitDistance = ray_triangle_intersection_distance(*kRay, stamp.triangle);
		if (kHitDistance.has_value() && *kHitDistance < kOccludedDistance) {
			return false;
		}
	}
	return true;
}

[[nodiscard]] bool source_wire_segment_visible(
		const RenderView &view,
		const OverlayCameraBasis &basis,
		std::span<const SourceWireDepthStampMetadata> stamps,
		const LineOverlaySegment &segment) noexcept {
	if (!has_matching_object_stamps(stamps, segment.element.object_id, view.view_index) ||
			source_wire_stamp_set_keeps_topology(stamps, segment.element.object_id, view.view_index)) {
		return true;
	}

	std::uint32_t visible_samples = 0U;
	for (const float kT : kSourceWireVisibilitySamples) {
		const quader::math::Vec3 kSample = segment.start + (segment.end - segment.start) * kT;
		if (source_wire_sample_visible(
					view.camera,
					basis,
					stamps,
					segment.element.object_id,
					segment.element,
					&segment,
					kSample,
					view.view_index)) {
			++visible_samples;
		}
	}
	return visible_samples > 0U;
}

[[nodiscard]] bool source_wire_point_visible(
		const RenderView &view,
		const OverlayCameraBasis &basis,
		std::span<const SourceWireDepthStampMetadata> stamps,
		const PointOverlayPrimitive &point) noexcept {
	if (!has_matching_object_stamps(stamps, point.element.object_id, view.view_index) ||
			source_wire_stamp_set_keeps_topology(stamps, point.element.object_id, view.view_index)) {
		return true;
	}

	return source_wire_sample_visible(
			view.camera,
			basis,
			stamps,
			point.element.object_id,
			point.element,
			nullptr,
			point.position,
			view.view_index);
}

[[nodiscard]] bool line_role_uses_depth_stamp_visibility(
		OverlaySemanticRole role,
		OverlaySourceKind source_kind) noexcept {
	return role == OverlaySemanticRole::SourceWire &&
			source_kind == OverlaySourceKind::SourceWire;
}

[[nodiscard]] bool point_role_uses_depth_stamp_visibility(
		OverlaySemanticRole role,
		OverlaySourceKind source_kind) noexcept {
	return role == OverlaySemanticRole::SourceVertex &&
			source_kind == OverlaySourceKind::SourceWire;
}

[[nodiscard]] float sanitized_near_plane(const RenderCamera &camera) noexcept {
	return std::max(0.0001F, camera.near_plane_m);
}

[[nodiscard]] float sanitized_far_plane(const RenderCamera &camera) noexcept {
	const float kNear = sanitized_near_plane(camera);
	return std::max(kNear + 0.001F, camera.far_plane_m);
}

[[nodiscard]] OverlayCameraProjector make_overlay_projector(
		const RenderView &view,
		bool homogeneous_depth) noexcept {
	const float kWidth = std::max(1.0F, static_cast<float>(view.rect.width));
	const float kHeight = std::max(1.0F, static_cast<float>(view.rect.height));
	const float kFovRadians = degrees_to_radians(std::clamp(view.camera.vertical_fov_degrees, 1.0F, 170.0F));
	const float kOrthographicHeight = std::max(0.0001F, view.camera.orthographic_height_m);
	return OverlayCameraProjector{
		.basis = camera_basis(view.camera),
		.viewport_width_px = kWidth,
		.viewport_height_px = kHeight,
		.aspect_ratio = kWidth / kHeight,
		.near_plane_m = sanitized_near_plane(view.camera),
		.far_plane_m = sanitized_far_plane(view.camera),
		.tan_half_fov_y = std::tan(kFovRadians * 0.5F),
		.orthographic_half_height_m = kOrthographicHeight * 0.5F,
		.perspective = view.camera.projection == CameraProjection::Perspective,
		.homogeneous_depth = homogeneous_depth,
	};
}

[[nodiscard]] float forward_depth(
		const RenderCamera &camera,
		const OverlayCameraBasis &basis,
		quader::math::Vec3 position) noexcept {
	return quader::math::dot(position - camera.eye, basis.view);
}

[[nodiscard]] float device_z_from_depth(
		const OverlayCameraProjector &projector,
		float depth) noexcept {
	const float kNear = projector.near_plane_m;
	const float kFar = projector.far_plane_m;
	const float kDiff = std::max(0.000001F, kFar - kNear);
	if (!projector.perspective) {
		if (projector.homogeneous_depth) {
			return (2.0F * depth - kNear - kFar) / kDiff;
		}
		return (depth - kNear) / kDiff;
	}

	const float kSafeDepth = std::max(depth, kNear);
	const float kDepthScale = projector.homogeneous_depth
			? (kFar + kNear) / kDiff
			: kFar / kDiff;
	const float kDepthBias = projector.homogeneous_depth
			? (2.0F * kFar * kNear) / kDiff
			: (kFar * kNear) / kDiff;
	return kDepthScale - kDepthBias / kSafeDepth;
}

[[nodiscard]] float depth_from_device_z(
		const OverlayCameraProjector &projector,
		float device_z) noexcept {
	const float kNear = projector.near_plane_m;
	const float kFar = projector.far_plane_m;
	const float kDiff = std::max(0.000001F, kFar - kNear);
	if (!projector.perspective) {
		if (projector.homogeneous_depth) {
			return (device_z * kDiff + kNear + kFar) * 0.5F;
		}
		return device_z * kDiff + kNear;
	}

	const float kDepthScale = projector.homogeneous_depth
			? (kFar + kNear) / kDiff
			: kFar / kDiff;
	const float kDepthBias = projector.homogeneous_depth
			? (2.0F * kFar * kNear) / kDiff
			: (kFar * kNear) / kDiff;
	const float kDenominator = std::max(0.000001F, kDepthScale - device_z);
	return kDepthBias / kDenominator;
}

void apply_normal_depth_bias(
		const OverlayCameraProjector &projector,
		OverlayProjectedPoint &point,
		float depth_bias_units) noexcept {
	if (!point.valid || depth_bias_units <= 0.0F) {
		return;
	}

	const float kDeviceOffset =
			depth_bias_units * projector.near_plane_m * kOverlayDeviceDepthBiasPerNearPlane;
	const float kNearDeviceZ = projector.homogeneous_depth ? -1.0F : 0.0F;
	point.device_z = std::max(kNearDeviceZ, point.device_z - kDeviceOffset);
}

[[nodiscard]] std::optional<OverlayProjectedPoint> project_overlay_point(
		const OverlayCameraProjector &projector,
		const RenderCamera &camera,
		quader::math::Vec3 position) noexcept {
	const float kDepth = forward_depth(camera, projector.basis, position);
	if (kDepth < projector.near_plane_m) {
		return std::nullopt;
	}

	const quader::math::Vec3 kRelative = position - camera.eye;
	const float kViewX = quader::math::dot(kRelative, projector.basis.right);
	const float kViewY = quader::math::dot(kRelative, projector.basis.up);
	float device_x = 0.0F;
	float device_y = 0.0F;
	if (projector.perspective) {
		const float kTanY = std::max(0.000001F, projector.tan_half_fov_y);
		device_x = kViewX / (kDepth * kTanY * projector.aspect_ratio);
		device_y = kViewY / (kDepth * kTanY);
	} else {
		const float kHalfHeight = std::max(0.000001F, projector.orthographic_half_height_m);
		device_x = kViewX / (kHalfHeight * projector.aspect_ratio);
		device_y = kViewY / kHalfHeight;
	}

	if (!std::isfinite(device_x) || !std::isfinite(device_y)) {
		return std::nullopt;
	}

	return OverlayProjectedPoint{
		.pixel = {
				(device_x * 0.5F + 0.5F) * projector.viewport_width_px,
				(0.5F - device_y * 0.5F) * projector.viewport_height_px,
		},
		.device_z = device_z_from_depth(projector, kDepth),
		.valid = true,
	};
}

[[nodiscard]] quader::math::Vec3 unproject_overlay_point(
		const OverlayCameraProjector &projector,
		const RenderCamera &camera,
		OverlayVec2 pixel,
		float device_z) noexcept {
	const float kDeviceX = (pixel.x / projector.viewport_width_px) * 2.0F - 1.0F;
	const float kDeviceY = 1.0F - (pixel.y / projector.viewport_height_px) * 2.0F;
	const float kDepth = std::clamp(
			depth_from_device_z(projector, device_z),
			projector.near_plane_m,
			projector.far_plane_m);
	float view_x = 0.0F;
	float view_y = 0.0F;
	if (projector.perspective) {
		view_x = kDeviceX * kDepth * projector.tan_half_fov_y * projector.aspect_ratio;
		view_y = kDeviceY * kDepth * projector.tan_half_fov_y;
	} else {
		view_x = kDeviceX * projector.orthographic_half_height_m * projector.aspect_ratio;
		view_y = kDeviceY * projector.orthographic_half_height_m;
	}

	return camera.eye +
			projector.basis.right * view_x +
			projector.basis.up * view_y +
			projector.basis.view * kDepth;
}

[[nodiscard]] std::optional<OverlayClippedLine> clip_line_to_near_plane(
		const OverlayCameraProjector &projector,
		const RenderCamera &camera,
		quader::math::Vec3 start,
		quader::math::Vec3 end) noexcept {
	const float kStartDepth = forward_depth(camera, projector.basis, start);
	const float kEndDepth = forward_depth(camera, projector.basis, end);
	if (kStartDepth < projector.near_plane_m && kEndDepth < projector.near_plane_m) {
		return std::nullopt;
	}

	if (kStartDepth < projector.near_plane_m || kEndDepth < projector.near_plane_m) {
		const float kDenominator = kEndDepth - kStartDepth;
		if (std::abs(kDenominator) <= kOverlayEpsilon) {
			return std::nullopt;
		}
		const float kT = std::clamp((projector.near_plane_m - kStartDepth) / kDenominator, 0.0F, 1.0F);
		const quader::math::Vec3 kClipped = start + (end - start) * kT;
		if (kStartDepth < projector.near_plane_m) {
			start = kClipped;
		} else {
			end = kClipped;
		}
	}

	return OverlayClippedLine{ .start = start, .end = end };
}

[[nodiscard]] OverlayVec2 operator+(OverlayVec2 left, OverlayVec2 right) noexcept {
	return { left.x + right.x, left.y + right.y };
}

[[nodiscard]] OverlayVec2 operator-(OverlayVec2 left, OverlayVec2 right) noexcept {
	return { left.x - right.x, left.y - right.y };
}

[[nodiscard]] OverlayVec2 operator*(OverlayVec2 value, float scale) noexcept {
	return { value.x * scale, value.y * scale };
}

[[nodiscard]] float length(OverlayVec2 value) noexcept {
	return std::sqrt(value.x * value.x + value.y * value.y);
}

[[nodiscard]] std::uint32_t available_quad_count(
		std::size_t requested_quad_count,
		const bgfx::VertexLayout &layout) noexcept {
	const std::uint32_t kRequested = static_cast<std::uint32_t>(std::min<std::size_t>(
			requested_quad_count,
			kMaxTransientQuads16));
	if (kRequested == 0U) {
		return 0;
	}

	const std::uint32_t kRequestedVertices = kRequested * kVerticesPerQuad;
	const std::uint32_t kRequestedIndices = kRequested * kIndicesPerQuad;
	const std::uint32_t kAvailableVertices =
			bgfx::getAvailTransientVertexBuffer(kRequestedVertices, layout);
	const std::uint32_t kAvailableIndices =
			bgfx::getAvailTransientIndexBuffer(kRequestedIndices);
	return std::min({
			kRequested,
			kAvailableVertices / kVerticesPerQuad,
			kAvailableIndices / kIndicesPerQuad,
	});
}

void append_position_vertex(
		OverlayPositionVertex *vertices,
		std::uint32_t &vertex_index,
		quader::math::Vec3 position) noexcept {
	vertices[vertex_index++] = OverlayPositionVertex{ position.x, position.y, position.z };
}

void append_quad(
		OverlayPositionVertex *vertices,
		std::uint16_t *indices,
		std::uint32_t &vertex_index,
		std::uint32_t &index_index,
		quader::math::Vec3 a,
		quader::math::Vec3 b,
		quader::math::Vec3 c,
		quader::math::Vec3 d) noexcept {
	const auto kBase = static_cast<std::uint16_t>(vertex_index);
	append_position_vertex(vertices, vertex_index, a);
	append_position_vertex(vertices, vertex_index, b);
	append_position_vertex(vertices, vertex_index, c);
	append_position_vertex(vertices, vertex_index, d);

	indices[index_index++] = kBase;
	indices[index_index++] = static_cast<std::uint16_t>(kBase + 1U);
	indices[index_index++] = static_cast<std::uint16_t>(kBase + 2U);
	indices[index_index++] = kBase;
	indices[index_index++] = static_cast<std::uint16_t>(kBase + 2U);
	indices[index_index++] = static_cast<std::uint16_t>(kBase + 3U);
}

[[nodiscard]] std::uint64_t depth_test_state_for(
		const PreparedOverlayRenderState &render_state) noexcept {
	if (!render_state.depth_test_enabled) {
		return 0;
	}

	if (render_state.equal_depth_test_enabled) {
		return BGFX_STATE_DEPTH_TEST_EQUAL;
	}

	return BGFX_STATE_DEPTH_TEST_LEQUAL;
}

[[nodiscard]] std::uint64_t state_for_overlay_render_state(
		const PreparedOverlayRenderState &render_state,
		std::uint64_t primitive_state = 0) noexcept {
	std::uint64_t state = primitive_state | BGFX_STATE_MSAA;
	if (render_state.color_write_enabled) {
		state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
		if (render_state.pass_kind != PreparedOverlayPassKind::DepthStamp) {
			state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
		}
	}
	if (render_state.depth_write_enabled) {
		state |= BGFX_STATE_WRITE_Z;
	}
	state |= depth_test_state_for(render_state);
	return state;
}

[[nodiscard]] std::uint32_t clamped_vertex_count(std::size_t count) noexcept {
	return static_cast<std::uint32_t>(std::min<std::size_t>(
			count,
			std::numeric_limits<std::uint32_t>::max()));
}

} // namespace

std::optional<OverlayScreenSpaceQuad> make_overlay_line_screen_space_quad(
		const RenderView &view,
		const LineOverlaySegment &segment,
		float width_px,
		float depth_bias_units,
		bool homogeneous_depth) noexcept {
	const OverlayCameraProjector kProjector = make_overlay_projector(view, homogeneous_depth);
	const std::optional<OverlayClippedLine> kClipped = clip_line_to_near_plane(
			kProjector,
			view.camera,
			segment.start,
			segment.end);
	if (!kClipped.has_value()) {
		return std::nullopt;
	}

	std::optional<OverlayProjectedPoint> projected_start = project_overlay_point(
			kProjector,
			view.camera,
			kClipped->start);
	std::optional<OverlayProjectedPoint> projected_end = project_overlay_point(
			kProjector,
			view.camera,
			kClipped->end);
	if (!projected_start.has_value() || !projected_end.has_value()) {
		return std::nullopt;
	}

	apply_normal_depth_bias(kProjector, *projected_start, depth_bias_units);
	apply_normal_depth_bias(kProjector, *projected_end, depth_bias_units);

	OverlayVec2 direction = projected_end->pixel - projected_start->pixel;
	const float kPixelLength = length(direction);
	if (kPixelLength <= kOverlayEpsilon) {
		direction = { 1.0F, 0.0F };
	} else {
		direction = direction * (1.0F / kPixelLength);
	}
	const OverlayVec2 kNormal{ -direction.y, direction.x };
	const float kSideDistance = (std::max(width_px, 1.0F) + kOverlayLineFeatherPixels) * 0.5F;
	const OverlayVec2 kStartCap = direction * -kSideDistance;
	const OverlayVec2 kEndCap = direction * kSideDistance;

	return OverlayScreenSpaceQuad{
		.corners = {
				unproject_overlay_point(kProjector,
						view.camera,
						projected_start->pixel + kStartCap + kNormal * -kSideDistance,
						projected_start->device_z),
				unproject_overlay_point(kProjector,
						view.camera,
						projected_end->pixel + kEndCap + kNormal * -kSideDistance,
						projected_end->device_z),
				unproject_overlay_point(kProjector,
						view.camera,
						projected_end->pixel + kEndCap + kNormal * kSideDistance,
						projected_end->device_z),
				unproject_overlay_point(kProjector,
						view.camera,
						projected_start->pixel + kStartCap + kNormal * kSideDistance,
						projected_start->device_z),
		},
	};
}

std::optional<OverlayScreenSpaceQuad> make_overlay_point_screen_space_quad(
		const RenderView &view,
		quader::math::Vec3 position,
		float size_px,
		float depth_bias_units,
		bool homogeneous_depth) noexcept {
	const OverlayCameraProjector kProjector = make_overlay_projector(view, homogeneous_depth);
	std::optional<OverlayProjectedPoint> projected = project_overlay_point(kProjector, view.camera, position);
	if (!projected.has_value()) {
		return std::nullopt;
	}

	apply_normal_depth_bias(kProjector, *projected, depth_bias_units);

	const float kHalfSize = std::max(size_px, 1.0F) * 0.5F;
	return OverlayScreenSpaceQuad{
		.corners = {
				unproject_overlay_point(kProjector,
						view.camera,
						projected->pixel + OverlayVec2{ -kHalfSize, -kHalfSize },
						projected->device_z),
				unproject_overlay_point(kProjector,
						view.camera,
						projected->pixel + OverlayVec2{ kHalfSize, -kHalfSize },
						projected->device_z),
				unproject_overlay_point(kProjector,
						view.camera,
						projected->pixel + OverlayVec2{ kHalfSize, kHalfSize },
						projected->device_z),
				unproject_overlay_point(kProjector,
						view.camera,
						projected->pixel + OverlayVec2{ -kHalfSize, kHalfSize },
						projected->device_z),
		},
	};
}

PreparedLineOverlayCommand make_source_wire_visibility_filtered_line_command(
		const RenderView &view,
		const PreparedLineOverlayCommand &lines,
		std::span<const SourceWireDepthStampMetadata> source_wire_depth_stamps) {
	PreparedLineOverlayCommand filtered = lines;
	if (!line_role_uses_depth_stamp_visibility(lines.semantic_role, lines.source_kind) ||
			lines.segments.empty() ||
			source_wire_depth_stamps.empty()) {
		return filtered;
	}

	const OverlayCameraBasis kBasis = camera_basis(view.camera);
	filtered.segments.clear();
	filtered.segments.reserve(lines.segments.size());
	for (const LineOverlaySegment &segment : lines.segments) {
		if (source_wire_segment_visible(view, kBasis, source_wire_depth_stamps, segment)) {
			filtered.segments.push_back(segment);
		}
	}
	filtered.command.payload_count = static_cast<std::uint32_t>(std::min<std::size_t>(
			filtered.segments.size(),
			std::numeric_limits<std::uint32_t>::max()));
	return filtered;
}

PreparedPointOverlayCommand make_source_wire_visibility_filtered_point_command(
		const RenderView &view,
		const PreparedPointOverlayCommand &points,
		std::span<const SourceWireDepthStampMetadata> source_wire_depth_stamps) {
	PreparedPointOverlayCommand filtered = points;
	if (!point_role_uses_depth_stamp_visibility(points.semantic_role, points.source_kind) ||
			points.points.empty() ||
			source_wire_depth_stamps.empty()) {
		return filtered;
	}

	const OverlayCameraBasis kBasis = camera_basis(view.camera);
	filtered.points.clear();
	filtered.points.reserve(points.points.size());
	for (const PointOverlayPrimitive &point : points.points) {
		if (source_wire_point_visible(view, kBasis, source_wire_depth_stamps, point)) {
			filtered.points.push_back(point);
		}
	}
	filtered.command.payload_count = static_cast<std::uint32_t>(std::min<std::size_t>(
			filtered.points.size(),
			std::numeric_limits<std::uint32_t>::max()));
	return filtered;
}

struct GpuOverlayRenderer::Impl {
	bgfx::VertexLayout grid_vertex_layout{};
	bgfx::VertexLayout line_vertex_layout{};
	UniqueVertexBufferHandle grid_vertex_buffer;
	UniqueIndexBufferHandle grid_index_buffer;
	UniqueUniformHandle line_color_uniform;
	UniqueUniformHandle grid_plane_size_uniform;
	UniqueUniformHandle grid_color_uniform;
	UniqueUniformHandle major_grid_color_uniform;
	UniqueUniformHandle origin_u_color_uniform;
	UniqueUniformHandle origin_v_color_uniform;
	UniqueUniformHandle camera_position_uniform;
	UniqueUniformHandle grid_u_axis_uniform;
	UniqueUniformHandle grid_v_axis_uniform;
	UniqueUniformHandle grid_params0_uniform;
	UniqueUniformHandle grid_params1_uniform;
	UniqueUniformHandle grid_params2_uniform;
	bool ready = false;
};

GpuOverlayRenderer::GpuOverlayRenderer() : impl_(std::make_unique<Impl>()) {
}

GpuOverlayRenderer::~GpuOverlayRenderer() = default;

bool GpuOverlayRenderer::initialize(RendererStatus &status) {
	shutdown();

	impl_->grid_vertex_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.end();
	impl_->line_vertex_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.end();
	impl_->grid_vertex_buffer.reset(bgfx::createVertexBuffer(
			bgfx::makeRef(kGridVertices, sizeof(kGridVertices)),
			impl_->grid_vertex_layout));
	impl_->grid_index_buffer.reset(bgfx::createIndexBuffer(bgfx::makeRef(kGridIndices, sizeof(kGridIndices))));
	impl_->line_color_uniform.reset(bgfx::createUniform("u_lineColor", bgfx::UniformType::Vec4));
	impl_->grid_plane_size_uniform.reset(bgfx::createUniform("u_gridPlaneSize", bgfx::UniformType::Vec4));
	impl_->grid_color_uniform.reset(bgfx::createUniform("u_gridColor", bgfx::UniformType::Vec4));
	impl_->major_grid_color_uniform.reset(bgfx::createUniform("u_majorGridColor", bgfx::UniformType::Vec4));
	impl_->origin_u_color_uniform.reset(bgfx::createUniform("u_originUColor", bgfx::UniformType::Vec4));
	impl_->origin_v_color_uniform.reset(bgfx::createUniform("u_originVColor", bgfx::UniformType::Vec4));
	impl_->camera_position_uniform.reset(bgfx::createUniform("u_cameraPosition", bgfx::UniformType::Vec4));
	impl_->grid_u_axis_uniform.reset(bgfx::createUniform("u_gridUAxis", bgfx::UniformType::Vec4));
	impl_->grid_v_axis_uniform.reset(bgfx::createUniform("u_gridVAxis", bgfx::UniformType::Vec4));
	impl_->grid_params0_uniform.reset(bgfx::createUniform("u_gridParams0", bgfx::UniformType::Vec4));
	impl_->grid_params1_uniform.reset(bgfx::createUniform("u_gridParams1", bgfx::UniformType::Vec4));
	impl_->grid_params2_uniform.reset(bgfx::createUniform("u_gridParams2", bgfx::UniformType::Vec4));

	impl_->ready = impl_->grid_vertex_buffer.valid() && impl_->grid_index_buffer.valid() && impl_->line_color_uniform.valid() && impl_->grid_plane_size_uniform.valid() && impl_->grid_color_uniform.valid() && impl_->major_grid_color_uniform.valid() && impl_->origin_u_color_uniform.valid() && impl_->origin_v_color_uniform.valid() && impl_->camera_position_uniform.valid() && impl_->grid_u_axis_uniform.valid() && impl_->grid_v_axis_uniform.valid() && impl_->grid_params0_uniform.valid() && impl_->grid_params1_uniform.valid() && impl_->grid_params2_uniform.valid();
	if (!impl_->ready) {
		push_overlay_resource_error(status, "GpuOverlayRenderer");
		shutdown();
		return false;
	}

	return true;
}

void GpuOverlayRenderer::shutdown() noexcept {
	impl_->grid_vertex_buffer.reset();
	impl_->grid_index_buffer.reset();
	impl_->line_color_uniform.reset();
	impl_->grid_plane_size_uniform.reset();
	impl_->grid_color_uniform.reset();
	impl_->major_grid_color_uniform.reset();
	impl_->origin_u_color_uniform.reset();
	impl_->origin_v_color_uniform.reset();
	impl_->camera_position_uniform.reset();
	impl_->grid_u_axis_uniform.reset();
	impl_->grid_v_axis_uniform.reset();
	impl_->grid_params0_uniform.reset();
	impl_->grid_params1_uniform.reset();
	impl_->grid_params2_uniform.reset();
	impl_->ready = false;
}

bool GpuOverlayRenderer::ready() const noexcept {
	return impl_->ready;
}

std::uint64_t grid_overlay_submit_state(OverlayDepthMode depth_mode) noexcept {
	std::uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA |
			BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
	if (depth_mode == OverlayDepthMode::DepthTested) {
		state |= BGFX_STATE_DEPTH_TEST_LEQUAL;
	}
	return state;
}

std::uint32_t GpuOverlayRenderer::submit_grid(
		bgfx::ViewId view_id,
		const RenderView &view,
		const PreparedGridOverlayCommand &prepared,
		bgfx::ProgramHandle program) const noexcept {
	if (!impl_->ready || !bgfx::isValid(program)) {
		return 0;
	}

	const GridOverlayCommand &grid = prepared.grid;
	float transform[16];
	bx::mtxSRT(
			transform,
			grid.plane_width,
			1.0F,
			grid.plane_height,
			grid.rotation_x,
			grid.rotation_y,
			grid.rotation_z,
			grid.origin.x,
			grid.origin.y,
			grid.origin.z);

	const float kPlaneSize[4] = { grid.plane_width, grid.plane_height, 0.0F, 0.0F };
	const float kCameraPosition[4] = { view.camera.eye.x, view.camera.eye.y, view.camera.eye.z, 0.0F };
	const float kGridUAxis[4] = { grid.u_axis.x, grid.u_axis.y, grid.u_axis.z, 0.0F };
	const float kGridVAxis[4] = { grid.v_axis.x, grid.v_axis.y, grid.v_axis.z, 0.0F };
	const float kParams0[4] = {
		grid.minor_spacing_m,
		std::max(grid.major_spacing_m, grid.minor_spacing_m),
		grid.minor_line_scale,
		grid.major_line_scale,
	};
	const float kParams1[4] = {
		grid.axis_line_scale,
		grid.fade_start_m,
		grid.fade_end_m,
		std::max(grid.plane_width, grid.plane_height) * 0.5F,
	};
	const float kParams2[4] = {
		std::max(0.0001F, grid.orthographic_height_m),
		std::max(1.0F, grid.viewport_height_px),
		grid.minor_spacing_m,
		grid.edge_softness_m,
	};

	bgfx::setTransform(transform);
	bgfx::setUniform(impl_->grid_plane_size_uniform.get(), kPlaneSize);
	bgfx::setUniform(impl_->grid_color_uniform.get(), prepared.minor_color_linear_sdr.data());
	bgfx::setUniform(impl_->major_grid_color_uniform.get(), prepared.major_color_linear_sdr.data());
	bgfx::setUniform(impl_->origin_u_color_uniform.get(), prepared.axis_u_color_linear_sdr.data());
	bgfx::setUniform(impl_->origin_v_color_uniform.get(), prepared.axis_v_color_linear_sdr.data());
	bgfx::setUniform(impl_->camera_position_uniform.get(), kCameraPosition);
	bgfx::setUniform(impl_->grid_u_axis_uniform.get(), kGridUAxis);
	bgfx::setUniform(impl_->grid_v_axis_uniform.get(), kGridVAxis);
	bgfx::setUniform(impl_->grid_params0_uniform.get(), kParams0);
	bgfx::setUniform(impl_->grid_params1_uniform.get(), kParams1);
	bgfx::setUniform(impl_->grid_params2_uniform.get(), kParams2);
	bgfx::setVertexBuffer(0, impl_->grid_vertex_buffer.get());
	bgfx::setIndexBuffer(impl_->grid_index_buffer.get());
	bgfx::setState(grid_overlay_submit_state(prepared.command.depth_mode));
	bgfx::submit(view_id, program);
	return 1;
}

std::uint32_t GpuOverlayRenderer::submit_lines(
		bgfx::ViewId view_id,
		const RenderView &view,
		const PreparedLineOverlayCommand &lines,
		bgfx::ProgramHandle program) const noexcept {
	if (!impl_->ready || !bgfx::isValid(program) || lines.segments.empty()) {
		return 0;
	}

	const std::uint32_t kQuadCount = available_quad_count(lines.segments.size(), impl_->line_vertex_layout);
	if (kQuadCount == 0U) {
		return 0;
	}

	const std::uint32_t kVertexCount = kQuadCount * kVerticesPerQuad;
	const std::uint32_t kIndexCount = kQuadCount * kIndicesPerQuad;
	bgfx::TransientVertexBuffer vertices{};
	bgfx::TransientIndexBuffer indices{};
	bgfx::allocTransientVertexBuffer(&vertices, kVertexCount, impl_->line_vertex_layout);
	bgfx::allocTransientIndexBuffer(&indices, kIndexCount);
	auto *data = reinterpret_cast<OverlayPositionVertex *>(vertices.data);
	auto *index_data = reinterpret_cast<std::uint16_t *>(indices.data);
	std::uint32_t vertex_index = 0;
	std::uint32_t index_index = 0;
	const float kThicknessPx = std::max(1.0F, lines.command.thickness_px);
	const float kDepthBias = lines.render_state.depth_test_enabled ? kOverlayLineDepthBiasUnits : 0.0F;
	const bool kHomogeneousDepth = current_render_homogeneous_depth();
	for (const LineOverlaySegment &segment : lines.segments) {
		if (vertex_index + kVerticesPerQuad > kVertexCount ||
				index_index + kIndicesPerQuad > kIndexCount) {
			break;
		}

		const std::optional<OverlayScreenSpaceQuad> kQuad = make_overlay_line_screen_space_quad(
				view,
				segment,
				kThicknessPx,
				kDepthBias,
				kHomogeneousDepth);
		if (!kQuad.has_value()) {
			continue;
		}
		append_quad(data,
				index_data,
				vertex_index,
				index_index,
				kQuad->corners[0],
				kQuad->corners[1],
				kQuad->corners[2],
				kQuad->corners[3]);
	}

	if (vertex_index == 0U || index_index == 0U) {
		return 0;
	}

	float transform[16];
	bx::mtxIdentity(transform);
	const std::uint64_t kState = state_for_overlay_render_state(lines.render_state);

	bgfx::setTransform(transform);
	bgfx::setUniform(impl_->line_color_uniform.get(), lines.color_linear_sdr.data());
	bgfx::setVertexBuffer(0, &vertices, 0, vertex_index);
	bgfx::setIndexBuffer(&indices, 0, index_index);
	bgfx::setState(kState);
	bgfx::submit(view_id, program);
	return 1;
}

std::uint32_t GpuOverlayRenderer::submit_triangles(
		bgfx::ViewId view_id,
		const PreparedTriangleOverlayCommand &triangles,
		bgfx::ProgramHandle program) const noexcept {
	if (!impl_->ready || !bgfx::isValid(program) || triangles.triangles.empty()) {
		return 0;
	}

	const std::uint32_t kVertexCount = clamped_vertex_count(triangles.triangles.size() * 3U);
	if (kVertexCount < 3U || bgfx::getAvailTransientVertexBuffer(kVertexCount, impl_->line_vertex_layout) < kVertexCount) {
		return 0;
	}

	bgfx::TransientVertexBuffer vertices{};
	bgfx::allocTransientVertexBuffer(&vertices, kVertexCount, impl_->line_vertex_layout);
	auto *data = reinterpret_cast<OverlayPositionVertex *>(vertices.data);
	std::uint32_t vertex_index = 0;
	for (const TriangleOverlayPrimitive &triangle : triangles.triangles) {
		if (vertex_index + 2U >= kVertexCount) {
			break;
		}
		data[vertex_index++] = OverlayPositionVertex{ triangle.a.x, triangle.a.y, triangle.a.z };
		data[vertex_index++] = OverlayPositionVertex{ triangle.b.x, triangle.b.y, triangle.b.z };
		data[vertex_index++] = OverlayPositionVertex{ triangle.c.x, triangle.c.y, triangle.c.z };
	}

	float transform[16];
	bx::mtxIdentity(transform);
	const std::uint64_t kState = state_for_overlay_render_state(triangles.render_state);

	bgfx::setTransform(transform);
	bgfx::setUniform(impl_->line_color_uniform.get(), triangles.color_linear_sdr.data());
	bgfx::setVertexBuffer(0, &vertices, 0, vertex_index);
	bgfx::setState(kState);
	bgfx::submit(view_id, program);
	return 1;
}

std::uint32_t GpuOverlayRenderer::submit_points(
		bgfx::ViewId view_id,
		const RenderView &view,
		const PreparedPointOverlayCommand &points,
		bgfx::ProgramHandle program) const noexcept {
	if (!impl_->ready || !bgfx::isValid(program) || points.points.empty()) {
		return 0;
	}

	const std::uint32_t kQuadCount = available_quad_count(points.points.size(), impl_->line_vertex_layout);
	if (kQuadCount == 0U) {
		return 0;
	}

	const std::uint32_t kVertexCount = kQuadCount * kVerticesPerQuad;
	const std::uint32_t kIndexCount = kQuadCount * kIndicesPerQuad;
	bgfx::TransientVertexBuffer vertices{};
	bgfx::TransientIndexBuffer indices{};
	bgfx::allocTransientVertexBuffer(&vertices, kVertexCount, impl_->line_vertex_layout);
	bgfx::allocTransientIndexBuffer(&indices, kIndexCount);
	auto *data = reinterpret_cast<OverlayPositionVertex *>(vertices.data);
	auto *index_data = reinterpret_cast<std::uint16_t *>(indices.data);
	std::uint32_t vertex_index = 0;
	std::uint32_t index_index = 0;
	const bool kHomogeneousDepth = current_render_homogeneous_depth();
	const float kDepthBias = points.render_state.depth_test_enabled ? kOverlayPointDepthBiasUnits : 0.0F;
	for (const PointOverlayPrimitive &point : points.points) {
		if (vertex_index + kVerticesPerQuad > kVertexCount ||
				index_index + kIndicesPerQuad > kIndexCount) {
			break;
		}

		const float kSizePx = std::max(1.0F, point.size_px > 0.0F ? point.size_px : points.size_px);
		const std::optional<OverlayScreenSpaceQuad> kQuad = make_overlay_point_screen_space_quad(
				view,
				point.position,
				kSizePx,
				kDepthBias,
				kHomogeneousDepth);
		if (!kQuad.has_value()) {
			continue;
		}
		append_quad(data,
				index_data,
				vertex_index,
				index_index,
				kQuad->corners[0],
				kQuad->corners[1],
				kQuad->corners[2],
				kQuad->corners[3]);
	}

	if (vertex_index == 0U || index_index == 0U) {
		return 0;
	}

	float transform[16];
	bx::mtxIdentity(transform);
	const std::uint64_t kState = state_for_overlay_render_state(points.render_state);

	bgfx::setTransform(transform);
	bgfx::setUniform(impl_->line_color_uniform.get(), points.color_linear_sdr.data());
	bgfx::setVertexBuffer(0, &vertices, 0, vertex_index);
	bgfx::setIndexBuffer(&indices, 0, index_index);
	bgfx::setState(kState);
	bgfx::submit(view_id, program);
	return 1;
}

} // namespace crimson::gpu
