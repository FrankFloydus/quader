#include "io/import_export_registry.hpp"

#include <set>
#include <string>
#include <utility>

namespace quader::io {
namespace {

[[nodiscard]] IoError registry_validation_error(std::string message, std::string subject = {})
{
    return make_io_error(IoErrorCode::ValidationFailed,
                         IoDiagnosticCode::ValidationFailed,
                         std::move(message),
                         {},
                         std::move(subject));
}

[[nodiscard]] quader::foundation::Result<void, IoError> validate_descriptor_extensions(
    const FileFormatDescriptor& descriptor)
{
    std::set<std::string> normalized_extensions;
    for (const auto& extension : descriptor.extensions) {
        const auto normalized = normalize_extension(extension);
        if (normalized.empty()) {
            return quader::foundation::Result<void, IoError>::failure(
                registry_validation_error("file format extension cannot be empty", descriptor.id));
        }

        const auto [_, inserted] = normalized_extensions.insert(normalized);
        if (!inserted) {
            return quader::foundation::Result<void, IoError>::failure(
                registry_validation_error("file format descriptor repeats extension '" + normalized + "'",
                                          descriptor.id));
        }
    }

    return quader::foundation::Result<void, IoError>::success();
}

template <class Interface>
[[nodiscard]] quader::foundation::Result<void, IoError> validate_unique_registration(
    const std::vector<std::unique_ptr<Interface>>& existing,
    const FileFormatDescriptor& descriptor)
{
    if (descriptor.id.empty()) {
        return quader::foundation::Result<void, IoError>::failure(
            registry_validation_error("file format id cannot be empty"));
    }

    auto extension_validation = validate_descriptor_extensions(descriptor);
    if (!extension_validation) {
        return extension_validation;
    }

    for (const auto& registered : existing) {
        const auto registered_descriptor = registered->descriptor();
        if (registered_descriptor.id == descriptor.id) {
            return quader::foundation::Result<void, IoError>::failure(
                registry_validation_error("duplicate file format id '" + descriptor.id + "'",
                                          descriptor.id));
        }

        for (const auto& registered_extension : registered_descriptor.extensions) {
            const auto normalized_registered = normalize_extension(registered_extension);
            for (const auto& candidate_extension : descriptor.extensions) {
                const auto normalized_candidate = normalize_extension(candidate_extension);
                if (normalized_registered == normalized_candidate) {
                    return quader::foundation::Result<void, IoError>::failure(
                        registry_validation_error(
                            "duplicate file extension claim '" + normalized_candidate + "'",
                            descriptor.id));
                }
            }
        }
    }

    return quader::foundation::Result<void, IoError>::success();
}

} // namespace

quader::foundation::Result<void, IoError> ImportExportRegistry::register_importer(
    std::unique_ptr<IImporter> importer)
{
    if (!importer) {
        return quader::foundation::Result<void, IoError>::failure(
            registry_validation_error("cannot register a null importer"));
    }

    const auto descriptor = importer->descriptor();
    auto validation = validate_unique_registration(importers_, descriptor);
    if (!validation) {
        return validation;
    }

    importers_.push_back(std::move(importer));
    return quader::foundation::Result<void, IoError>::success();
}

quader::foundation::Result<void, IoError> ImportExportRegistry::register_exporter(
    std::unique_ptr<IExporter> exporter)
{
    if (!exporter) {
        return quader::foundation::Result<void, IoError>::failure(
            registry_validation_error("cannot register a null exporter"));
    }

    const auto descriptor = exporter->descriptor();
    auto validation = validate_unique_registration(exporters_, descriptor);
    if (!validation) {
        return validation;
    }

    exporters_.push_back(std::move(exporter));
    return quader::foundation::Result<void, IoError>::success();
}

const IImporter* ImportExportRegistry::importer_for_path(const std::filesystem::path& path) const
{
    for (const auto& importer : importers_) {
        const auto descriptor = importer->descriptor();
        if (descriptor_supports_extension(descriptor, path) && importer->can_import(path)) {
            return importer.get();
        }
    }

    return nullptr;
}

const IExporter* ImportExportRegistry::exporter_for_path(const std::filesystem::path& path) const
{
    for (const auto& exporter : exporters_) {
        const auto descriptor = exporter->descriptor();
        if (descriptor_supports_extension(descriptor, path) && exporter->can_export(path)) {
            return exporter.get();
        }
    }

    return nullptr;
}

std::vector<FileFormatDescriptor> ImportExportRegistry::import_formats() const
{
    std::vector<FileFormatDescriptor> formats;
    formats.reserve(importers_.size());
    for (const auto& importer : importers_) {
        formats.push_back(importer->descriptor());
    }

    return formats;
}

std::vector<FileFormatDescriptor> ImportExportRegistry::export_formats() const
{
    std::vector<FileFormatDescriptor> formats;
    formats.reserve(exporters_.size());
    for (const auto& exporter : exporters_) {
        formats.push_back(exporter->descriptor());
    }

    return formats;
}

} // namespace quader::io
