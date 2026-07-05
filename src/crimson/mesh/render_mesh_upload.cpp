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

#include "math/scalar.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

namespace crimson {
namespace {

[[nodiscard]] bool valid_bounds(quader::math::Aabb bounds) noexcept {
	return !quader::math::empty(bounds) && quader::math::is_finite(bounds.min) && quader::math::is_finite(bounds.max);
}

[[nodiscard]] RendererDiagnostic upload_diagnostic(std::string detail, RenderMeshHandle handle) {
	return RendererDiagnostic{
		.severity = RendererDiagnosticSeverity::Error,
		.code = RendererDiagnosticCode::UploadSkippedInvalidResource,
		.message = "Crimson rejected a prepared render mesh upload.",
		.detail = std::move(detail),
		.subsystem = "crimson.mesh_upload",
		.resource_name = "RenderMeshUploadDesc[" + std::to_string(handle.index) + "]",
	};
}

[[nodiscard]] std::uint64_t byte_size(std::span<const float> values) noexcept {
	return static_cast<std::uint64_t>(values.size_bytes());
}

[[nodiscard]] std::uint64_t byte_size(std::span<const std::uint32_t> values) noexcept {
	return static_cast<std::uint64_t>(values.size_bytes());
}

} // namespace

quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic> validate_render_mesh_upload_desc(
		const RenderMeshUploadDesc &desc) {
	if (!is_valid_handle(desc.handle)) {
		return quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic>::failure(
				upload_diagnostic("Prepared render mesh uploads require a valid public render mesh handle.", desc.handle));
	}
	if (desc.position_normal_uv_interleaved.empty()) {
		return quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic>::failure(
				upload_diagnostic("Prepared render mesh uploads require vertex data.", desc.handle));
	}
	if (desc.position_normal_uv_interleaved.size() % 8 != 0) {
		return quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic>::failure(
				upload_diagnostic("Position/normal/UV0 upload data must be interleaved as 8 floats per vertex.", desc.handle));
	}
	if (desc.indices.empty()) {
		return quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic>::failure(
				upload_diagnostic("Prepared render mesh uploads require index data.", desc.handle));
	}
	if ((desc.attributes & VertexAttributePosition) == 0) {
		return quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic>::failure(
				upload_diagnostic("Prepared render mesh uploads require position attributes.", desc.handle));
	}
	if ((desc.attributes & VertexAttributeNormal) == 0) {
		return quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic>::failure(
				upload_diagnostic("Prepared render mesh uploads require normal attributes.", desc.handle));
	}
	if ((desc.attributes & VertexAttributeUv0) == 0) {
		return quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic>::failure(
				upload_diagnostic("Prepared render mesh uploads require UV0 attributes.", desc.handle));
	}
	if (!valid_bounds(desc.bounds)) {
		return quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic>::failure(
				upload_diagnostic("Prepared render mesh uploads require finite non-empty bounds.", desc.handle));
	}

	return quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic>::success(RenderMeshUploadRecord{
			.handle = desc.handle,
			.uploaded_revision = desc.revision,
			.vertex_bytes = byte_size(desc.position_normal_uv_interleaved),
			.index_bytes = byte_size(desc.indices),
	});
}

quader::foundation::Result<FrameUploadStats, RendererDiagnostic> RenderMeshUploadTracker::process_uploads(
		std::span<const RenderMeshUploadDesc> uploads) {
	FrameUploadStats stats;
	for (const RenderMeshUploadDesc &desc : uploads) {
		auto validated = validate_render_mesh_upload_desc(desc);
		if (!validated) {
			return quader::foundation::Result<FrameUploadStats, RendererDiagnostic>::failure(
					std::move(validated).error());
		}

		RenderMeshUploadRecord record = validated.value();
		auto existing = std::find_if(records_.begin(), records_.end(), [record](const RenderMeshUploadRecord &current) {
			return current.handle == record.handle;
		});

		if (existing == records_.end()) {
			records_.push_back(record);
			++stats.mesh_create_count;
			stats.uploaded_vertex_bytes += record.vertex_bytes;
			stats.uploaded_index_bytes += record.index_bytes;
			continue;
		}

		if (existing->uploaded_revision == record.uploaded_revision) {
			++stats.skipped_clean_resource_count;
			continue;
		}

		*existing = record;
		++stats.mesh_update_count;
		stats.uploaded_vertex_bytes += record.vertex_bytes;
		stats.uploaded_index_bytes += record.index_bytes;
	}

	return quader::foundation::Result<FrameUploadStats, RendererDiagnostic>::success(stats);
}

bool RenderMeshUploadTracker::destroy(RenderMeshHandle handle) noexcept {
	const auto kExisting = std::find_if(records_.begin(), records_.end(), [handle](const RenderMeshUploadRecord &current) {
		return current.handle == handle;
	});
	if (kExisting == records_.end()) {
		return false;
	}
	records_.erase(kExisting);
	return true;
}

std::size_t RenderMeshUploadTracker::live_record_count() const noexcept {
	return records_.size();
}

const RenderMeshUploadRecord *RenderMeshUploadTracker::find(RenderMeshHandle handle) const noexcept {
	const auto kExisting = std::find_if(records_.begin(), records_.end(), [handle](const RenderMeshUploadRecord &current) {
		return current.handle == handle;
	});
	return kExisting == records_.end() ? nullptr : &*kExisting;
}

void RenderMeshUploadTracker::clear() noexcept {
	records_.clear();
}

} // namespace crimson
