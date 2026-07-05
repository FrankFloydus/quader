/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "io/document_fragment_validator.hpp"

#include "document/document_error.hpp"
#include "mesh/core/mesh_validation.hpp"

#include <string>
#include <utility>

namespace quader::io {
namespace {

[[nodiscard]] std::string object_subject(std::size_t index, const ImportedMeshObject &object) {
	if (!object.name.empty()) {
		return object.name;
	}

	return "mesh_object[" + std::to_string(index) + "]";
}

void add_fragment_diagnostic(IoDiagnosticList &diagnostics,
		IoSeverity severity,
		IoDiagnosticCode code,
		std::string message,
		std::string subject) {
	diagnostics.add(IoDiagnostic{
			severity,
			code,
			std::move(message),
			{},
			{},
			std::move(subject),
	});
}

[[nodiscard]] IoDiagnostic first_error_or_fallback(const IoDiagnosticList &diagnostics,
		IoDiagnostic fallback) {
	for (const auto &diagnostic : diagnostics) {
		if (diagnostic.severity == IoSeverity::Error) {
			return diagnostic;
		}
	}

	return fallback;
}

[[nodiscard]] IoError validation_error_from_diagnostics(const IoDiagnosticList &diagnostics) {
	IoError error = make_io_error(
			IoErrorCode::ValidationFailed,
			first_error_or_fallback(
					diagnostics,
					IoDiagnostic{
							IoSeverity::Error,
							IoDiagnosticCode::ValidationFailed,
							"imported document failed validation",
							{},
							{},
							"document",
					}));
	error.related = diagnostics;
	return error;
}

void append_import_diagnostics(const ImportedDocument &imported, IoDiagnosticList &diagnostics) {
	diagnostics.append(imported.diagnostics);
	diagnostics.append(imported.root_fragment.diagnostics);
	for (const auto &material : imported.root_fragment.materials) {
		diagnostics.append(material.diagnostics);
	}
}

} // namespace

bool FragmentValidationResult::ok() const noexcept {
	return !diagnostics.has_errors();
}

FragmentValidationResult validate_document_fragment(const DocumentFragment &fragment) {
	FragmentValidationResult result;

	if (fragment.mesh_objects.empty()) {
		add_fragment_diagnostic(result.diagnostics,
				IoSeverity::Warning,
				IoDiagnosticCode::ValidationFailed,
				"document fragment does not contain mesh objects",
				"fragment");
	}

	for (std::size_t index = 0; index < fragment.mesh_objects.size(); ++index) {
		const auto &object = fragment.mesh_objects[index];
		const auto kSubject = object_subject(index, object);

		if (!quader::document::is_finite(object.transform)) {
			add_fragment_diagnostic(result.diagnostics,
					IoSeverity::Error,
					IoDiagnosticCode::ValidationFailed,
					"imported mesh object has a non-finite transform",
					kSubject);
		}

		if (object.material_index && *object.material_index >= fragment.materials.size()) {
			add_fragment_diagnostic(result.diagnostics,
					IoSeverity::Error,
					IoDiagnosticCode::ValidationFailed,
					"imported mesh object references a missing material",
					kSubject);
		}

		const auto kMeshValidation = quader::mesh::validate_mesh(object.mesh);
		for (const auto &issue : kMeshValidation.issues()) {
			add_fragment_diagnostic(result.diagnostics,
					IoSeverity::Error,
					IoDiagnosticCode::MeshValidationFailed,
					issue.diagnostic.message,
					kSubject);
		}
	}

	return result;
}

quader::foundation::Result<quader::document::Document, IoError> build_document_from_import(
		ImportedDocument imported) {
	IoDiagnosticList diagnostics;
	append_import_diagnostics(imported, diagnostics);

	if (imported.root_fragment.mesh_objects.empty()) {
		diagnostics.add(IoDiagnostic{
				IoSeverity::Error,
				IoDiagnosticCode::ValidationFailed,
				"cannot build a document from an empty import",
				{},
				{},
				"document",
		});
	}

	auto validation = validate_document_fragment(imported.root_fragment);
	diagnostics.append(validation.diagnostics);
	if (diagnostics.has_errors()) {
		return quader::foundation::Result<quader::document::Document, IoError>::failure(
				validation_error_from_diagnostics(diagnostics));
	}

	quader::document::Document document;
	for (auto &object : imported.root_fragment.mesh_objects) {
		auto created = document.create_mesh_object(std::move(object.name),
				std::move(object.mesh),
				object.transform);
		if (!created) {
			diagnostics.add(IoDiagnostic{
					IoSeverity::Error,
					IoDiagnosticCode::DocumentValidationFailed,
					created.error().diagnostic.message,
					{},
					{},
					"document",
			});
			return quader::foundation::Result<quader::document::Document, IoError>::failure(
					validation_error_from_diagnostics(diagnostics));
		}
	}

	return quader::foundation::Result<quader::document::Document, IoError>::success(
			std::move(document));
}

} // namespace quader::io
