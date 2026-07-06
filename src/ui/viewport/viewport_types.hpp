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

#include "math/vec3.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include <QString>
#include <QVector>
#include <QtGlobal>

namespace quader::ui {

/// Viewport size in physical pixels.
struct ViewportPixelSize {
	/// Width in physical pixels.
	int width = 1;
	/// Height in physical pixels.
	int height = 1;
};

/// Viewport-local point in physical pixels.
struct ViewportPoint {
	/// X coordinate.
	double x = 0.0;
	/// Y coordinate.
	double y = 0.0;
};

/// Viewport rectangle in physical pixels.
struct ViewportRect {
	/// Left edge.
	int x = 0;
	/// Top edge.
	int y = 0;
	/// Width in pixels.
	int width = 1;
	/// Height in pixels.
	int height = 1;
};

/// Native window/surface handle passed to the render host.
struct NativeViewportSurface {
	/// Platform type represented by `native_window`.
	enum class Platform {
		Unknown,     ///< No supported native platform.
		WindowsHwnd, ///< Win32 `HWND`.
	};

	/// Native platform.
	Platform platform = Platform::Unknown;
	/// Borrowed native window handle; ownership stays with Qt.
	void *native_window = nullptr;
};

/// Viewport layout mode.
enum class ViewportLayoutMode {
	Single, ///< One viewport pane.
	Quad,   ///< Four viewport panes.
};

/// Viewport mesh surface shading mode.
enum class ViewportShadingMode {
	Wireframe, ///< Reference wireframe/shaded scene layer.
	Shaded,   ///< Unlit editor shaded mode.
	Rendered, ///< Lit rendered mode.
};

/// Splitter handle under the pointer.
enum class ViewportSplitHandle {
	None,       ///< No splitter.
	Vertical,   ///< Vertical splitter.
	Horizontal, ///< Horizontal splitter.
};

/// Toolkit-neutral viewport mouse button.
enum class ViewportMouseButton {
	Left,   ///< Left button.
	Middle, ///< Middle button.
	Right,  ///< Right button.
	Other,  ///< Unsupported or auxiliary button.
};

/// Toolkit-neutral viewport key used by navigation/tools.
enum class ViewportKey {
	W,      ///< W key.
	A,      ///< A key.
	S,      ///< S key.
	D,      ///< D key.
	Escape, ///< Escape key.
	Other,  ///< Unsupported key.
};

/// Camera projection mode.
enum class CameraProjection {
	Perspective, ///< Perspective projection.
	Orthographic, ///< Orthographic projection.
};

/// Active viewport navigation mode.
enum class NavigationMode {
	None,  ///< No navigation gesture.
	Orbit, ///< Orbit around the camera target.
	Pan,   ///< Pan camera target/eye.
	Fly,   ///< Free-look fly navigation.
};

/// One viewport pane in the current layout.
struct ViewportPane {
	/// Pane rectangle in physical pixels.
	ViewportRect rect;
	/// Camera index assigned to this pane.
	int camera_index = 0;
	/// User-visible pane name.
	QString name;
};

/// CPU-side camera snapshot sent to a render host.
struct ViewportCameraSnapshot {
	/// Camera index.
	int camera_index = 0;
	/// Camera eye position in world space.
	quader::math::Vec3 eye{};
	/// Camera target point in world space.
	quader::math::Vec3 target{};
	/// Camera up vector in world space.
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	/// Camera forward vector in world space.
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
	/// Projection mode.
	CameraProjection projection = CameraProjection::Perspective;
	/// Perspective field of view in degrees.
	float fov_degrees = 60.0F;
	/// Orthographic view height in world units.
	float orthographic_size = 24.0F;
};

/// Stable id for viewport picking requests.
using ViewportPickRequestId = std::uint64_t;

/// Viewport-level picking payload kind.
enum class ViewportPickElementKind : std::uint8_t {
	None,    ///< No hit.
	Object,  ///< Object-level hit.
	Submesh, ///< Submesh hit.
	Face,    ///< Face hit.
	Edge,    ///< Edge hit.
	Vertex,  ///< Vertex hit.
};

/// Viewport-level picking payload.
struct ViewportPickPayload {
	/// Stable object id encoded for UI use.
	std::uint64_t object_id = 0;
	/// Submesh index.
	std::uint32_t submesh_index = 0;
	/// Hit element kind.
	ViewportPickElementKind element_kind = ViewportPickElementKind::None;
	/// Hit element index.
	std::uint32_t element_index = 0;
};

/// Pixel readback request issued by viewport UI.
struct ViewportPickRequest {
	/// Request id returned with the result.
	ViewportPickRequestId request_id = 0;
	/// View index containing the requested pixel.
	std::uint32_t view_index = 0;
	/// X coordinate in physical pixels.
	std::uint16_t x_px = 0;
	/// Y coordinate in physical pixels.
	std::uint16_t y_px = 0;
};

/// Completed viewport picking result.
struct ViewportPickResult {
	/// Request id copied from the request.
	ViewportPickRequestId request_id = 0;
	/// True when the renderer returned a hit payload.
	bool hit = false;
	/// Decoded picking payload.
	ViewportPickPayload payload;
};

/// Render request submitted by the viewport controller.
struct ViewportRenderRequest {
	/// Full surface size.
	ViewportPixelSize surface_size;
	/// Qt device-pixel ratio for the surface.
	double device_pixel_ratio = 1.0;
	/// Active layout mode.
	ViewportLayoutMode layout_mode = ViewportLayoutMode::Single;
	/// Borrowed pane list valid for the call.
	std::span<const ViewportPane> panes;
	/// Borrowed camera list valid for the call.
	std::span<const ViewportCameraSnapshot> cameras;
	/// Borrowed picking request list valid for the call.
	std::span<const ViewportPickRequest> picking_requests;
	/// Mesh surface shading mode for document objects.
	ViewportShadingMode shading_mode = ViewportShadingMode::Shaded;
	/// True when scene animation should advance.
	bool scene_animation_enabled = true;
	/// Elapsed application time in seconds.
	double elapsed_seconds = 0.0;
};

/// Compact viewport frame stats presented in the UI.
struct ViewportFrameStats {
	/// Rendered width in pixels.
	int width = 0;
	/// Rendered height in pixels.
	int height = 0;
	/// Number of rendered viewport panes.
	int viewport_count = 1;
	/// Frames per second estimate.
	double fps = 0.0;
};

/// UI-facing stats for one render pass.
struct ViewportRenderPassStats {
	/// Pass name.
	QString pass_name;
	/// Number of resources read.
	int resource_read_count = 0;
	/// Number of resources written.
	int resource_write_count = 0;
	/// Draw call count.
	int draw_call_count = 0;
	/// Draw packet count.
	int draw_packet_count = 0;
	/// CPU time in microseconds.
	quint64 cpu_time_us = 0;
};

/// UI-facing renderer counter row.
struct ViewportRendererCounter {
	/// Counter domain.
	QString domain;
	/// Counter name.
	QString name;
	/// Counter value.
	quint64 value = 0;
	/// Counter unit label.
	QString unit;
};

/// UI-facing renderer diagnostic row.
struct ViewportRendererDiagnosticRow {
	/// Severity text.
	QString severity;
	/// Diagnostic code.
	QString code;
	/// Subsystem that reported the diagnostic.
	QString subsystem;
	/// Associated resource name.
	QString resource_name;
	/// Short diagnostic message.
	QString message;
	/// Detailed diagnostic text.
	QString detail;
	/// Frame index associated with the diagnostic.
	quint64 frame_index = 0;
};

/// Full diagnostics snapshot exposed by a viewport render host.
struct ViewportDiagnosticsSnapshot {
	/// Renderer display name.
	QString renderer_name;
	/// Latest frame stats.
	ViewportFrameStats frame;
	/// Render pass stats.
	QVector<ViewportRenderPassStats> passes;
	/// Renderer counters.
	QVector<ViewportRendererCounter> counters;
	/// Renderer diagnostics.
	QVector<ViewportRendererDiagnosticRow> diagnostics;
	/// Text dump of the current render graph.
	QString frame_graph_dump;
};

/// Visible splitter width in pixels.
constexpr int kViewportSplitterWidth = 4;
/// Splitter hit-test tolerance in pixels.
constexpr int kViewportSplitterHitTolerance = 6;
/// Minimum pane extent in pixels.
constexpr int kMinimumViewportExtent = 96;

/// Return the number of panes for a layout mode.
[[nodiscard]] constexpr int pane_count_for_layout(ViewportLayoutMode mode) noexcept {
	return mode == ViewportLayoutMode::Quad ? 4 : 1;
}

/// Fixed-capacity pane list for all supported viewport layouts.
using ViewportPaneArray = std::array<ViewportPane, 4>;
/// Fixed-capacity camera snapshot list for all supported viewport layouts.
using ViewportCameraSnapshotArray = std::array<ViewportCameraSnapshot, 4>;

} // namespace quader::ui
