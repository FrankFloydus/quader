#include "foundation/logging.hpp"
#include "io/import_export_registry.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

namespace {

class FakeImporter final : public quader::io::IImporter {
public:
    explicit FakeImporter(quader::io::FileFormatDescriptor descriptor)
        : descriptor_(std::move(descriptor))
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
        return quader::foundation::Result<quader::io::ImportResult, quader::io::IoError>::success(
            quader::io::ImportResult{quader::io::DocumentFragment{}, {}});
    }

private:
    quader::io::FileFormatDescriptor descriptor_;
};

class FakeExporter final : public quader::io::IExporter {
public:
    explicit FakeExporter(quader::io::FileFormatDescriptor descriptor)
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
    export_file(const quader::io::ExportRequest&) const override
    {
        return quader::foundation::Result<quader::io::ExportResult, quader::io::IoError>::success(
            quader::io::ExportResult{});
    }

private:
    quader::io::FileFormatDescriptor descriptor_;
};

quader::io::FileFormatDescriptor importer_descriptor(std::string id,
                                                     std::string extension,
                                                     std::string display_name = {})
{
    return quader::io::FileFormatDescriptor{
        std::move(id),
        display_name.empty() ? "Test Format" : std::move(display_name),
        {std::move(extension)},
        true,
        true,
        false,
        false,
    };
}

quader::io::FileFormatDescriptor exporter_descriptor(std::string id, std::string extension)
{
    return quader::io::FileFormatDescriptor{
        std::move(id),
        "Test Export",
        {std::move(extension)},
        false,
        false,
        true,
        true,
    };
}

TEST(IoRegistry, RegistryPreservesRegistrationOrderAndMatchesExtensionsCaseInsensitively)
{
    quader::io::ImportExportRegistry registry;
    auto first = std::make_unique<FakeImporter>(importer_descriptor("first", "qdr", "First"));
    const auto* first_ptr = first.get();
    auto second = std::make_unique<FakeImporter>(importer_descriptor("second", "obj", "Second"));
    const auto* second_ptr = second.get();

    EXPECT_TRUE(registry.register_importer(std::move(first)));
    EXPECT_TRUE(registry.register_importer(std::move(second)));

    const auto formats = registry.import_formats();
    EXPECT_EQ(formats.size(), 2U);
    EXPECT_EQ(formats[0].id, std::string("first"));
    EXPECT_EQ(formats[1].id, std::string("second"));
    EXPECT_TRUE(registry.importer_for_path("MODEL.QDR") == first_ptr);
    EXPECT_TRUE(registry.importer_for_path("mesh.Obj") == second_ptr);
    EXPECT_TRUE(registry.importer_for_path("mesh.unknown") == nullptr);
}

TEST(IoRegistry, RegistryRejectsDuplicateImporterIdsAndExtensions)
{
    quader::io::ImportExportRegistry registry;
    EXPECT_TRUE(registry.register_importer(
        std::make_unique<FakeImporter>(importer_descriptor("mesh", "mesh"))));

    const auto duplicate_id = registry.register_importer(
        std::make_unique<FakeImporter>(importer_descriptor("mesh", "other")));
    EXPECT_FALSE(duplicate_id);
    EXPECT_EQ(duplicate_id.error().code, quader::io::IoErrorCode::ValidationFailed);

    const auto duplicate_extension = registry.register_importer(
        std::make_unique<FakeImporter>(importer_descriptor("other", "MESH")));
    EXPECT_FALSE(duplicate_extension);
    EXPECT_EQ(duplicate_extension.error().diagnostic.code,
              quader::io::IoDiagnosticCode::ValidationFailed);
}

TEST(IoRegistry, RegistryRejectsNullAndDuplicateExporterRegistrations)
{
    quader::io::ImportExportRegistry registry;
    std::unique_ptr<quader::io::IExporter> null_exporter;
    const auto null_result = registry.register_exporter(std::move(null_exporter));
    EXPECT_FALSE(null_result);

    EXPECT_TRUE(registry.register_exporter(
        std::make_unique<FakeExporter>(exporter_descriptor("scene", "qscene"))));
    const auto duplicate = registry.register_exporter(
        std::make_unique<FakeExporter>(exporter_descriptor("other", ".QSCENE")));
    EXPECT_FALSE(duplicate);

    const auto formats = registry.export_formats();
    EXPECT_EQ(formats.size(), 1U);
    EXPECT_EQ(formats[0].id, std::string("scene"));
}

} // namespace
