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
#include "ui/viewport/viewport_render_host.hpp"

#include <memory>
#include <optional>

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

/// Create a standalone Crimson viewport render host with prototype content.
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
