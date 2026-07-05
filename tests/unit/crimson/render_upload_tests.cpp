/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/mesh/render_mesh_upload.hpp"

#include <gtest/gtest.h>

#include <array>
#include <iostream>
#include <string_view>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

[[nodiscard]] quader::math::Aabb unit_bounds() {
	return quader::math::Aabb{
		.min = { -1.0F, -1.0F, -1.0F },
		.max = { 1.0F, 1.0F, 1.0F },
	};
}

[[nodiscard]] crimson::RenderMeshUploadDesc upload_desc(crimson::RenderMeshRevision revision) {
	static const std::array<float, 18> kVertices = {
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
	static const std::array<std::uint32_t, 3> kIndices = { 0, 1, 2 };
	return crimson::RenderMeshUploadDesc{
		.handle = crimson::RenderMeshHandle{ 1, 1 },
		.revision = revision,
		.position_normal_interleaved = kVertices,
		.indices = kIndices,
		.attributes = crimson::VertexAttributePosition | crimson::VertexAttributeNormal,
		.bounds = unit_bounds(),
	};
}

TEST(RenderUpload, UploadTrackerCountsCreateCleanSkipAndUpdate) {
	crimson::RenderMeshUploadTracker tracker;

	const std::array kFirstUploads = { upload_desc(crimson::RenderMeshRevision{ 1, 1, 1 }) };
	const auto kFirst = tracker.process_uploads(kFirstUploads);
	expect_true(kFirst.has_value(), "first prepared upload is accepted");
	expect_true(kFirst && kFirst.value().mesh_create_count == 1, "new revision creates an upload record");
	expect_true(kFirst && kFirst.value().uploaded_vertex_bytes == 72, "vertex upload bytes are counted");
	expect_true(kFirst && kFirst.value().uploaded_index_bytes == 12, "index upload bytes are counted");

	const std::array kCleanUploads = { upload_desc(crimson::RenderMeshRevision{ 1, 1, 1 }) };
	const auto kClean = tracker.process_uploads(kCleanUploads);
	expect_true(kClean.has_value(), "clean repeated upload is accepted");
	expect_true(kClean && kClean.value().skipped_clean_resource_count == 1, "same revision increments clean skip counter");
	expect_true(kClean && kClean.value().uploaded_vertex_bytes == 0, "clean upload does not count bytes");

	const std::array kChangedUploads = { upload_desc(crimson::RenderMeshRevision{ 2, 1, 1 }) };
	const auto kChanged = tracker.process_uploads(kChangedUploads);
	expect_true(kChanged.has_value(), "changed prepared upload is accepted");
	expect_true(kChanged && kChanged.value().mesh_update_count == 1, "changed revision schedules an update");
	expect_true(tracker.find(crimson::RenderMeshHandle{ 1, 1 }) != nullptr, "upload tracker stores latest record");
}

TEST(RenderUpload, InvalidUploadsReturnStructuredDiagnostics) {
	crimson::RenderMeshUploadDesc invalid = upload_desc(crimson::RenderMeshRevision{ 1, 1, 1 });
	invalid.position_normal_interleaved = {};

	const auto kResult = crimson::validate_render_mesh_upload_desc(invalid);
	expect_true(!kResult, "invalid upload is rejected");
	expect_true(
			!kResult && kResult.error().code == crimson::RendererDiagnosticCode::UploadSkippedInvalidResource,
			"invalid upload uses upload-specific diagnostic code");
	expect_true(!kResult && crimson::has_structured_context(kResult.error()), "invalid upload diagnostic is structured");
}

TEST(RenderUpload, DestroyRemovesTrackedRecord) {
	crimson::RenderMeshUploadTracker tracker;
	const std::array kUploads = { upload_desc(crimson::RenderMeshRevision{ 1, 1, 1 }) };
	(void)tracker.process_uploads(kUploads);
	expect_true(tracker.live_record_count() == 1, "tracker has one record before destroy");
	expect_true(tracker.destroy(crimson::RenderMeshHandle{ 1, 1 }), "tracked record can be destroyed");
	expect_true(tracker.live_record_count() == 0, "destroy removes tracked record");
	expect_true(!tracker.destroy(crimson::RenderMeshHandle{ 1, 1 }), "destroying a stale record returns false");
}

} // namespace
