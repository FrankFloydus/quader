/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace crimson {

/// Severity level for renderer diagnostics.
enum class RendererDiagnosticSeverity {
	/// Informational diagnostic.
	Info,
	/// Recoverable renderer warning.
	Warning,
	/// Error that prevents part of the requested operation.
	Error,
	/// Fatal error that prevents renderer startup or continued operation.
	Fatal,
};

/// Machine-readable renderer diagnostic category.
enum class RendererDiagnosticCode {
	/// Native host surface is missing or invalid.
	SurfaceUnavailable,
	/// Requested graphics backend is unavailable.
	BackendUnsupported,
	/// Required renderer capability is missing.
	CapabilityMissing,
	/// Graphics runtime initialization failed.
	RuntimeInitializationFailed,
	/// Shader manifest is missing or inconsistent.
	ShaderManifestInvalid,
	/// Compiled shader target is missing.
	ShaderTargetMissing,
	/// Compiled shader binary is missing.
	ShaderFileMissing,
	/// GPU shader program creation failed.
	ShaderProgramCreationFailed,
	/// GPU resource creation failed.
	ResourceCreationFailed,
	/// Render graph validation failed.
	FrameGraphValidationFailed,
	/// Frame snapshot is invalid.
	InvalidFrameSnapshot,
	/// Resize request failed.
	ResizeFailed,
	/// Color-space configuration is invalid.
	ColorSpaceInvalid,
	/// Material schema is invalid.
	MaterialSchemaInvalid,
	/// Material instance failed validation.
	MaterialValidationFailed,
	/// Picking readback failed.
	PickingReadbackFailed,
	/// Framebuffer format or setup is unsupported.
	FrameBufferUnsupported,
	/// Runtime resource lifetime is inconsistent.
	ResourceLifetimeError,
	/// Performance counter data is invalid.
	PerformanceCounterInvalid,
	/// Golden image capture is unsupported by the active path.
	GoldenCaptureUnsupported,
	/// Golden image readback failed.
	GoldenReadbackFailed,
	/// Upload was skipped because the resource was invalid.
	UploadSkippedInvalidResource,
	/// Benchmark configuration is invalid.
	BenchmarkConfigurationInvalid,
};

/// Renderer capability used by startup validation and diagnostics.
enum class RendererCapability {
	/// Hardware instancing support.
	Instancing,
	/// 2D texture support.
	Texture2D,
	/// Cube texture support.
	TextureCube,
	/// Floating-point render-target support.
	FloatingPointRenderTarget,
	/// Render-to-texture support.
	RenderToTexture,
	/// Depth texture support.
	DepthTexture,
	/// Hardware sRGB texture sampling support.
	SrgbTextureSampling,
	/// sRGB backbuffer support.
	SrgbBackbuffer,
	/// Manual final sRGB conversion support.
	ManualSrgbFinalConversion,
	/// Integer picking target support.
	IntegerPickingTarget,
	/// RGBA8 encoded picking target support.
	Rgba8PickingTarget,
	/// GPU readback support.
	Readback,
};

/// Support status for one renderer capability.
struct RendererCapabilityStatus {
	/// Capability being reported.
	RendererCapability capability = RendererCapability::Instancing;
	/// True when the active backend supports the capability.
	bool supported = false;
	/// Optional backend-specific detail.
	std::string detail;
};

/// Structured renderer diagnostic entry.
struct RendererDiagnostic {
	/// Diagnostic severity.
	RendererDiagnosticSeverity severity = RendererDiagnosticSeverity::Info;
	/// Machine-readable diagnostic category.
	RendererDiagnosticCode code = RendererDiagnosticCode::CapabilityMissing;
	/// Short diagnostic message.
	std::string message;
	/// Optional extended detail.
	std::string detail;
	/// Subsystem that produced the diagnostic, when available.
	std::string subsystem;
	/// Resource associated with the diagnostic, when available.
	std::string resource_name;
	/// Renderer frame index associated with the diagnostic, or zero when not frame-specific.
	std::uint64_t frame_index = 0;
};

/// Snapshot of renderer startup/runtime status.
struct RendererStatus {
	/// Selected backend display name.
	std::string backend_name;
	/// True after successful renderer initialization.
	bool initialized = false;
	/// Capability support rows for the active backend.
	std::vector<RendererCapabilityStatus> capabilities;
	/// Diagnostics accumulated by the renderer.
	std::vector<RendererDiagnostic> diagnostics;
};

/**
 * Return the stable severity name.
 *
 * @param severity Severity to name.
 * @return Static severity name.
 */
constexpr std::string_view renderer_diagnostic_severity_name(RendererDiagnosticSeverity severity) noexcept {
	switch (severity) {
		case RendererDiagnosticSeverity::Info:
			return "Info";
		case RendererDiagnosticSeverity::Warning:
			return "Warning";
		case RendererDiagnosticSeverity::Error:
			return "Error";
		case RendererDiagnosticSeverity::Fatal:
			return "Fatal";
	}

	return "Unknown";
}

/**
 * Return the stable diagnostic-code name.
 *
 * @param code Diagnostic code to name.
 * @return Static diagnostic-code name.
 */
constexpr std::string_view renderer_diagnostic_code_name(RendererDiagnosticCode code) noexcept {
	switch (code) {
		case RendererDiagnosticCode::SurfaceUnavailable:
			return "SurfaceUnavailable";
		case RendererDiagnosticCode::BackendUnsupported:
			return "BackendUnsupported";
		case RendererDiagnosticCode::CapabilityMissing:
			return "CapabilityMissing";
		case RendererDiagnosticCode::RuntimeInitializationFailed:
			return "RuntimeInitializationFailed";
		case RendererDiagnosticCode::ShaderManifestInvalid:
			return "ShaderManifestInvalid";
		case RendererDiagnosticCode::ShaderTargetMissing:
			return "ShaderTargetMissing";
		case RendererDiagnosticCode::ShaderFileMissing:
			return "ShaderFileMissing";
		case RendererDiagnosticCode::ShaderProgramCreationFailed:
			return "ShaderProgramCreationFailed";
		case RendererDiagnosticCode::ResourceCreationFailed:
			return "ResourceCreationFailed";
		case RendererDiagnosticCode::FrameGraphValidationFailed:
			return "FrameGraphValidationFailed";
		case RendererDiagnosticCode::InvalidFrameSnapshot:
			return "InvalidFrameSnapshot";
		case RendererDiagnosticCode::ResizeFailed:
			return "ResizeFailed";
		case RendererDiagnosticCode::ColorSpaceInvalid:
			return "ColorSpaceInvalid";
		case RendererDiagnosticCode::MaterialSchemaInvalid:
			return "MaterialSchemaInvalid";
		case RendererDiagnosticCode::MaterialValidationFailed:
			return "MaterialValidationFailed";
		case RendererDiagnosticCode::PickingReadbackFailed:
			return "PickingReadbackFailed";
		case RendererDiagnosticCode::FrameBufferUnsupported:
			return "FrameBufferUnsupported";
		case RendererDiagnosticCode::ResourceLifetimeError:
			return "ResourceLifetimeError";
		case RendererDiagnosticCode::PerformanceCounterInvalid:
			return "PerformanceCounterInvalid";
		case RendererDiagnosticCode::GoldenCaptureUnsupported:
			return "GoldenCaptureUnsupported";
		case RendererDiagnosticCode::GoldenReadbackFailed:
			return "GoldenReadbackFailed";
		case RendererDiagnosticCode::UploadSkippedInvalidResource:
			return "UploadSkippedInvalidResource";
		case RendererDiagnosticCode::BenchmarkConfigurationInvalid:
			return "BenchmarkConfigurationInvalid";
	}

	return "Unknown";
}

/**
 * Return the stable capability name.
 *
 * @param capability Capability to name.
 * @return Static capability name.
 */
constexpr std::string_view renderer_capability_name(RendererCapability capability) noexcept {
	switch (capability) {
		case RendererCapability::Instancing:
			return "Instancing";
		case RendererCapability::Texture2D:
			return "Texture2D";
		case RendererCapability::TextureCube:
			return "TextureCube";
		case RendererCapability::FloatingPointRenderTarget:
			return "FloatingPointRenderTarget";
		case RendererCapability::RenderToTexture:
			return "RenderToTexture";
		case RendererCapability::DepthTexture:
			return "DepthTexture";
		case RendererCapability::SrgbTextureSampling:
			return "SrgbTextureSampling";
		case RendererCapability::SrgbBackbuffer:
			return "SrgbBackbuffer";
		case RendererCapability::ManualSrgbFinalConversion:
			return "ManualSrgbFinalConversion";
		case RendererCapability::IntegerPickingTarget:
			return "IntegerPickingTarget";
		case RendererCapability::Rgba8PickingTarget:
			return "Rgba8PickingTarget";
		case RendererCapability::Readback:
			return "Readback";
	}

	return "Unknown";
}

/**
 * Check whether renderer status contains an error or fatal diagnostic.
 *
 * @param status Status to inspect.
 * @return True when any diagnostic is `Error` or `Fatal`.
 */
inline bool has_error_diagnostic(const RendererStatus &status) {
	for (const RendererDiagnostic &diagnostic : status.diagnostics) {
		if (diagnostic.severity == RendererDiagnosticSeverity::Error || diagnostic.severity == RendererDiagnosticSeverity::Fatal) {
			return true;
		}
	}

	return false;
}

/**
 * Check whether a diagnostic has subsystem and resource context.
 *
 * @param diagnostic Diagnostic to inspect.
 * @return True when both context fields are non-empty.
 */
inline bool has_structured_context(const RendererDiagnostic &diagnostic) noexcept {
	return !diagnostic.subsystem.empty() && !diagnostic.resource_name.empty();
}

} // namespace crimson
