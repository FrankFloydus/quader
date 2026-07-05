#include "../document/document_test_helpers.hpp"
#include "../mesh/mesh_corruption_fixtures.hpp"

#include <gtest/gtest.h>

#include "foundation/logging.hpp"
#include "io/document_fragment_validator.hpp"
#include "mesh/core/mesh_validation.hpp"

#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace {

quader::io::DocumentFragment make_valid_fragment() {
	auto mesh = quader::tests::document_fixtures::make_triangle_mesh();
	quader::io::DocumentFragment fragment;
	fragment.materials.push_back(quader::io::ImportedMaterialMetadata{
			"Default",
			"test",
			"0",
			{},
			{},
	});
	fragment.mesh_objects.push_back(quader::io::ImportedMeshObject{
			"Triangle",
			std::move(mesh.mesh),
			{},
			0U,
	});
	return fragment;
}

TEST(IoDocumentFragment, FragmentValidationAcceptsValidMeshObjectsAndWarnsForEmptyFragments) {
	const auto kValid = quader::io::validate_document_fragment(make_valid_fragment());
	EXPECT_TRUE(kValid.ok());
	EXPECT_TRUE(kValid.diagnostics.empty());

	const auto kEmpty = quader::io::validate_document_fragment(quader::io::DocumentFragment{});
	EXPECT_TRUE(kEmpty.ok());
	EXPECT_FALSE(kEmpty.diagnostics.empty());
	EXPECT_EQ(kEmpty.diagnostics[0].severity, quader::io::IoSeverity::Warning);
}

TEST(IoDocumentFragment, FragmentValidationReportsAllPracticalObjectLevelErrors) {
	quader::document::Transform invalid_transform;
	invalid_transform.scale.z = std::numeric_limits<float>::infinity();

	quader::io::DocumentFragment fragment;
	fragment.mesh_objects.push_back(quader::io::ImportedMeshObject{
			"Broken",
			quader::tests::mesh_fixtures::make_invalid_triangle_with_nonfinite_vertex_position(
					quader::math::Vec3{ std::numeric_limits<float>::infinity(), 0.0F, 0.0F }),
			invalid_transform,
			3U,
	});

	const auto kValidation = quader::io::validate_document_fragment(fragment);
	EXPECT_FALSE(kValidation.ok());
	EXPECT_TRUE(kValidation.diagnostics.has_errors());
	EXPECT_TRUE(kValidation.diagnostics.size() >= 3U);
}

TEST(IoDocumentFragment, BuildDocumentFromImportUsesDocumentValidationGate) {
	quader::io::ImportedDocument imported;
	imported.source_name = "scene.fake";
	imported.root_fragment = make_valid_fragment();

	auto built = quader::io::build_document_from_import(std::move(imported));
	EXPECT_TRUE(built);
	EXPECT_EQ(built.value().object_count(), 1U);

	const auto kObjectId = built.value().object_ids().front();
	const auto *object = built.value().find_mesh_object(kObjectId);
	EXPECT_TRUE(object != nullptr);
	EXPECT_EQ(object->name, std::string("Triangle"));
	EXPECT_TRUE(quader::mesh::validate_mesh(object->mesh).ok());
}

TEST(IoDocumentFragment, BuildDocumentFromImportRejectsInvalidImportWithoutReturningPartialDocument) {
	auto valid_mesh = quader::tests::document_fixtures::make_triangle_mesh();

	quader::io::ImportedDocument imported;
	imported.source_name = "scene.fake";
	imported.root_fragment.mesh_objects.push_back(quader::io::ImportedMeshObject{
			"Valid",
			std::move(valid_mesh.mesh),
			{},
			std::nullopt,
	});
	imported.root_fragment.mesh_objects.push_back(quader::io::ImportedMeshObject{
			"Invalid",
			quader::tests::mesh_fixtures::make_invalid_triangle_with_nonfinite_vertex_position(
					quader::math::Vec3{ 0.0F, std::numeric_limits<float>::infinity(), 0.0F }),
			{},
			std::nullopt,
	});

	const auto kBuilt = quader::io::build_document_from_import(std::move(imported));
	EXPECT_FALSE(kBuilt);
	EXPECT_EQ(kBuilt.error().code, quader::io::IoErrorCode::ValidationFailed);
	EXPECT_TRUE(kBuilt.error().related.has_errors());
}

TEST(IoDocumentFragment, BuildDocumentFromImportRejectsEmptyDocumentReplacement) {
	quader::io::ImportedDocument imported;
	imported.source_name = "empty.fake";

	const auto kBuilt = quader::io::build_document_from_import(std::move(imported));
	EXPECT_FALSE(kBuilt);
	EXPECT_EQ(kBuilt.error().code, quader::io::IoErrorCode::ValidationFailed);
}

} // namespace
