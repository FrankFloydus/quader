#include "../document/document_test_helpers.hpp"
#include "../mesh/mesh_corruption_fixtures.hpp"

#include <gtest/gtest.h>

#include "foundation/logging.hpp"
#include "io/export_service.hpp"
#include "io/import_service.hpp"

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace {

class ServiceFakeImporter final : public quader::io::IImporter {
public:
    ServiceFakeImporter(quader::io::FileFormatDescriptor descriptor, quader::io::DocumentFragment fragment)
        : descriptor_(std::move(descriptor))
        , fragment_(std::move(fragment))
    {
    }

    [[nodiscard]] quader::io::FileFormatDescriptor descriptor() const override
    {
        return descriptor_;
    }

    [[nodiscard]] bool can_import(const std::filesystem::path& path) const override
    {
        return quader::io::descriptor_supports_extension(descriptor_, path);
    }

    [[nodiscard]] quader::foundation::Result<quader::io::ImportResult, quader::io::IoError>
    import_file(const quader::io::ImportRequest&) const override
    {
        ++import_calls;
        quader::io::ImportResult result;
        result.payload = fragment_;
        result.diagnostics.add(quader::io::IoSeverity::Info,
                               quader::io::IoDiagnosticCode::Unknown,
                               "fake importer ran");
        return quader::foundation::Result<quader::io::ImportResult, quader::io::IoError>::success(
            std::move(result));
    }

    mutable int import_calls = 0;

private:
    quader::io::FileFormatDescriptor descriptor_;
    quader::io::DocumentFragment fragment_;
};

class ServiceFakeExporter final : public quader::io::IExporter {
public:
    explicit ServiceFakeExporter(quader::io::FileFormatDescriptor descriptor)
        : descriptor_(std::move(descriptor))
    {
    }

    [[nodiscard]] quader::io::FileFormatDescriptor descriptor() const override
    {
        return descriptor_;
    }

    [[nodiscard]] bool can_export(const std::filesystem::path& path) const override
    {
        return quader::io::descriptor_supports_extension(descriptor_, path);
    }

    [[nodiscard]] quader::foundation::Result<quader::io::ExportResult, quader::io::IoError>
    export_file(const quader::io::ExportRequest& request) const override
    {
        ++export_calls;
        saw_deterministic_default = request.options.deterministic;
        quader::io::ExportResult result;
        result.diagnostics.add(quader::io::IoSeverity::Info,
                               quader::io::IoDiagnosticCode::Unknown,
                               "fake exporter ran");
        return quader::foundation::Result<quader::io::ExportResult, quader::io::IoError>::success(
            std::move(result));
    }

    mutable int export_calls = 0;
    mutable bool saw_deterministic_default = false;

private:
    quader::io::FileFormatDescriptor descriptor_;
};

quader::io::FileFormatDescriptor import_format()
{
    return quader::io::FileFormatDescriptor{"fake",
                                            "Fake Import",
                                            {"fake"},
                                            true,
                                            true,
                                            false,
                                            false};
}

quader::io::FileFormatDescriptor export_format()
{
    return quader::io::FileFormatDescriptor{"fake-export",
                                            "Fake Export",
                                            {"out"},
                                            false,
                                            false,
                                            true,
                                            true};
}

quader::io::DocumentFragment valid_fragment()
{
    auto mesh = quader::tests::document_fixtures::make_triangle_mesh();
    quader::io::DocumentFragment fragment;
    fragment.mesh_objects.push_back(quader::io::ImportedMeshObject{
        "Triangle",
        std::move(mesh.mesh),
        {},
        std::nullopt,
    });
    return fragment;
}

quader::io::DocumentFragment invalid_mesh_fragment()
{
    quader::io::DocumentFragment fragment;
    fragment.mesh_objects.push_back(quader::io::ImportedMeshObject{
        "Invalid",
        quader::tests::mesh_fixtures::make_invalid_triangle_with_nonfinite_vertex_position(
            quader::math::Vec3{std::numeric_limits<float>::infinity(), 0.0F, 0.0F}),
        {},
        std::nullopt,
    });
    return fragment;
}

TEST(IoImportExportService, ImportServiceRejectsUnsupportedExtensionsBeforeImporterExecution)
{
    quader::io::ImportExportRegistry registry;
    auto importer = std::make_unique<ServiceFakeImporter>(import_format(), valid_fragment());
    const auto* importer_ptr = importer.get();
    EXPECT_TRUE(registry.register_importer(std::move(importer)));

    quader::io::ImportService service{registry};
    const auto result = service.import_file(quader::io::ImportRequest{"scene.unknown", {}});
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code, quader::io::IoErrorCode::UnsupportedFormat);
    EXPECT_EQ(importer_ptr->import_calls, 0);
}

