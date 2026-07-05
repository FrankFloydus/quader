/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "benchmark_main.hpp"

#include "crimson/mesh/render_mesh_upload.hpp"

#include <array>
#include <vector>

namespace quader::benchmarks {
namespace {

[[nodiscard]] std::vector<crimson::RenderMeshUploadDesc> make_uploads(
		std::uint32_t count,
		std::vector<std::array<float, 18>> &vertices,
		std::vector<std::array<std::uint32_t, 3>> &indices) {
	vertices.resize(count);
	indices.resize(count);
	std::vector<crimson::RenderMeshUploadDesc> uploads;
	uploads.reserve(count);
	for (std::uint32_t index = 0; index < count; ++index) {
		vertices[index] = {
			-1.0F,
			-1.0F,
			0.0F,
			0.0F,
			0.0F,
			1.0F,
			1.0F,
			-1.0F,
			0.0F,
			0.0F,
			0.0F,
			1.0F,
			0.0F,
			1.0F,
			0.0F,
			0.0F,
			0.0F,
			1.0F,
		};
		indices[index] = { 0, 1, 2 };
		uploads.push_back(crimson::RenderMeshUploadDesc{
				.handle = crimson::RenderMeshHandle{ index + 1, 1 },
				.revision = crimson::RenderMeshRevision{ 1, 1, index % 4 },
				.position_normal_interleaved = vertices[index],
				.indices = indices[index],
				.attributes = crimson::VertexAttributePosition | crimson::VertexAttributeNormal,
				.bounds = quader::math::Aabb{
						.min = { -1.0F, -1.0F, -1.0F },
						.max = { 1.0F, 1.0F, 1.0F },
				},
		});
	}
	return uploads;
}

} // namespace

BenchmarkResult run_crimson_upload_benchmark(const BenchmarkRunConfig &config) {
	std::vector<std::array<float, 18>> vertices;
	std::vector<std::array<std::uint32_t, 3>> indices;
	const std::vector<crimson::RenderMeshUploadDesc> kUploads =
			make_uploads(config.fixture_size, vertices, indices);

	return run_benchmark("crimson_upload_tracker", config, [&]() {
		crimson::RenderMeshUploadTracker tracker;
		const auto kFirst = tracker.process_uploads(kUploads);
		const auto kClean = tracker.process_uploads(kUploads);
		crimson::FrameUploadStats upload_stats;
		if (kFirst) {
			upload_stats.mesh_create_count += kFirst.value().mesh_create_count;
			upload_stats.uploaded_vertex_bytes += kFirst.value().uploaded_vertex_bytes;
			upload_stats.uploaded_index_bytes += kFirst.value().uploaded_index_bytes;
		}
		if (kClean) {
			upload_stats.skipped_clean_resource_count += kClean.value().skipped_clean_resource_count;
		}
		const crimson::FrameStats kStats = crimson::make_frame_stats(crimson::FrameStatsInput{
				.uploads = upload_stats,
		});
		return crimson::make_renderer_counters(kStats);
	});
}

} // namespace quader::benchmarks
