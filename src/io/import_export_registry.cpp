#include "io/import_export_registry.hpp"

#include <set>
#include <string>
#include <utility>

namespace quader::io {
namespace {

[[nodiscard]] IoError registry_validation_error(std::string message, std::string subject = {}) {
	return make_io_error(IoErrorCode::ValidationFailed,
			IoDiagnosticCode::ValidationFailed,
			std::move(message),
			{},
			std::move(subject));
}

[[nodiscard]] quader::foundation::Result<void, IoError> validate_descriptor_extensions(
		const FileFormatDescriptor &descriptor) {
	std::set<std::string> normalized_extensions;
	for (const auto &extension : descriptor.extensions) {
		const auto kNormalized = normalize_extension(extension);
		if (kNormalized.empty()) {
			return quader::foundation::Result<void, IoError>::failure(
					registry_validation_error("file format extension cannot be empty", descriptor.id));
		}

		const auto [_, inserted] = normalized_extensions.insert(kNormalized);
		if (!inserted) {
			return quader::foundation::Result<void, IoError>::failure(
					registry_validation_error("file format descriptor repeats extension '" + kNormalized + "'",
							descriptor.id));
		}
	}

	return quader::foundation::Result<void, IoError>::success();
}

template <class Interface>
[[nodiscard]] quader::foundation::Result<void, IoError> validate_unique_registration(
		const std::vector<std::unique_ptr<Interface>> &existing,
		const FileFormatDescriptor &descriptor) {
	if (descriptor.id.empty()) {
		return quader::foundation::Result<void, IoError>::failure(
				registry_validation_error("file format id cannot be empty"));
	}

	auto extension_validation = validate_descriptor_extensions(descriptor);
	if (!extension_validation) {
		return extension_validation;
	}

	for (const auto &registered : existing) {
		const auto kRegisteredDescriptor = registered->descriptor();
		if (kRegisteredDescriptor.id == descriptor.id) {
			return quader::foundation::Result<void, IoError>::failure(
					registry_validation_error("duplicate file format id '" + descriptor.id + "'",
							descriptor.id));
		}

		for (const auto &registered_extension : kRegisteredDescriptor.extensions) {
			const auto kNormalizedRegistered = normalize_extension(registered_extension);
			for (const auto &candidate_extension : descriptor.extensions) {
				const auto kNormalizedCandidate = normalize_extension(candidate_extension);
				if (kNormalizedRegistered == kNormalizedCandidate) {
					return quader::foundation::Result<void, IoError>::failure(
							registry_validation_error(
									"duplicate file extension claim '" + kNormalizedCandidate + "'",
									descriptor.id));
				}
			}
		}
	}

	return quader::foundation::Result<void, IoError>::success();
}

} // namespace

quader::foundation::Result<void, IoError> ImportExportRegistry::register_importer(
		std::unique_ptr<IImporter> importer) {
	if (!importer) {
		return quader::foundation::Result<void, IoError>::failure(
				registry_validation_error("cannot register a null importer"));
	}

	const auto kDescriptor = importer->descriptor();
	auto validation = validate_unique_registration(importers_, kDescriptor);
	if (!validation) {
		return validation;
	}

	importers_.push_back(std::move(importer));
	return quader::foundation::Result<void, IoError>::success();
}

quader::foundation::Result<void, IoError> ImportExportRegistry::register_exporter(
		std::unique_ptr<IExporter> exporter) {
	if (!exporter) {
		return quader::foundation::Result<void, IoError>::failure(
				registry_validation_error("cannot register a null exporter"));
	}

	const auto kDescriptor = exporter->descriptor();
	auto validation = validate_unique_registration(exporters_, kDescriptor);
	if (!validation) {
		return validation;
	}

	exporters_.push_back(std::move(exporter));
	return quader::foundation::Result<void, IoError>::success();
}

const IImporter *ImportExportRegistry::importer_for_path(const std::filesystem::path &path) const {
	for (const auto &importer : importers_) {
		const auto kDescriptor = importer->descriptor();
		if (descriptor_supports_extension(kDescriptor, path) && importer->can_import(path)) {
			return importer.get();
		}
	}

	return nullptr;
}

const IExporter *ImportExportRegistry::exporter_for_path(const std::filesystem::path &path) const {
	for (const auto &exporter : exporters_) {
		const auto kDescriptor = exporter->descriptor();
		if (descriptor_supports_extension(kDescriptor, path) && exporter->can_export(path)) {
			return exporter.get();
		}
	}

	return nullptr;
}

std::vector<FileFormatDescriptor> ImportExportRegistry::import_formats() const {
	std::vector<FileFormatDescriptor> formats;
	formats.reserve(importers_.size());
	for (const auto &importer : importers_) {
		formats.push_back(importer->descriptor());
	}

	return formats;
}

std::vector<FileFormatDescriptor> ImportExportRegistry::export_formats() const {
	std::vector<FileFormatDescriptor> formats;
	formats.reserve(exporters_.size());
	for (const auto &exporter : exporters_) {
		formats.push_back(exporter->descriptor());
	}

	return formats;
}

} // namespace quader::io
