#include "ui/viewport/crimson_viewport_render_host.hpp"

#include "crimson/frame/frame_builder.hpp"
#include "crimson/renderer.hpp"

#include <QCoreApplication>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace quader::ui {
namespace {

[[nodiscard]] std::uint32_t positive_u32(int value) noexcept {
	return static_cast<std::uint32_t>(std::max(1, value));
}

[[nodiscard]] std::uint16_t positive_u16(int value) noexcept {
	return static_cast<std::uint16_t>(std::clamp(value, 1, 65535));
}

[[nodiscard]] std::uint16_t coordinate_u16(int value) noexcept {
	return static_cast<std::uint16_t>(std::clamp(value, 0, 65535));
}

[[nodiscard]] const char *view_name_for_pane(const ViewportPane &pane) noexcept {
	switch (pane.camera_index) {
		case 0:
			return "Perspective";
		case 1:
			return "Top";
		case 2:
			return "Front";
		case 3:
			return "Right";
		default:
			return "Viewport";
	}
}

[[nodiscard]] crimson::PrototypeCameraProjection projection_from(CameraProjection projection) noexcept {
	return projection == CameraProjection::Orthographic
			? crimson::PrototypeCameraProjection::Orthographic
			: crimson::PrototypeCameraProjection::Perspective;
}

[[nodiscard]] ViewportPickElementKind viewport_kind_from(crimson::PickingElementKind kind) noexcept {
	switch (kind) {
		case crimson::PickingElementKind::None:
			return ViewportPickElementKind::None;
		case crimson::PickingElementKind::Object:
			return ViewportPickElementKind::Object;
		case crimson::PickingElementKind::Submesh:
			return ViewportPickElementKind::Submesh;
		case crimson::PickingElementKind::Face:
			return ViewportPickElementKind::Face;
		case crimson::PickingElementKind::Edge:
			return ViewportPickElementKind::Edge;
		case crimson::PickingElementKind::Vertex:
			return ViewportPickElementKind::Vertex;
	}
	return ViewportPickElementKind::None;
}

[[nodiscard]] QString qstring_from_ascii(std::string_view value) {
	return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

[[nodiscard]] int bounded_int(std::uint32_t value) noexcept {
	return value > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
			? std::numeric_limits<int>::max()
			: static_cast<int>(value);
}

[[nodiscard]] ViewportFrameStats viewport_frame_stats_from(const crimson::FrameStats &stats) noexcept {
	return ViewportFrameStats{
		.width = bounded_int(stats.width_px),
		.height = bounded_int(stats.height_px),
		.viewport_count = bounded_int(stats.view_count),
		.fps = stats.fps,
	};
}

[[nodiscard]] ViewportPickResult viewport_pick_result_from(const crimson::PickingResult &result) noexcept {
	return ViewportPickResult{
		.request_id = result.request_id,
		.hit = result.hit,
		.payload = ViewportPickPayload{
				.object_id = result.payload.object_id,
				.submesh_index = result.payload.submesh_index,
				.element_kind = viewport_kind_from(result.payload.element_kind),
				.element_index = result.payload.element_index,
		},
	};
}

} // namespace

ViewportDiagnosticsSnapshot viewport_diagnostics_from_crimson(
		const crimson::RendererDiagnosticsSnapshot &snapshot,
		QString renderer_name) {
	ViewportDiagnosticsSnapshot result;
	result.renderer_name = !renderer_name.isEmpty()
			? std::move(renderer_name)
			: QString::fromStdString(snapshot.status.backend_name);
	if (snapshot.latest_frame_stats) {
		result.frame = viewport_frame_stats_from(*snapshot.latest_frame_stats);
	}

	result.passes.reserve(static_cast<qsizetype>(snapshot.pass_stats.size()));
	for (const crimson::RenderPassStats &pass : snapshot.pass_stats) {
		result.passes.push_back(ViewportRenderPassStats{
				.pass_name = QString::fromStdString(pass.pass_name),
				.resource_read_count = bounded_int(pass.resource_read_count),
				.resource_write_count = bounded_int(pass.resource_write_count),
				.draw_call_count = bounded_int(pass.draw_call_count),
				.draw_packet_count = bounded_int(pass.draw_packet_count),
				.cpu_time_us = static_cast<quint64>(pass.cpu_time_us),
		});
	}

	result.counters.reserve(static_cast<qsizetype>(snapshot.counters.size()));
	for (const crimson::RendererCounter &counter : snapshot.counters) {
		result.counters.push_back(ViewportRendererCounter{
				.domain = qstring_from_ascii(crimson::renderer_counter_domain_name(counter.domain)),
				.name = QString::fromStdString(counter.name),
				.value = static_cast<quint64>(counter.value),
				.unit = qstring_from_ascii(crimson::renderer_metric_unit_name(counter.unit)),
		});
	}

	result.diagnostics.reserve(static_cast<qsizetype>(snapshot.recent_diagnostics.size()));
	for (const crimson::RendererDiagnostic &diagnostic : snapshot.recent_diagnostics) {
		result.diagnostics.push_back(ViewportRendererDiagnosticRow{
				.severity = qstring_from_ascii(crimson::renderer_diagnostic_severity_name(diagnostic.severity)),
				.code = qstring_from_ascii(crimson::renderer_diagnostic_code_name(diagnostic.code)),
				.subsystem = QString::fromStdString(diagnostic.subsystem),
				.resource_name = QString::fromStdString(diagnostic.resource_name),
				.message = QString::fromStdString(diagnostic.message),
				.detail = QString::fromStdString(diagnostic.detail),
				.frame_index = static_cast<quint64>(diagnostic.frame_index),
		});
	}

	result.frame_graph_dump = QString::fromStdString(snapshot.frame_graph_dump);
	return result;
}

class CrimsonViewportRenderHost final : public IViewportRenderHost {
public:
	CrimsonViewportRenderHost();
	~CrimsonViewportRenderHost() override;

