#include "foundation/logging.hpp"
#include "mesh/core/mesh_attributes.hpp"
#include "mesh/core/mesh_traversal.hpp"
#include "mesh/core/mesh_validation.hpp"
#include "mesh_corruption_fixtures.hpp"
#include "mesh_fixtures.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

bool contains_issue(const quader::mesh::MeshValidationReport &report,
		quader::mesh::MeshValidationCode code) {
	for (const auto &issue : report.issues()) {
		if (issue.code == code) {
			return true;
		}
	}

	return false;
}

quader::mesh::HalfedgeId first_face_halfedge(const quader::mesh::Polyhedron &mesh,
		quader::mesh::FaceId face) {
	auto halfedges = mesh.face_halfedges(face);
	EXPECT_TRUE(halfedges);
	return halfedges.value().front();
}

quader::mesh::HalfedgeId first_boundary_halfedge(const quader::mesh::Polyhedron &mesh,
		quader::mesh::FaceId face) {
	const auto kInterior = first_face_halfedge(mesh, face);
	auto opposite = mesh.opposite_halfedge(kInterior);
	EXPECT_TRUE(opposite);
	return opposite.value();
}

bool is_same_cyclic_vertex_order(const std::vector<quader::mesh::VertexId> &actual,
		const std::array<quader::mesh::VertexId, 3> &expected) {
	if (actual.size() != expected.size()) {
		return false;
	}

	for (std::size_t offset = 0; offset < expected.size(); ++offset) {
		bool matches = true;
		for (std::size_t index = 0; index < expected.size(); ++index) {
			if (actual[(offset + index) % expected.size()] != expected[index]) {
				matches = false;
				break;
			}
		}
		if (matches) {
			return true;
		}
	}
	return false;
}

TEST(MeshCore, MeshIdsDefaultToInvalidAndRejectInvalidPrimitives) {
	quader::mesh::Polyhedron mesh;

	const quader::mesh::VertexId kInvalidVertex;
	const quader::mesh::FaceId kInvalidFace;
	EXPECT_FALSE(kInvalidVertex.is_valid());
	EXPECT_FALSE(kInvalidFace.is_valid());
	EXPECT_FALSE(mesh.delete_vertex(kInvalidVertex));

	const std::array<quader::mesh::VertexId, 3> kInvalidFaceVertices{
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 0.0F, 0.0F }),
		quader::mesh::VertexId{},
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 1.0F, 0.0F }),
	};
	EXPECT_FALSE(mesh.create_face(kInvalidFaceVertices));
}

TEST(MeshCore, SingleTriangleHasValidTopologyAndBoundaryPlaceholders) {
	auto fixture = quader::tests::mesh_fixtures::make_single_triangle();

	EXPECT_EQ(fixture.mesh.vertex_count(), 3U);
	EXPECT_EQ(fixture.mesh.face_count(), 1U);
	EXPECT_EQ(fixture.mesh.edge_count(), 3U);
	EXPECT_EQ(fixture.mesh.halfedge_count(), 6U);

	const auto kReport = quader::mesh::validate_mesh(fixture.mesh);
	EXPECT_TRUE(kReport.ok());

	auto face_vertices = quader::mesh::face_vertices(fixture.mesh, fixture.face);
	EXPECT_TRUE(face_vertices);
	EXPECT_EQ(face_vertices.value().size(), 3U);
	EXPECT_TRUE(is_same_cyclic_vertex_order(face_vertices.value(), fixture.vertices));

	auto halfedges = fixture.mesh.face_halfedges(fixture.face);
	EXPECT_TRUE(halfedges);
	for (const auto kHalfedge : halfedges.value()) {
		auto interior_boundary = fixture.mesh.is_boundary_halfedge(kHalfedge);
		EXPECT_TRUE(interior_boundary);
		EXPECT_FALSE(interior_boundary.value());

		auto opposite = fixture.mesh.opposite_halfedge(kHalfedge);
		EXPECT_TRUE(opposite);

		auto boundary = fixture.mesh.is_boundary_halfedge(opposite.value());
		EXPECT_TRUE(boundary);
		EXPECT_TRUE(boundary.value());
		EXPECT_FALSE(fixture.mesh.halfedge_face(opposite.value()));
		EXPECT_FALSE(fixture.mesh.next_halfedge(opposite.value()));
		EXPECT_FALSE(fixture.mesh.previous_halfedge(opposite.value()));

		auto boundary_opposite = fixture.mesh.opposite_halfedge(opposite.value());
		EXPECT_TRUE(boundary_opposite);
		EXPECT_EQ(boundary_opposite.value(), kHalfedge);
	}
}

