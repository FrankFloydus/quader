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

#include "crimson/diagnostics/renderer_diagnostics_snapshot.hpp"
#include "crimson/material/base_shader.hpp"
#include "crimson/mesh/render_mesh.hpp"
#include "crimson/overlays/overlay_command.hpp"
#include "crimson/scene/render_object.hpp"
#include "document/object_id.hpp"
#include "math/aabb.hpp"
#include "ui/viewport/viewport_render_host.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include <QString>

namespace quader::document {
class Document;
struct MeshObject;
}

namespace quader::tools {
class ToolManager;
}

namespace quader::ui {

/**
 * Convert Crimson diagnostics into UI-facing viewport diagnostics.
 *
 * @param snapshot Crimson diagnostics snapshot.
 * @param renderer_name Optional renderer name override.
 * @return UI diagnostics snapshot.
 */
[[nodiscard]] ViewportDiagnosticsSnapshot viewport_diagnostics_from_crimson(
		const crimson::RendererDiagnosticsSnapshot &snapshot,
		QString renderer_name = {});

/**
 * Return the Crimson base shader used for document mesh surfaces in a viewport shading mode.
 *
 * @param mode User-facing viewport shading mode.
 * @return Base shader id submitted for document meshes.
 */
[[nodiscard]] crimson::BaseShaderId viewport_base_shader_for_shading_mode(
		ViewportShadingMode mode) noexcept;

/**
 * Build the Crimson render upload payload for a document mesh object.
 *
 * The viewport render host uses this to translate editable mesh topology into
 * Crimson's position/normal/UV triangle buffer.
 *
 * @param object Document mesh object to upload.
 * @return Render mesh upload payload, or empty for invalid/empty mesh data.
 */
[[nodiscard]] std::optional<crimson::RenderMeshUpload> make_crimson_viewport_mesh_upload(
		const quader::document::MeshObject &object);

/// Cached document mesh render data for one viewport frame.
struct ViewportDocumentRenderMesh {
	/// Stable Crimson mesh handle for the document object.
	crimson::RenderMeshHandle handle;
	/// Local-space bounds represented by the mesh.
	quader::math::Aabb local_bounds;
	/// Upload payload when the mesh revision is new or stale.
	std::optional<crimson::RenderMeshUpload> upload;
};

/// Cache document mesh render metadata so clean viewport frames skip upload payloads.
class ViewportDocumentRenderCache final {
public:
	/// Return render data for a document mesh object, including an upload only when stale.
	[[nodiscard]] std::optional<ViewportDocumentRenderMesh> mesh_for(
			const quader::document::MeshObject &object);
	/// Remove cached objects no longer present in the document.
	void prune(std::span<const quader::document::ObjectId> live_objects);
	/// Drop every cached mesh entry.
	void clear() noexcept;
	/// Return the number of cached document mesh entries.
	[[nodiscard]] std::size_t cached_mesh_count() const noexcept;

private:
	struct Entry {
		quader::document::ObjectId object;
		crimson::RenderMeshRevision revision;
		crimson::RenderMeshHandle handle;
		quader::math::Aabb local_bounds;
	};

	std::vector<Entry> entries_;
};

/**
 * Append document mesh uploads and render objects for a viewport frame.
 *
 * Wireframe viewport mode intentionally emits no filled scene render objects;
 * scene topology is supplied by `append_crimson_scene_wireframe_overlays`.
 */
void append_crimson_document_render_data(
		const quader::document::Document &document,
		ViewportDocumentRenderCache &document_render_cache,
		std::vector<crimson::RenderMeshUpload> &mesh_uploads,
		std::vector<crimson::RenderObject> &objects,
		ViewportShadingMode shading_mode);

/// Append x-ray/draw-on-top scene topology overlays for wireframe viewport mode.
void append_crimson_scene_wireframe_overlays(
		const quader::document::Document &document,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads);

/// Create a standalone Crimson viewport render host with viewport content.
std::unique_ptr<IViewportRenderHost> create_crimson_viewport_render_host();

/**
 * Create a Crimson viewport render host backed by document and tool state.
 *
 * @param document Document state to translate into frame snapshots.
 * @param tool_manager Tool manager whose preview is translated into overlays.
 * @return Render host owned by the caller.
 */
std::unique_ptr<IViewportRenderHost> create_crimson_viewport_render_host(
		const quader::document::Document &document,
		const quader::tools::ToolManager &tool_manager);

} // namespace quader::ui