	[[nodiscard]] ViewportRenderResult initialize_surface(
			const NativeViewportSurface &surface,
			ViewportPixelSize size,
			double device_pixel_ratio) override;
	[[nodiscard]] ViewportRenderResult resize_surface(
			ViewportPixelSize size,
			double device_pixel_ratio) override;
	[[nodiscard]] ViewportRenderResult render_frame(const ViewportRenderRequest &request) override;
	void shutdown_surface() override;

	[[nodiscard]] std::optional<ViewportFrameStats> latest_frame_stats() const override;
	[[nodiscard]] std::optional<ViewportDiagnosticsSnapshot> latest_diagnostics() const override;
	[[nodiscard]] QString renderer_name() const override;

private:
	[[nodiscard]] crimson::NativeSurfaceDescriptor make_surface_descriptor(
			const NativeViewportSurface &surface,
			ViewportPixelSize size,
			double device_pixel_ratio) const;
	[[nodiscard]] crimson::ViewportExtent make_extent(ViewportPixelSize size, double device_pixel_ratio) const;
	[[nodiscard]] ViewportRenderResult result_from_diagnostic(const crimson::RendererDiagnostic &diagnostic) const;
	[[nodiscard]] std::array<crimson::PrototypeCamera, 4> make_cameras(
			std::span<const ViewportCameraSnapshot> cameras) const;
	[[nodiscard]] std::array<crimson::PrototypeViewportView, 4> make_views(
			std::span<const ViewportPane> panes) const;
	[[nodiscard]] std::vector<crimson::PickingRequest> make_picking_requests(
			std::span<const ViewportPickRequest> requests) const;