TEST(MeshCore, DeleteFaceInvalidatesTopologyIdsAndReusesVertexGeneration) {
	auto fixture = quader::tests::mesh_fixtures::make_single_triangle();
	const auto kFace = fixture.face;
	const auto kHalfedge = first_face_halfedge(fixture.mesh, kFace);
	auto edge = fixture.mesh.halfedge_edge(kHalfedge);
	ASSERT_TRUE(edge);

	auto delete_referenced_vertex = fixture.mesh.delete_vertex(fixture.vertices[0]);
	EXPECT_FALSE(delete_referenced_vertex);
	EXPECT_EQ(delete_referenced_vertex.error().code, quader::mesh::MeshErrorCode::VertexStillReferenced);

	auto deleted_face = fixture.mesh.delete_face(kFace);
	EXPECT_TRUE(deleted_face);
	EXPECT_FALSE(fixture.mesh.is_valid(kFace));
	EXPECT_FALSE(fixture.mesh.is_valid(kHalfedge));
	EXPECT_FALSE(fixture.mesh.is_valid(edge.value()));
	EXPECT_EQ(fixture.mesh.face_count(), 0U);
	EXPECT_EQ(fixture.mesh.edge_count(), 0U);
	EXPECT_EQ(fixture.mesh.halfedge_count(), 0U);

	auto deleted_vertex = fixture.mesh.delete_vertex(fixture.vertices[0]);
	EXPECT_TRUE(deleted_vertex);
	EXPECT_FALSE(fixture.mesh.is_valid(fixture.vertices[0]));

	const auto kReusedVertex = fixture.mesh.create_vertex(quader::math::Vec3{ 2.0F, 0.0F, 0.0F });
	EXPECT_EQ(kReusedVertex.index(), fixture.vertices[0].index());
	EXPECT_NE(kReusedVertex.generation(), fixture.vertices[0].generation());
	EXPECT_TRUE(fixture.mesh.is_valid(kReusedVertex));
}

TEST(MeshCore, ValidationReportsBackendAndMappingCorruption) {
	auto missing_backend_fixture = quader::tests::mesh_fixtures::make_single_triangle();
	quader::tests::mesh_fixtures::MeshCorruptionAccess::remove_vertex_backend_handle(
			missing_backend_fixture.mesh,
			missing_backend_fixture.vertices[0]);
	const auto kMissingBackendReport = quader::mesh::validate_mesh(missing_backend_fixture.mesh);
	EXPECT_FALSE(kMissingBackendReport.ok());
	EXPECT_TRUE(contains_issue(kMissingBackendReport, quader::mesh::MeshValidationCode::MissingBackendHandle));

	auto missing_quader_fixture = quader::tests::mesh_fixtures::make_single_triangle();
	const auto kOriginHalfedge = first_face_halfedge(missing_quader_fixture.mesh, missing_quader_fixture.face);
	quader::tests::mesh_fixtures::MeshCorruptionAccess::break_halfedge_origin_mapping(
			missing_quader_fixture.mesh,
			kOriginHalfedge);
	const auto kMissingQuaderReport = quader::mesh::validate_mesh(missing_quader_fixture.mesh);
	EXPECT_FALSE(kMissingQuaderReport.ok());
	EXPECT_TRUE(contains_issue(kMissingQuaderReport, quader::mesh::MeshValidationCode::MissingQuaderId));

	auto reverse_fixture = quader::tests::mesh_fixtures::make_single_triangle();
	const auto kHalfedge = first_face_halfedge(reverse_fixture.mesh, reverse_fixture.face);
	quader::tests::mesh_fixtures::MeshCorruptionAccess::break_halfedge_reverse_mapping(
			reverse_fixture.mesh,
			kHalfedge);
	const auto kReverseReport = quader::mesh::validate_mesh(reverse_fixture.mesh);
	EXPECT_FALSE(kReverseReport.ok());
	EXPECT_TRUE(contains_issue(kReverseReport, quader::mesh::MeshValidationCode::ReverseMappingMismatch));
}