TEST(IoImportExportService, ImportServiceValidatesSuccessfulPayloadsAndPreservesDiagnostics)
{
    quader::io::ImportExportRegistry registry;
    auto importer = std::make_unique<ServiceFakeImporter>(import_format(), valid_fragment());
    const auto* importer_ptr = importer.get();
    EXPECT_TRUE(registry.register_importer(std::move(importer)));

    quader::io::ImportService service{registry};
    const auto result = service.import_file(quader::io::ImportRequest{"scene.FAKE", {}});
    EXPECT_TRUE(result);
    EXPECT_EQ(importer_ptr->import_calls, 1);
    EXPECT_FALSE(result.value().diagnostics.has_errors());
    EXPECT_EQ(result.value().diagnostics.size(), 1U);
}

TEST(IoImportExportService, ImportServiceRejectsInvalidMeshPayloads)
{
    quader::io::ImportExportRegistry registry;
    auto importer = std::make_unique<ServiceFakeImporter>(import_format(), invalid_mesh_fragment());
    const auto* importer_ptr = importer.get();
    EXPECT_TRUE(registry.register_importer(std::move(importer)));

    quader::io::ImportService service{registry};
    const auto result = service.import_file(quader::io::ImportRequest{"scene.fake", {}});
    EXPECT_FALSE(result);
    EXPECT_EQ(importer_ptr->import_calls, 1);
    EXPECT_EQ(result.error().code, quader::io::IoErrorCode::ValidationFailed);
    EXPECT_TRUE(result.error().related.has_errors());
}

TEST(IoImportExportService, ExportServiceRejectsUnsupportedExtensionsBeforeExporterExecution)
{
    quader::io::ImportExportRegistry registry;
    auto exporter = std::make_unique<ServiceFakeExporter>(export_format());
    const auto* exporter_ptr = exporter.get();
    EXPECT_TRUE(registry.register_exporter(std::move(exporter)));

    quader::document::Document document;
    quader::io::ExportService service{registry};
    const auto result = service.export_file(quader::io::ExportRequest{"scene.unknown", document, {}});
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code, quader::io::IoErrorCode::UnsupportedFormat);
    EXPECT_EQ(exporter_ptr->export_calls, 0);
}

TEST(IoImportExportService, ExportServiceUsesRegisteredExporterAndDeterministicDefault)
{
    quader::io::ImportExportRegistry registry;
    auto exporter = std::make_unique<ServiceFakeExporter>(export_format());
    const auto* exporter_ptr = exporter.get();
    EXPECT_TRUE(registry.register_exporter(std::move(exporter)));

    quader::document::Document document;
    quader::io::ExportService service{registry};
    const auto result = service.export_file(quader::io::ExportRequest{"scene.OUT", document, {}});
    EXPECT_TRUE(result);
    EXPECT_EQ(exporter_ptr->export_calls, 1);
    EXPECT_TRUE(exporter_ptr->saw_deterministic_default);
    EXPECT_FALSE(result.value().diagnostics.has_errors());
}

} // namespace
