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

#include "crimson/frame/frame_stats.hpp"
#include "crimson/mesh/render_mesh.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "foundation/result.hpp"

#include <span>
#include <vector>

namespace crimson {

/**
 * Validate a render mesh upload descriptor.
 *
 * @param desc Upload descriptor to inspect.
 * @return Upload record with byte counts, or a renderer diagnostic.
 */
[[nodiscard]] quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic> validate_render_mesh_upload_desc(
		const RenderMeshUploadDesc &desc);

/// Tracks uploaded render mesh revisions and upload counters.
class RenderMeshUploadTracker final {
public:
	/**
	 * Validate/process upload descriptors and update tracked records.
	 *
	 * @param uploads Upload descriptors borrowed for this call.
	 * @return Frame upload counters, or a renderer diagnostic.
	 */
	[[nodiscard]] quader::foundation::Result<FrameUploadStats, RendererDiagnostic> process_uploads(
			std::span<const RenderMeshUploadDesc> uploads);
	/**
	 * Destroy a tracked upload record.
	 *
	 * @param handle Render mesh handle to remove.
	 * @return True when a record was removed.
	 */
	[[nodiscard]] bool destroy(RenderMeshHandle handle) noexcept;
	/**
	 * Return the number of live tracked records.
	 *
	 * @return Live record count.
	 */
	[[nodiscard]] std::size_t live_record_count() const noexcept;
	/**
	 * Find a tracked upload record.
	 *
	 * @param handle Render mesh handle to resolve.
	 * @return Borrowed record, or `nullptr` when absent.
	 */
	[[nodiscard]] const RenderMeshUploadRecord *find(RenderMeshHandle handle) const noexcept;
	/// Remove all tracked upload records.
	void clear() noexcept;

private:
	std::vector<RenderMeshUploadRecord> records_;
};

} // namespace crimson
