/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/native_surface.hpp"
#include "crimson/renderer_config.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "crimson/renderer_types.hpp"
#include "crimson/scene/render_camera_projection.hpp"
#include "crimson/scene/render_object.hpp"
#include "math/vec3.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string_view>

namespace {

template <typename T>
concept HasPublicProgramMember = requires(T object) {
	object.program;
};

[[nodiscard]] quader::math::Vec3 normalized_or(quader::math::Vec3 value,
		quader::math::Vec3 fallback) noexcept {
	const quader::math::Vec3 kNormalized = quader::math::normalized(value);
	return quader::math::length_squared(kNormalized) <= 0.000001F ? fallback : kNormalized;
}

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

TEST(RendererPublicTypes, HandlesEncodeInvalidAndGenerationState) {
	expect_true(!crimson::is_valid_handle(crimson::RenderMeshHandle{}), "default mesh handle is invalid");
	expect_true(!crimson::is_valid_handle(crimson::RenderTextureHandle{}), "default texture handle is invalid");
	expect_true(crimson::is_valid_handle(crimson::RenderMeshHandle{ 1, 1 }), "non-zero mesh handle is valid");
	expect_true(
			crimson::RenderMeshHandle{ 7, 2 } == crimson::RenderMeshHandle{ 7, 2 },
			"matching mesh handles compare equal");
	expect_true(
			crimson::RenderMeshHandle{ 7, 2 } != crimson::RenderMeshHandle{ 7, 3 },
			"generation participates in mesh handle equality");
}

TEST(RendererPublicTypes, ExtentAndNativeSurfaceValidationRejectInvalidSurfaces) {
	expect_true(crimson::is_valid_extent(crimson::ViewportExtent{}), "default extent is valid");
	expect_true(
			!crimson::is_valid_extent(crimson::ViewportExtent{ 0, 1, 1.0f }),
			"zero width is an invalid extent");
	expect_true(
			!crimson::is_valid_extent(crimson::ViewportExtent{ 1, 1, 0.0f }),
			"zero device pixel ratio is invalid");

	crimson::NativeSurfaceDescriptor surface;
	expect_true(!crimson::is_valid_native_surface_descriptor(surface), "default surface is invalid");

	surface.platform = crimson::NativeSurfacePlatform::Windows;
	surface.native_window_handle = reinterpret_cast<void *>(0x1);
	surface.extent = crimson::ViewportExtent{ 1280, 720, 1.0f };
	expect_true(crimson::is_valid_native_surface_descriptor(surface), "populated Windows surface is valid");
}

TEST(RendererPublicTypes, NamesAreStableForPublicDiagnosticsAndConfig) {
	expect_true(
			crimson::shader_program_id_name(crimson::ShaderProgramId::ViewportLitCube) == "ViewportLitCube",
			"viewport cube program name is stable");
	expect_true(
			crimson::shader_program_id_name(crimson::ShaderProgramId::ToneMap) == "ToneMap",
			"tone-map program name is stable");
	expect_true(
			crimson::shader_program_id_name(crimson::ShaderProgramId::OpaquePbr) == "OpaquePbr",
			"OpaquePbr program name is stable");
	expect_true(
			crimson::shader_program_id_name(crimson::ShaderProgramId::OverlayLine) == "OverlayLine",
			"OverlayLine program name is stable");
	expect_true(
			crimson::shader_program_id_name(crimson::ShaderProgramId::Picking) == "Picking",
			"Picking program name is stable");
	expect_true(
			crimson::graphics_backend_preference_name(crimson::GraphicsBackendPreference::Direct3D12) == "Direct3D12",
			"backend preference name is stable");
	expect_true(
			crimson::renderer_diagnostic_code_name(crimson::RendererDiagnosticCode::ShaderFileMissing) == "ShaderFileMissing",
			"shader diagnostic code name is stable");
	expect_true(
			crimson::native_surface_platform_name(crimson::NativeSurfacePlatform::LinuxWayland) == "LinuxWayland",
			"native surface platform name is stable");
}

TEST(RendererPublicTypes, StatusReportsErrorSeverity) {
	crimson::RendererStatus status;
	expect_true(!crimson::has_error_diagnostic(status), "empty status has no error diagnostics");

	status.diagnostics.push_back(crimson::RendererDiagnostic{
			.severity = crimson::RendererDiagnosticSeverity::Warning,
			.code = crimson::RendererDiagnosticCode::CapabilityMissing,
	});
	expect_true(!crimson::has_error_diagnostic(status), "warning status has no error diagnostics");

	status.diagnostics.push_back(crimson::RendererDiagnostic{
			.severity = crimson::RendererDiagnosticSeverity::Error,
			.code = crimson::RendererDiagnosticCode::SurfaceUnavailable,
	});
	expect_true(crimson::has_error_diagnostic(status), "error status is detected");
}

TEST(RendererPublicTypes, RenderObjectContractDoesNotExposeProgramSelection) {
	expect_true(
			!HasPublicProgramMember<crimson::RenderObject>,
			"RenderObject does not expose arbitrary shader program selection");
}

TEST(RendererPublicTypes, CameraProjectionRaysFollowCrimsonRenderedViewBasis) {
	const crimson::RenderCamera kCamera{
		.eye = { 0.0F, 0.0F, 10.0F },
		.target = { 0.0F, 0.0F, 0.0F },
		.up = { 0.0F, 1.0F, 0.0F },
		.forward = { 0.0F, 0.0F, -1.0F },
		.vertical_fov_degrees = 60.0F,
	};
	const crimson::RenderCameraViewportSize kViewport{ 100.0F, 100.0F };
	const quader::math::Vec3 kRenderedRight = normalized_or(
			quader::math::cross(kCamera.up, kCamera.forward),
			{ -1.0F, 0.0F, 0.0F });

	const crimson::RenderCameraRay kCenterRay = crimson::render_camera_ray_from_viewport_point(
			kCamera,
			kViewport,
			{ 50.0F, 50.0F },
			false);
	expect_true(
			quader::math::dot(kCenterRay.direction, kCamera.forward) > 0.999F,
			"center pixel ray follows the rendered camera forward");

	const crimson::RenderCameraRay kRightRay = crimson::render_camera_ray_from_viewport_point(
			kCamera,
			kViewport,
			{ 90.0F, 50.0F },
			false);
	expect_true(
			quader::math::dot(kRightRay.direction, kRenderedRight) > 0.0F,
			"right-side viewport ray points toward Crimson's rendered right axis");

	const crimson::RenderCameraRay kLeftRay = crimson::render_camera_ray_from_viewport_point(
			kCamera,
			kViewport,
			{ 10.0F, 50.0F },
			false);
	expect_true(
			quader::math::dot(kLeftRay.direction, kRenderedRight) < 0.0F,
			"left-side viewport ray points away from Crimson's rendered right axis");
}

TEST(RendererPublicTypes, OrthographicCameraProjectionOffsetsTopRightAlongRenderedAxes) {
	const crimson::RenderCamera kCamera{
		.eye = { 0.0F, 0.0F, 10.0F },
		.target = { 0.0F, 0.0F, 0.0F },
		.up = { 0.0F, 1.0F, 0.0F },
		.forward = { 0.0F, 0.0F, -1.0F },
		.projection = crimson::CameraProjection::Orthographic,
		.orthographic_height_m = 10.0F,
	};
	const crimson::RenderCameraRay kRay = crimson::render_camera_ray_from_viewport_point(
			kCamera,
			{ 100.0F, 100.0F },
			{ 100.0F, 0.0F },
			false);
	const quader::math::Vec3 kRenderedRight = normalized_or(
			quader::math::cross(kCamera.up, kCamera.forward),
			{ -1.0F, 0.0F, 0.0F });
	const quader::math::Vec3 kDelta = kRay.origin - kCamera.target;

	expect_true(
			quader::math::dot(kDelta, kRenderedRight) > 0.0F,
			"orthographic top-right ray origin offsets along rendered right");
	expect_true(
			quader::math::dot(kDelta, kCamera.up) > 0.0F,
			"orthographic top-right ray origin offsets along camera up");
	expect_true(
			quader::math::dot(kRay.direction, kCamera.forward) > 0.999F,
			"orthographic ray direction follows camera forward");
}

} // namespace
