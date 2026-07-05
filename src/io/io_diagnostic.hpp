#pragma once

#include "foundation/diagnostic.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace quader::io {

enum class IoSeverity {
	Info,
	Warning,
	Error,
};

enum class IoDiagnosticCode {
	Unknown,
	UnsupportedFormat,
	FileNotFound,
	FileReadFailed,
	FileWriteFailed,
	ParseFailed,
	ValidationFailed,
	MeshValidationFailed,
	DocumentValidationFailed,
	MaterialMappingFailed,
	UnsupportedFeaturePreserved,
	ExportNotSupported,
};

struct SourceLocation {
	std::uint32_t line = 0;
	std::uint32_t column = 0;
	std::string json_pointer;
};

struct IoDiagnostic {
	IoSeverity severity = IoSeverity::Info;
	IoDiagnosticCode code = IoDiagnosticCode::Unknown;
	std::string message;
	std::filesystem::path source_path;
	SourceLocation location;
	std::string subject;
};

class IoDiagnosticList final {
public:
	using container_type = std::vector<IoDiagnostic>;
	using const_iterator = container_type::const_iterator;

	void add(IoDiagnostic diagnostic);
	void add(IoSeverity severity, IoDiagnosticCode code, std::string message);
	void add_error(IoDiagnosticCode code, std::string message);
	void append(const IoDiagnosticList &diagnostics);
	void clear() noexcept;

	[[nodiscard]] bool empty() const noexcept;
	[[nodiscard]] bool has_errors() const noexcept;
	[[nodiscard]] std::size_t size() const noexcept;
	[[nodiscard]] std::span<const IoDiagnostic> diagnostics() const noexcept;
	[[nodiscard]] const IoDiagnostic &operator[](std::size_t index) const noexcept;
	[[nodiscard]] const_iterator begin() const noexcept;
	[[nodiscard]] const_iterator end() const noexcept;

private:
	container_type diagnostics_;
};

[[nodiscard]] quader::foundation::Severity to_foundation_severity(IoSeverity severity) noexcept;
[[nodiscard]] quader::foundation::Diagnostic to_foundation_diagnostic(
		const IoDiagnostic &diagnostic);
[[nodiscard]] quader::foundation::DiagnosticList to_foundation_diagnostics(
		const IoDiagnosticList &diagnostics);

} // namespace quader::io
