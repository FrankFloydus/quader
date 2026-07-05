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

#include "foundation/diagnostic.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace quader::io {

/// Severity level for import/export diagnostics.
enum class IoSeverity {
	/// Informational diagnostic with no failure implied.
	Info,
	/// Recoverable or noteworthy condition.
	Warning,
	/// Error condition that should block the current operation.
	Error,
};

/// Machine-readable import/export diagnostic category.
enum class IoDiagnosticCode {
	/// Unclassified diagnostic.
	Unknown,
	/// No importer or exporter supports the requested format.
	UnsupportedFormat,
	/// Source file was not found.
	FileNotFound,
	/// Source file could not be read.
	FileReadFailed,
	/// Destination file could not be written.
	FileWriteFailed,
	/// Source data could not be parsed.
	ParseFailed,
	/// Imported or exported data failed validation.
	ValidationFailed,
	/// Mesh validation failed.
	MeshValidationFailed,
	/// Document validation failed.
	DocumentValidationFailed,
	/// Material mapping failed.
	MaterialMappingFailed,
	/// Unsupported source feature was preserved as metadata.
	UnsupportedFeaturePreserved,
	/// Requested export operation is unsupported.
	ExportNotSupported,
};

/// Optional source location attached to an I/O diagnostic.
struct SourceLocation {
	/// One-based source line, or zero when unknown.
	std::uint32_t line = 0;
	/// One-based source column, or zero when unknown.
	std::uint32_t column = 0;
	/// JSON pointer or similar structured path when available.
	std::string json_pointer;
};

/// One diagnostic produced by an import/export operation.
struct IoDiagnostic {
	/// Diagnostic severity.
	IoSeverity severity = IoSeverity::Info;
	/// Machine-readable diagnostic category.
	IoDiagnosticCode code = IoDiagnosticCode::Unknown;
	/// Human-readable diagnostic message.
	std::string message;
	/// Source path related to the diagnostic, when available.
	std::filesystem::path source_path;
	/// Structured source location, when available.
	SourceLocation location;
	/// Optional subject such as object, mesh, or material name.
	std::string subject;
};

/// Ordered collection of import/export diagnostics.
class IoDiagnosticList final {
public:
	/// Container type used to store diagnostics.
	using container_type = std::vector<IoDiagnostic>;
	/// Constant iterator over stored diagnostics.
	using const_iterator = container_type::const_iterator;

	/**
	 * Append a diagnostic.
	 *
	 * @param diagnostic Diagnostic to append.
	 */
	void add(IoDiagnostic diagnostic);
	/**
	 * Append a diagnostic from fields.
	 *
	 * @param severity Severity assigned to the diagnostic.
	 * @param code Diagnostic category.
	 * @param message Message to store.
	 */
	void add(IoSeverity severity, IoDiagnosticCode code, std::string message);
	/**
	 * Append an error diagnostic.
	 *
	 * @param code Diagnostic category.
	 * @param message Error message to store.
	 */
	void add_error(IoDiagnosticCode code, std::string message);
	/**
	 * Append diagnostics from another list.
	 *
	 * @param diagnostics Diagnostics copied into this list.
	 */
	void append(const IoDiagnosticList &diagnostics);
	/// Remove all diagnostics.
	void clear() noexcept;

	/// Return true when no diagnostics are stored.
	[[nodiscard]] bool empty() const noexcept;
	/// Return true when at least one diagnostic has `IoSeverity::Error`.
	[[nodiscard]] bool has_errors() const noexcept;
	/// Return the number of stored diagnostics.
	[[nodiscard]] std::size_t size() const noexcept;
	/// Return a borrowed span of stored diagnostics.
	[[nodiscard]] std::span<const IoDiagnostic> diagnostics() const noexcept;
	/**
	 * Access a diagnostic by index.
	 *
	 * @param index Zero-based diagnostic index.
	 * @return Diagnostic at `index`.
	 */
	[[nodiscard]] const IoDiagnostic &operator[](std::size_t index) const noexcept;
	/// Return an iterator to the first diagnostic.
	[[nodiscard]] const_iterator begin() const noexcept;
	/// Return an iterator one past the last diagnostic.
	[[nodiscard]] const_iterator end() const noexcept;

private:
	container_type diagnostics_;
};

/**
 * Convert an I/O severity to a foundation severity.
 *
 * @param severity I/O severity to convert.
 * @return Equivalent foundation severity.
 */
[[nodiscard]] quader::foundation::Severity to_foundation_severity(IoSeverity severity) noexcept;
/**
 * Convert an I/O diagnostic to a foundation diagnostic.
 *
 * @param diagnostic I/O diagnostic to convert.
 * @return Foundation diagnostic carrying severity and message.
 */
[[nodiscard]] quader::foundation::Diagnostic to_foundation_diagnostic(
		const IoDiagnostic &diagnostic);
/**
 * Convert an I/O diagnostic list to a foundation diagnostic list.
 *
 * @param diagnostics I/O diagnostics to copy.
 * @return Foundation diagnostic list.
 */
[[nodiscard]] quader::foundation::DiagnosticList to_foundation_diagnostics(
		const IoDiagnosticList &diagnostics);

} // namespace quader::io
