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

#include "foundation/result.hpp"
#include "mesh/core/mesh_error.hpp"
#include "mesh/core/mesh_ids.hpp"
#include "mesh/core/polyhedron.hpp"

#include <utility>
#include <vector>

namespace quader::mesh {

/**
 * Return the vertices that bound a face.
 *
 * @param mesh Mesh to query.
 * @param face Face whose halfedge cycle is traversed.
 * @return Vertex IDs in face-cycle order, or a mesh error.
 */
[[nodiscard]] inline quader::foundation::Result<std::vector<VertexId>, MeshError> face_vertices(
		const Polyhedron &mesh,
		FaceId face) {
	auto halfedges = mesh.face_halfedges(face);
	if (!halfedges) {
		return quader::foundation::Result<std::vector<VertexId>, MeshError>::failure(
				std::move(halfedges).error());
	}

	std::vector<VertexId> vertices;
	vertices.reserve(halfedges.value().size());

	for (const auto kHalfedge : halfedges.value()) {
		auto origin = mesh.halfedge_origin(kHalfedge);
		if (!origin) {
			return quader::foundation::Result<std::vector<VertexId>, MeshError>::failure(
					std::move(origin).error());
		}

		vertices.push_back(origin.value());
	}

	return quader::foundation::Result<std::vector<VertexId>, MeshError>::success(std::move(vertices));
}

/**
 * Return the edges that bound a face.
 *
 * @param mesh Mesh to query.
 * @param face Face whose halfedge cycle is traversed.
 * @return Edge IDs in face-cycle order, or a mesh error.
 */
[[nodiscard]] inline quader::foundation::Result<std::vector<EdgeId>, MeshError> face_edges(
		const Polyhedron &mesh,
		FaceId face) {
	auto halfedges = mesh.face_halfedges(face);
	if (!halfedges) {
		return quader::foundation::Result<std::vector<EdgeId>, MeshError>::failure(std::move(halfedges).error());
	}

	std::vector<EdgeId> edges;
	edges.reserve(halfedges.value().size());

	for (const auto kHalfedge : halfedges.value()) {
		auto edge = mesh.halfedge_edge(kHalfedge);
		if (!edge) {
			return quader::foundation::Result<std::vector<EdgeId>, MeshError>::failure(
					std::move(edge).error());
		}

		edges.push_back(edge.value());
	}

	return quader::foundation::Result<std::vector<EdgeId>, MeshError>::success(std::move(edges));
}

/**
 * Find a directed halfedge between two vertices.
 *
 * @param mesh Mesh to scan.
 * @param origin Origin vertex.
 * @param target Target vertex.
 * @return Matching halfedge, or `HalfedgeId::invalid()` when absent or inputs are invalid.
 */
[[nodiscard]] inline HalfedgeId find_halfedge(const Polyhedron &mesh,
		VertexId origin,
		VertexId target) noexcept {
	if (!mesh.is_valid(origin) || !mesh.is_valid(target)) {
		return HalfedgeId::invalid();
	}

	for (const auto kHalfedge : mesh.halfedge_ids()) {
		auto halfedge_origin = mesh.halfedge_origin(kHalfedge);
		if (!halfedge_origin || halfedge_origin.value() != origin) {
			continue;
		}

		auto halfedge_target = mesh.halfedge_target(kHalfedge);
		if (halfedge_target && halfedge_target.value() == target) {
			return kHalfedge;
		}
	}

	return HalfedgeId::invalid();
}

} // namespace quader::mesh