TEST(MeshCore, ValidationReportsFaceBackendCorruption) {
	auto fixture = quader::tests::mesh_fixtures::make_single_triangle();
	quader::tests::mesh_fixtures::MeshCorruptionAccess::remove_face_backend_handle(
			fixture.mesh,
			fixture.face);

	const auto kReport = quader::mesh::validate_mesh(fixture.mesh);
	EXPECT_FALSE(kReport.ok());
	EXPECT_TRUE(contains_issue(kReport, quader::mesh::MeshValidationCode::MissingBackendHandle));
}

TEST(MeshCore, VertexPositionQueriesAndMutationRejectNonFinitePositions) {
	quader::mesh::Polyhedron mesh;
	const auto kVertex = mesh.create_vertex(quader::math::Vec3{ 0.0F, 0.0F, 0.0F });

	auto position = mesh.vertex_position(kVertex);
	EXPECT_TRUE(position);
	EXPECT_TRUE(quader::math::nearly_equal(position.value(), quader::math::Vec3{ 0.0F, 0.0F, 0.0F }));

	auto moved = mesh.set_vertex_position(kVertex, quader::math::Vec3{ 2.0F, 3.0F, 4.0F });
	EXPECT_TRUE(moved);
	auto moved_position = mesh.vertex_position(kVertex);
	EXPECT_TRUE(moved_position);
	EXPECT_TRUE(quader::math::nearly_equal(moved_position.value(), quader::math::Vec3{ 2.0F, 3.0F, 4.0F }));

	auto rejected = mesh.set_vertex_position(
			kVertex,
			quader::math::Vec3{ std::numeric_limits<float>::infinity(), 3.0F, 4.0F });
	EXPECT_FALSE(rejected);
	EXPECT_EQ(rejected.error().code, quader::mesh::MeshErrorCode::NonFinitePosition);

	auto unchanged_position = mesh.vertex_position(kVertex);
	EXPECT_TRUE(unchanged_position);
	EXPECT_TRUE(quader::math::nearly_equal(unchanged_position.value(), quader::math::Vec3{ 2.0F, 3.0F, 4.0F }));
}

TEST(MeshCore, CreateFaceRejectsNonFiniteVertexPositions) {
	quader::mesh::Polyhedron mesh;
	const std::array<quader::mesh::VertexId, 3> kVertices{
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 0.0F, 0.0F }),
		mesh.create_vertex(quader::math::Vec3{ std::numeric_limits<float>::infinity(), 0.0F, 0.0F }),
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 1.0F, 0.0F }),
	};

	const auto kFace = mesh.create_face(kVertices);

	EXPECT_FALSE(kFace);
	EXPECT_EQ(kFace.error().code, quader::mesh::MeshErrorCode::NonFinitePosition);
	EXPECT_EQ(mesh.face_count(), 0U);

	const auto kReport = quader::mesh::validate_mesh(mesh);
	EXPECT_FALSE(kReport.ok());
	EXPECT_TRUE(contains_issue(kReport, quader::mesh::MeshValidationCode::NonFinitePosition));
}

