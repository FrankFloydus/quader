#pragma once

#include "math/vec3.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include <QVector>
#include <QString>
#include <QtGlobal>

namespace quader::ui {

struct ViewportPixelSize {
    int width = 1;
    int height = 1;
};

struct ViewportPoint {
    double x = 0.0;
    double y = 0.0;
};

struct ViewportRect {
    int x = 0;
    int y = 0;
    int width = 1;
    int height = 1;
};

struct NativeViewportSurface {
    enum class Platform {
        Unknown,
        WindowsHwnd,
    };

    Platform platform = Platform::Unknown;
    void* native_window = nullptr;
};

enum class ViewportLayoutMode {
    Single,
    Quad,
};

enum class ViewportSplitHandle {
    None,
    Vertical,
    Horizontal,
};

enum class ViewportMouseButton {
    Left,
    Middle,
    Right,
    Other,
};

enum class ViewportKey {
    W,
    A,
    S,
    D,
    Other,
};

enum class CameraProjection {
    Perspective,
    Orthographic,
};

enum class NavigationMode {
    None,
    Orbit,
    Pan,
    Fly,
};

struct ViewportPane {
    ViewportRect rect;
    int camera_index = 0;
    QString name;
};

struct ViewportCameraSnapshot {
    int camera_index = 0;
    quader::math::Vec3 eye{};
    quader::math::Vec3 target{};
    quader::math::Vec3 up{0.0F, 1.0F, 0.0F};
    quader::math::Vec3 forward{0.0F, 0.0F, -1.0F};
    CameraProjection projection = CameraProjection::Perspective;
    float fov_degrees = 60.0F;
    float orthographic_size = 24.0F;
};

using ViewportPickRequestId = std::uint64_t;

enum class ViewportPickElementKind : std::uint8_t {
    None,
    Object,
    Submesh,
    Face,
    Edge,
    Vertex,
};

struct ViewportPickPayload {
    std::uint64_t object_id = 0;
    std::uint32_t submesh_index = 0;
    ViewportPickElementKind element_kind = ViewportPickElementKind::None;
    std::uint32_t element_index = 0;
};

struct ViewportPickRequest {
    ViewportPickRequestId request_id = 0;
    std::uint32_t view_index = 0;
    std::uint16_t x_px = 0;
    std::uint16_t y_px = 0;
};

struct ViewportPickResult {
    ViewportPickRequestId request_id = 0;
    bool hit = false;
    ViewportPickPayload payload;
};

struct ViewportRenderRequest {
    ViewportPixelSize surface_size;
    double device_pixel_ratio = 1.0;
    ViewportLayoutMode layout_mode = ViewportLayoutMode::Single;
    std::span<const ViewportPane> panes;
    std::span<const ViewportCameraSnapshot> cameras;
    std::span<const ViewportPickRequest> picking_requests;
    bool prototype_animation_enabled = true;
    double elapsed_seconds = 0.0;
};

struct ViewportFrameStats {
    int width = 0;
    int height = 0;
    int viewport_count = 1;
    double fps = 0.0;
};

struct ViewportRenderPassStats {
    QString pass_name;
    int resource_read_count = 0;
    int resource_write_count = 0;
    int draw_call_count = 0;
    int draw_packet_count = 0;
    quint64 cpu_time_us = 0;
};

struct ViewportRendererCounter {
    QString domain;
    QString name;
    quint64 value = 0;
    QString unit;
};

struct ViewportRendererDiagnosticRow {
    QString severity;
    QString code;
    QString subsystem;
    QString resource_name;
    QString message;
    QString detail;
    quint64 frame_index = 0;
};

struct ViewportDiagnosticsSnapshot {
    QString renderer_name;
    ViewportFrameStats frame;
    QVector<ViewportRenderPassStats> passes;
    QVector<ViewportRendererCounter> counters;
    QVector<ViewportRendererDiagnosticRow> diagnostics;
    QString frame_graph_dump;
};

constexpr int kViewportSplitterWidth = 4;
constexpr int kViewportSplitterHitTolerance = 6;
constexpr int kMinimumViewportExtent = 96;

[[nodiscard]] constexpr int pane_count_for_layout(ViewportLayoutMode mode) noexcept
{
    return mode == ViewportLayoutMode::Quad ? 4 : 1;
}

using ViewportPaneArray = std::array<ViewportPane, 4>;
using ViewportCameraSnapshotArray = std::array<ViewportCameraSnapshot, 4>;

} // namespace quader::ui