	std::unique_ptr<crimson::Renderer> renderer_;
	QString renderer_name_;
	std::optional<ViewportFrameStats> latest_stats_;
};

CrimsonViewportRenderHost::CrimsonViewportRenderHost() = default;

CrimsonViewportRenderHost::~CrimsonViewportRenderHost() {
	shutdown_surface();
}

ViewportRenderResult CrimsonViewportRenderHost::initialize_surface(
		const NativeViewportSurface &surface,
		ViewportPixelSize size,
		double device_pixel_ratio) {
	shutdown_surface();

	crimson::RendererConfig config;
	config.backend_preference = crimson::GraphicsBackendPreference::Auto;
	config.shader_root = (QCoreApplication::applicationDirPath() + QStringLiteral("/shaders")).toStdString();
	config.vsync = true;
	config.enable_debug_text = true;

	auto created = crimson::Renderer::create(config, make_surface_descriptor(surface, size, device_pixel_ratio));
	if (!created) {
		return result_from_diagnostic(created.error());
	}

	renderer_ = std::move(created).value();
	const crimson::RendererStatus kStatus = renderer_->status();
	renderer_name_ = QString::fromStdString(kStatus.backend_name);
	return ViewportRenderResult::success(renderer_name_);
}

ViewportRenderResult CrimsonViewportRenderHost::resize_surface(
		ViewportPixelSize size,
		double device_pixel_ratio) {
	if (renderer_ == nullptr) {
		return ViewportRenderResult::failure(QStringLiteral("Renderer is not initialized."));
	}

	auto result = renderer_->resize(make_extent(size, device_pixel_ratio));
	if (!result) {
		return result_from_diagnostic(result.error());
	}

	return ViewportRenderResult::success(renderer_name_);
}

ViewportRenderResult CrimsonViewportRenderHost::render_frame(const ViewportRenderRequest &request) {
	if (renderer_ == nullptr) {
		return ViewportRenderResult::failure(QStringLiteral("Renderer is not initialized."));
	}

	const auto kCameras = make_cameras(request.cameras);
	const auto kViews = make_views(request.panes);
	const std::vector<crimson::PickingRequest> kPickingRequests = make_picking_requests(request.picking_requests);
	const auto kFrame = crimson::PrototypeViewportFrame{
		.target_extent = make_extent(request.surface_size, request.device_pixel_ratio),
		.views = std::span<const crimson::PrototypeViewportView>(kViews.data(), request.panes.size()),
		.cameras = std::span<const crimson::PrototypeCamera>(kCameras.data(), kCameras.size()),
		.picking_requests = std::span<const crimson::PickingRequest>(kPickingRequests.data(), kPickingRequests.size()),
		.animation_enabled = request.prototype_animation_enabled,
		.elapsed_seconds = request.elapsed_seconds,
	};

	const crimson::FrameBuilder kFrameBuilder;
	auto snapshot = kFrameBuilder.build_prototype_snapshot(kFrame);
	if (!snapshot) {
		return result_from_diagnostic(snapshot.error());
	}

	auto rendered = renderer_->render(snapshot.value());
	if (!rendered) {
		return result_from_diagnostic(rendered.error());
	}

	crimson::FrameRenderResult frame_result = std::move(rendered).value();
	const crimson::FrameStats kStats = frame_result.stats;
	latest_stats_ = viewport_frame_stats_from(kStats);
	std::vector<ViewportPickResult> completed_pick_results;
	completed_pick_results.reserve(frame_result.completed_picking_results.size());
	for (const crimson::PickingResult &result : frame_result.completed_picking_results) {
		completed_pick_results.push_back(viewport_pick_result_from(result));
	}
	return ViewportRenderResult::success(renderer_name_, std::move(completed_pick_results));
}

void CrimsonViewportRenderHost::shutdown_surface() {
	renderer_.reset();
	latest_stats_ = std::nullopt;
}

std::optional<ViewportFrameStats> CrimsonViewportRenderHost::latest_frame_stats() const {
	return latest_stats_;
}

std::optional<ViewportDiagnosticsSnapshot> CrimsonViewportRenderHost::latest_diagnostics() const {
	if (renderer_ == nullptr) {
		return std::nullopt;
	}

	return viewport_diagnostics_from_crimson(renderer_->diagnostics_snapshot(), renderer_name_);
}

QString CrimsonViewportRenderHost::renderer_name() const {
	return renderer_name_;
}

crimson::NativeSurfaceDescriptor CrimsonViewportRenderHost::make_surface_descriptor(
		const NativeViewportSurface &surface,
		ViewportPixelSize size,
		double device_pixel_ratio) const {
	crimson::NativeSurfacePlatform platform = crimson::NativeSurfacePlatform::Unknown;
	if (surface.platform == NativeViewportSurface::Platform::WindowsHwnd) {
		platform = crimson::NativeSurfacePlatform::Windows;
	}

	return crimson::NativeSurfaceDescriptor{
		.platform = platform,
		.native_window_handle = surface.native_window,
		.native_display_handle = nullptr,
		.native_layer_handle = nullptr,
		.extent = make_extent(size, device_pixel_ratio),
	};
}

crimson::ViewportExtent CrimsonViewportRenderHost::make_extent(
		ViewportPixelSize size,
		double device_pixel_ratio) const {
	return crimson::ViewportExtent{
		.width_px = positive_u32(size.width),
		.height_px = positive_u32(size.height),
		.device_pixel_ratio = static_cast<float>(std::max(0.01, device_pixel_ratio)),
	};
}

ViewportRenderResult CrimsonViewportRenderHost::result_from_diagnostic(
		const crimson::RendererDiagnostic &diagnostic) const {
	QString summary = QString::fromStdString(diagnostic.message);
	if (summary.isEmpty()) {
		summary = QStringLiteral("Crimson renderer error");
	}

	return ViewportRenderResult::failure(summary, QString::fromStdString(diagnostic.detail));
}

std::array<crimson::PrototypeCamera, 4> CrimsonViewportRenderHost::make_cameras(
		std::span<const ViewportCameraSnapshot> cameras) const {
	std::array<crimson::PrototypeCamera, 4> result{};
	for (std::size_t index = 0; index < result.size(); ++index) {
		const ViewportCameraSnapshot &source = cameras[index < cameras.size() ? index : 0];
		result[index] = crimson::PrototypeCamera{
			.eye = source.eye,
			.target = source.target,
			.up = source.up,
			.forward = source.forward,
			.projection = projection_from(source.projection),
			.fov_degrees = source.fov_degrees,
			.orthographic_size = source.orthographic_size,
		};
	}
	return result;
}

std::array<crimson::PrototypeViewportView, 4> CrimsonViewportRenderHost::make_views(
		std::span<const ViewportPane> panes) const {
	std::array<crimson::PrototypeViewportView, 4> result{};
	for (std::size_t index = 0; index < result.size(); ++index) {
		const ViewportPane &source = panes[index < panes.size() ? index : 0];
		result[index] = crimson::PrototypeViewportView{
			.rect = crimson::PrototypeViewportRect{
					.x = coordinate_u16(source.rect.x),
					.y = coordinate_u16(source.rect.y),
					.width = positive_u16(source.rect.width),
					.height = positive_u16(source.rect.height),
			},
			.camera_index = static_cast<std::uint8_t>(std::clamp(source.camera_index, 0, 3)),
			.debug_name = view_name_for_pane(source),
		};
	}
	return result;
}

std::vector<crimson::PickingRequest> CrimsonViewportRenderHost::make_picking_requests(
		std::span<const ViewportPickRequest> requests) const {
	std::vector<crimson::PickingRequest> result;
	result.reserve(requests.size());
	for (const ViewportPickRequest &request : requests) {
		result.push_back(crimson::PickingRequest{
				.request_id = request.request_id,
				.view_index = request.view_index,
				.x_px = request.x_px,
				.y_px = request.y_px,
		});
	}
	return result;
}

std::unique_ptr<IViewportRenderHost> create_crimson_viewport_render_host() {
	return std::make_unique<CrimsonViewportRenderHost>();
}

} // namespace quader::ui
