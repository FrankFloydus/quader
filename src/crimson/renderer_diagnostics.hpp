#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace crimson {

enum class RendererDiagnosticSeverity {
	Info,
	Warning,
	Error,
	Fatal,
};

enum class RendererDiagnosticCode {
	SurfaceUnavailable,
	BackendUnsupported,
	CapabilityMissing,
	RuntimeInitializationFailed,
	ShaderManifestInvalid,
	ShaderTargetMissing,
	ShaderFileMissing,
	ShaderProgramCreationFailed,
	ResourceCreationFailed,
	FrameGraphValidationFailed,
	InvalidFrameSnapshot,
	ResizeFailed,
	ColorSpaceInvalid,
	MaterialSchemaInvalid,
	MaterialValidationFailed,
	PickingReadbackFailed,
	FrameBufferUnsupported,
	ResourceLifetimeError,
	PerformanceCounterInvalid,
	GoldenCaptureUnsupported,
	GoldenReadbackFailed,
	UploadSkippedInvalidResource,
	BenchmarkConfigurationInvalid,
};

enum class RendererCapability {
	Instancing,
	Texture2D,
	TextureCube,
	FloatingPointRenderTarget,
	RenderToTexture,
	DepthTexture,
	SrgbTextureSampling,
	SrgbBackbuffer,
	ManualSrgbFinalConversion,
	IntegerPickingTarget,
	Rgba8PickingTarget,
	Readback,
};

struct RendererCapabilityStatus {
	RendererCapability capability = RendererCapability::Instancing;
	bool supported = false;
	std::string detail;
};

struct RendererDiagnostic {
	RendererDiagnosticSeverity severity = RendererDiagnosticSeverity::Info;
	RendererDiagnosticCode code = RendererDiagnosticCode::CapabilityMissing;
	std::string message;
	std::string detail;
	std::string subsystem;
	std::string resource_name;
	std::uint64_t frame_index = 0;
};

struct RendererStatus {
	std::string backend_name;
	bool initialized = false;
	std::vector<RendererCapabilityStatus> capabilities;
	std::vector<RendererDiagnostic> diagnostics;
};

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

inline bool has_error_diagnostic(const RendererStatus &status) {
	for (const RendererDiagnostic &diagnostic : status.diagnostics) {
		if (diagnostic.severity == RendererDiagnosticSeverity::Error || diagnostic.severity == RendererDiagnosticSeverity::Fatal) {
			return true;
		}
	}

	return false;
}

inline bool has_structured_context(const RendererDiagnostic &diagnostic) noexcept {
	return !diagnostic.subsystem.empty() && !diagnostic.resource_name.empty();
}

} // namespace crimson
