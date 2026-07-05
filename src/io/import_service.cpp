#include "io/import_service.hpp"

#include "io/document_fragment_validator.hpp"
#include "io/imported_document.hpp"

#include <string>
#include <type_traits>
#include <variant>

namespace quader::io {
namespace {

[[nodiscard]] IoDiagnostic first_error_or_fallback(const IoDiagnosticList &diagnostics,
		IoDiagnostic fallback) {
	for (const auto &diagnostic : diagnostics) {
		if (diagnostic.severity == IoSeverity::Error) {
			return diagnostic;
		}
	}

	return fallback;
}

[[nodiscard]] IoError validation_error_from_diagnostics(const IoDiagnosticList &diagnostics,
		const std::filesystem::path &path) {
	IoError error = make_io_error(
			IoErrorCode::ValidationFailed,
			first_error_or_fallback(
					diagnostics,
					IoDiagnostic{
							IoSeverity::Error,
							IoDiagnosticCode::ValidationFailed,
							"imported content failed validation",
							path,
							{},
							{},
					}));
	error.related = diagnostics;
	return error;
}

void append_fragment_diagnostics(const DocumentFragment &fragment, IoDiagnosticList &diagnostics) {
	diagnostics.append(fragment.diagnostics);
	for (const auto &material : fragment.materials) {
		diagnostics.append(material.diagnostics);
	}
}

void append_imported_document_diagnostics(const ImportedDocument &imported,
		IoDiagnosticList &diagnostics) {
	diagnostics.append(imported.diagnostics);
	append_fragment_diagnostics(imported.root_fragment, diagnostics);
}

void append_payload_diagnostics(const ImportResult &result, IoDiagnosticList &diagnostics) {
	diagnostics.append(result.diagnostics);
	std::visit(
			[&diagnostics](const auto &payload) {
				using Payload = std::decay_t<decltype(payload)>;
				if constexpr (std::is_same_v<Payload, ImportedDocument>) {
					append_imported_document_diagnostics(payload, diagnostics);
				} else {
					append_fragment_diagnostics(payload, diagnostics);
				}
			},
			result.payload);
}

void append_empty_replace_error(const std::filesystem::path &path,
		const DocumentFragment &fragment,
		IoDiagnosticList &diagnostics) {
	if (!fragment.mesh_objects.empty()) {
		return;
	}

	diagnostics.add(IoDiagnostic{
			IoSeverity::Error,
			IoDiagnosticCode::ValidationFailed,
			"cannot replace a document with an empty import",
			path,
			{},
			"document",
	});
}

void validate_payload(const ImportRequest &request, ImportResult &result, IoDiagnosticList &diagnostics) {
	std::visit(
			[&request, &diagnostics](const auto &payload) {
				using Payload = std::decay_t<decltype(payload)>;
				if constexpr (std::is_same_v<Payload, ImportedDocument>) {
					auto validation = validate_document_fragment(payload.root_fragment);
					diagnostics.append(validation.diagnostics);
					if (request.options.mode == DocumentImportMode::ReplaceDocument) {
						append_empty_replace_error(request.path, payload.root_fragment, diagnostics);
					}
				} else {
					auto validation = validate_document_fragment(payload);
					diagnostics.append(validation.diagnostics);
					if (request.options.mode == DocumentImportMode::ReplaceDocument) {
						append_empty_replace_error(request.path, payload, diagnostics);
					}
				}
			},
			result.payload);
}

} // namespace

ImportService::ImportService(const ImportExportRegistry &registry) : registry_(registry) {
}

quader::foundation::Result<ImportResult, IoError> ImportService::import_file(
		const ImportRequest &request) const {
	const IImporter *importer = registry_.importer_for_path(request.path);
	if (importer == nullptr) {
		return quader::foundation::Result<ImportResult, IoError>::failure(make_io_error(
				IoErrorCode::UnsupportedFormat,
				IoDiagnosticCode::UnsupportedFormat,
				"no importer is registered for '" + path_extension_without_dot(request.path) + "' files",
				request.path));
	}

	auto imported = importer->import_file(request);
	if (!imported) {
		return imported;
	}

	ImportResult result = std::move(imported).value();
	IoDiagnosticList diagnostics;
	append_payload_diagnostics(result, diagnostics);
	validate_payload(request, result, diagnostics);
	result.diagnostics = diagnostics;

	if (diagnostics.has_errors()) {
		return quader::foundation::Result<ImportResult, IoError>::failure(
				validation_error_from_diagnostics(diagnostics, request.path));
	}

	return quader::foundation::Result<ImportResult, IoError>::success(std::move(result));
}

} // namespace quader::io