TEST(MeshCore, CreateFaceRejectsDegenerateFinitePositions) {
	quader::mesh::Polyhedron mesh;
	const std::array<quader::mesh::VertexId, 3> kVertices{
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 0.0F, 0.0F }),
		mesh.create_vertex(quader::math::Vec3{ 1.0F, 0.0F, 0.0F }),
		mesh.create_vertex(quader::math::Vec3{ 2.0F, 0.0F, 0.0F }),
	};

	const auto kFace = mesh.create_face(kVertices);

	EXPECT_FALSE(kFace);
	EXPECT_EQ(kFace.error().code, quader::mesh::MeshErrorCode::DegenerateFace);
	EXPECT_EQ(mesh.face_count(), 0U);
	EXPECT_TRUE(quader::mesh::validate_mesh(mesh).ok());
}

TEST(MeshCore, CreateFaceRejectsInvalidAndDuplicateVerticesWithoutPartialTopology) {
	quader::mesh::Polyhedron mesh;
	const std::array<quader::mesh::VertexId, 3> kVertices{
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 0.0F, 0.0F }),
		mesh.create_vertex(quader::math::Vec3{ 1.0F, 0.0F, 0.0F }),
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 1.0F, 0.0F }),
	};

	const std::array<quader::mesh::VertexId, 3> kInvalidVertices{
		kVertices[0],
		quader::mesh::VertexId{},
		kVertices[2],
	};
	const auto kInvalidFace = mesh.create_face(kInvalidVertices);
	EXPECT_FALSE(kInvalidFace);
	EXPECT_EQ(kInvalidFace.error().code, quader::mesh::MeshErrorCode::InvalidVertex);

	const std::array<quader::mesh::VertexId, 3> kDuplicateVertices{
		kVertices[0],
		kVertices[1],
		kVertices[1],
	};
	const auto kDuplicateFace = mesh.create_face(kDuplicateVertices);
	EXPECT_FALSE(kDuplicateFace);
	EXPECT_EQ(kDuplicateFace.error().code, quader::mesh::MeshErrorCode::DegenerateFace);

	EXPECT_EQ(mesh.vertex_count(), 3U);
	EXPECT_EQ(mesh.face_count(), 0U);
	EXPECT_EQ(mesh.edge_count(), 0U);
	EXPECT_EQ(mesh.halfedge_count(), 0U);
	EXPECT_TRUE(quader::mesh::validate_mesh(mesh).ok());
}

TEST(MeshCore, CopyAndMovePreserveIdsAttributesTopologyAndValidation) {
	auto fixture = quader::tests::mesh_fixtures::make_single_triangle();
	auto weight_attribute = fixture.mesh.attributes().create<float>(
			quader::mesh::AttributeDomain::Vertex,
			"weight",
			1.0F);
	ASSERT_TRUE(weight_attribute);

	auto weight = fixture.mesh.vertex_attribute<float>(weight_attribute.value(), fixture.vertices[1]);
	ASSERT_TRUE(weight);
	*weight.value() = 7.5F;

	quader::mesh::Polyhedron copied = fixture.mesh;
	EXPECT_TRUE(copied.is_valid(fixture.vertices[1]));
	EXPECT_TRUE(copied.is_valid(fixture.face));
	EXPECT_EQ(copied.vertex_count(), 3U);
	EXPECT_EQ(copied.edge_count(), 3U);
	EXPECT_EQ(copied.halfedge_count(), 6U);
	EXPECT_EQ(copied.face_count(), 1U);
	auto copied_weight = copied.vertex_attribute<float>(weight_attribute.value(), fixture.vertices[1]);
	ASSERT_TRUE(copied_weight);
	EXPECT_EQ(*copied_weight.value(), 7.5F);
	EXPECT_TRUE(quader::mesh::validate_mesh(copied).ok());

	quader::mesh::Polyhedron moved = std::move(copied);
	EXPECT_TRUE(moved.is_valid(fixture.vertices[1]));
	EXPECT_TRUE(moved.is_valid(fixture.face));
	auto moved_weight = moved.vertex_attribute<float>(weight_attribute.value(), fixture.vertices[1]);
	ASSERT_TRUE(moved_weight);
	EXPECT_EQ(*moved_weight.value(), 7.5F);
	EXPECT_TRUE(quader::mesh::validate_mesh(moved).ok());
}

TEST(MeshCore, CompactStoragePreservesLiveIdsAndAttributes) {
	auto fixture = quader::tests::mesh_fixtures::make_single_triangle();
	auto weight_attribute = fixture.mesh.attributes().create<float>(
			quader::mesh::AttributeDomain::Vertex,
			"weight",
			1.0F);
	ASSERT_TRUE(weight_attribute);
	auto weight = fixture.mesh.vertex_attribute<float>(weight_attribute.value(), fixture.vertices[2]);
	ASSERT_TRUE(weight);
	*weight.value() = 4.0F;

	const auto kStaleFace = fixture.face;
	const auto kStaleHalfedge = first_face_halfedge(fixture.mesh, fixture.face);
	auto stale_edge = fixture.mesh.halfedge_edge(kStaleHalfedge);
	ASSERT_TRUE(stale_edge);

	ASSERT_TRUE(fixture.mesh.delete_face(fixture.face));
	auto compaction = fixture.mesh.compact_storage();
	ASSERT_TRUE(compaction);

	EXPECT_FALSE(fixture.mesh.is_valid(kStaleFace));
	EXPECT_FALSE(fixture.mesh.is_valid(kStaleHalfedge));
	EXPECT_FALSE(fixture.mesh.is_valid(stale_edge.value()));
	for (const auto kVertex : fixture.vertices) {
		EXPECT_TRUE(fixture.mesh.is_valid(kVertex));
	}
	auto preserved_weight = fixture.mesh.vertex_attribute<float>(weight_attribute.value(), fixture.vertices[2]);
	ASSERT_TRUE(preserved_weight);
	EXPECT_EQ(*preserved_weight.value(), 4.0F);
	EXPECT_EQ(fixture.mesh.face_count(), 0U);
	EXPECT_TRUE(quader::mesh::validate_mesh(fixture.mesh).ok());
}

TEST(MeshCore, AttributeStorageHooksAreTypedAndResetReusedSlots) {
	quader::mesh::Polyhedron mesh;
	auto weight_attribute = mesh.attributes().create<float>(quader::mesh::AttributeDomain::Vertex, "weight", 1.5F);
	EXPECT_TRUE(weight_attribute);

	const auto kVertex = mesh.create_vertex(quader::math::Vec3{ 0.0F, 0.0F, 0.0F });
	auto weight = mesh.vertex_attribute<float>(weight_attribute.value(), kVertex);
	EXPECT_TRUE(weight);
	EXPECT_EQ(*weight.value(), 1.5F);
	*weight.value() = 2.25F;

	auto wrong_type = mesh.vertex_attribute<std::uint32_t>(weight_attribute.value(), kVertex);
	EXPECT_FALSE(wrong_type);
	EXPECT_EQ(wrong_type.error().code, quader::mesh::MeshErrorCode::AttributeTypeMismatch);

	EXPECT_TRUE(mesh.delete_vertex(kVertex));
	const auto kReusedVertex = mesh.create_vertex(quader::math::Vec3{ 1.0F, 0.0F, 0.0F });
	EXPECT_EQ(kReusedVertex.index(), kVertex.index());
	auto reset_weight = mesh.vertex_attribute<float>(weight_attribute.value(), kReusedVertex);
	EXPECT_TRUE(reset_weight);
	EXPECT_EQ(*reset_weight.value(), 1.5F);
}

} // namespace
