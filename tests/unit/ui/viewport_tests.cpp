#include "ui/viewport/crimson_viewport_render_host.hpp"
#include "ui/viewport/viewport_camera_controller.hpp"
#include "ui/viewport/viewport_controller.hpp"
#include "ui/viewport/viewport_layout_state.hpp"
#include "ui/viewport/viewport_render_host.hpp"

#include <gtest/gtest.h>

#include "math/vec3.hpp"

#include <cmath>
#include <cstdio>
#include <optional>
#include <vector>

namespace {

struct FakeRenderHost final : quader::ui::IViewportRenderHost {
	int initialize_calls = 0;
	int resize_calls = 0;
	int render_calls = 0;
	quader::ui::ViewportPixelSize initialized_size;
	quader::ui::ViewportPixelSize resized_size;
	quader::ui::ViewportLayoutMode last_layout = quader::ui::ViewportLayoutMode::Single;
	std::size_t last_pane_count = 0;
	std::size_t last_camera_count = 0;
	bool last_animation_enabled = false;
	std::optional<quader::ui::ViewportFrameStats> stats;
	std::optional<quader::ui::ViewportDiagnosticsSnapshot> diagnostics;

	quader::ui::ViewportRenderResult initialize_surface(
			const quader::ui::NativeViewportSurface &,
			quader::ui::ViewportPixelSize size,
			double) override {
		++initialize_calls;
		initialized_size = size;
		stats = quader::ui::ViewportFrameStats{ size.width, size.height, 1, 60.0 };
		return quader::ui::ViewportRenderResult::success(QStringLiteral("FakeRenderer"));
	}

	quader::ui::ViewportRenderResult resize_surface(quader::ui::ViewportPixelSize size, double) override {
		++resize_calls;
		resized_size = size;
		stats = quader::ui::ViewportFrameStats{ size.width, size.height, 1, 60.0 };
		return quader::ui::ViewportRenderResult::success(QStringLiteral("FakeRenderer"));
	}

	quader::ui::ViewportRenderResult render_frame(const quader::ui::ViewportRenderRequest &request) override {
		++render_calls;
		last_layout = request.layout_mode;
		last_pane_count = request.panes.size();
		last_camera_count = request.cameras.size();
		last_animation_enabled = request.prototype_animation_enabled;
		stats = quader::ui::ViewportFrameStats{
			request.surface_size.width,
			request.surface_size.height,
			static_cast<int>(request.panes.size()),
			60.0,
		};
		return quader::ui::ViewportRenderResult::success(QStringLiteral("FakeRenderer"));
	}

	void shutdown_surface() override {}

	std::optional<quader::ui::ViewportFrameStats> latest_frame_stats() const override {
		return stats;
	}

	std::optional<quader::ui::ViewportDiagnosticsSnapshot> latest_diagnostics() const override {
		return diagnostics;
	}

	QString renderer_name() const override {
		return QStringLiteral("FakeRenderer");
	}
};

TEST(Viewport, LayoutStateClampsQuadPanesAndHitTests) {
	quader::ui::ViewportLayoutState layout;
	EXPECT_EQ(layout.pane_count(), 1);

	const quader::ui::ViewportPaneArray kSingle = layout.panes_for({ 800, 600 });
	EXPECT_EQ(kSingle[0].rect.width, 800);
	EXPECT_EQ(kSingle[0].rect.height, 600);
	EXPECT_EQ(layout.pane_at({ 800, 600 }, { 20.0, 20.0 }), 0);

	layout.set_quad_enabled(true);
	layout.set_vertical_split(-1.0F);
	layout.set_horizontal_split(2.0F);
	const quader::ui::ViewportPaneArray kQuad = layout.panes_for({ 320, 240 });
	EXPECT_EQ(layout.pane_count(), 4);
	for (int index = 0; index < 4; ++index) {
		EXPECT_TRUE(kQuad[static_cast<std::size_t>(index)].rect.width > 0);
		EXPECT_TRUE(kQuad[static_cast<std::size_t>(index)].rect.height > 0);
	}

	EXPECT_TRUE(layout.pane_at({ 320, 240 }, { 10.0, 10.0 }) >= 0);
	EXPECT_NE(layout.splitter_at({ 320, 240 }, { 160.0, 10.0 }), quader::ui::ViewportSplitHandle::Horizontal);
}

TEST(Viewport, CameraControllerUpdatesFromSyntheticInputWithoutQtEvents) {
	quader::ui::ViewportCameraController cameras;
	const quader::ui::ViewportCameraSnapshot kBefore = cameras.camera_snapshots()[0];

	EXPECT_TRUE(cameras.begin_navigation(0, quader::ui::NavigationMode::Orbit, { 100.0, 100.0 }));
	cameras.update_navigation({ 140.0, 115.0 }, { 800, 600 });
	cameras.end_navigation();

	const quader::ui::ViewportCameraSnapshot kAfter = cameras.camera_snapshots()[0];
	EXPECT_TRUE(!quader::math::nearly_equal(kBefore.eye, kAfter.eye, 0.0001F));
	EXPECT_EQ(cameras.navigation_mode(), quader::ui::NavigationMode::None);

	const quader::ui::ViewportCameraSnapshot kTop = cameras.camera_snapshots()[1];
	EXPECT_EQ(kTop.projection, quader::ui::CameraProjection::Orthographic);
}

TEST(Viewport, ControllerForwardsSurfaceResizeAndQuadRenderRequests) {
	FakeRenderHost host;
	quader::ui::ViewportController controller(host);

	controller.initialize_surface(
			quader::ui::NativeViewportSurface{
					quader::ui::NativeViewportSurface::Platform::WindowsHwnd,
					reinterpret_cast<void *>(0x1),
			},
			{ 640, 480 },
			1.0);

	EXPECT_EQ(host.initialize_calls, 1);
	EXPECT_EQ(host.initialized_size.width, 640);
	EXPECT_EQ(host.initialized_size.height, 480);

	controller.resize_surface({ 800, 600 }, 1.0);
	EXPECT_EQ(host.resize_calls, 1);
	EXPECT_EQ(host.resized_size.width, 800);
	EXPECT_EQ(host.resized_size.height, 600);

	controller.set_quad_viewports_enabled(true);
	controller.render_frame(1.0, 0.016F);
	EXPECT_EQ(host.render_calls, 1);
	EXPECT_EQ(host.last_layout, quader::ui::ViewportLayoutMode::Quad);
	EXPECT_EQ(host.last_pane_count, 4U);
	EXPECT_EQ(host.last_camera_count, 4U);
	EXPECT_TRUE(host.last_animation_enabled);

	controller.set_prototype_animation_enabled(false);
	controller.render_frame(2.0, 0.016F);
	EXPECT_FALSE(host.last_animation_enabled);
}

TEST(Viewport, RenderResultCarriesTypedPickResultsWithoutSelectionMutation) {
	std::vector<quader::ui::ViewportPickResult> pick_results = {
		quader::ui::ViewportPickResult{
				.request_id = 9,
				.hit = true,
				.payload = quader::ui::ViewportPickPayload{
						.object_id = 42,
						.submesh_index = 2,
						.element_kind = quader::ui::ViewportPickElementKind::Object,
				},
		},
	};

	const quader::ui::ViewportRenderResult kResult = quader::ui::ViewportRenderResult::success(
			QStringLiteral("FakeRenderer"),
			std::move(pick_results));
	EXPECT_TRUE(kResult.ok);
	EXPECT_EQ(kResult.completed_pick_results.size(), 1U);
	EXPECT_EQ(kResult.completed_pick_results[0].request_id, 9U);
	EXPECT_TRUE(kResult.completed_pick_results[0].hit);
	EXPECT_EQ(kResult.completed_pick_results[0].payload.object_id, 42U);
}

TEST(Viewport, RenderHostCanExposeDiagnosticsSnapshotWithoutGpuHandles) {
	FakeRenderHost host;
	host.diagnostics = quader::ui::ViewportDiagnosticsSnapshot{
		.renderer_name = QStringLiteral("FakeRenderer"),
		.frame = quader::ui::ViewportFrameStats{
				.width = 320,
				.height = 200,
				.viewport_count = 1,
				.fps = 30.0,
		},
		.passes = {
				quader::ui::ViewportRenderPassStats{
						.pass_name = QStringLiteral("PresentPass"),
						.draw_call_count = 1,
						.draw_packet_count = 1,
				},
		},
		.counters = {
				quader::ui::ViewportRendererCounter{
						.domain = QStringLiteral("Frame"),
						.name = QStringLiteral("frame_index"),
						.value = 7,
						.unit = QStringLiteral("Frames"),
				},
		},
		.diagnostics = {
				quader::ui::ViewportRendererDiagnosticRow{
						.severity = QStringLiteral("Warning"),
						.code = QStringLiteral("GoldenCaptureUnsupported"),
						.subsystem = QStringLiteral("golden"),
						.resource_name = QStringLiteral("GoldenCapturePass"),
						.message = QStringLiteral("Capture unsupported"),
						.detail = QStringLiteral("No active readback path"),
						.frame_index = 7,
				},
		},
		.frame_graph_dump = QStringLiteral("FrameSetupPass -> PresentPass"),
	};

	const std::optional<quader::ui::ViewportDiagnosticsSnapshot> kDiagnostics = host.latest_diagnostics();
	if (!kDiagnostics.has_value()) {
		ADD_FAILURE() << "render host did not expose diagnostics";
		return;
	}
	const auto &diagnostics_snapshot = *kDiagnostics;
	EXPECT_EQ(diagnostics_snapshot.renderer_name, QStringLiteral("FakeRenderer"));
	EXPECT_EQ(diagnostics_snapshot.frame.width, 320);
	EXPECT_EQ(diagnostics_snapshot.passes.size(), 1);
	EXPECT_EQ(diagnostics_snapshot.counters.size(), 1);
	EXPECT_EQ(diagnostics_snapshot.diagnostics.size(), 1);
	EXPECT_EQ(diagnostics_snapshot.diagnostics[0].resource_name, QStringLiteral("GoldenCapturePass"));
}

TEST(Viewport, CrimsonDiagnosticsMappingPreservesStructuredFields) {
	crimson::RendererDiagnosticsSnapshot source;
	source.status.backend_name = "SyntheticBackend";
	source.latest_frame_stats = crimson::FrameStats{
		.frame_index = 11,
		.width_px = 1920,
		.height_px = 1080,
		.view_count = 4,
		.fps = 59.5,
	};
	source.pass_stats = {
		crimson::RenderPassStats{
				.pass_name = "OpaquePbrPass",
				.resource_read_count = 2,
				.resource_write_count = 1,
				.draw_call_count = 3,
				.draw_packet_count = 5,
				.cpu_time_us = 17,
		},
	};
	source.counters = {
		crimson::RendererCounter{
				.domain = crimson::RendererCounterDomain::Upload,
				.name = "uploaded_vertex_bytes",
				.value = 128,
				.unit = crimson::RendererMetricUnit::Bytes,
		},
	};
	source.recent_diagnostics = {
		crimson::RendererDiagnostic{
				.severity = crimson::RendererDiagnosticSeverity::Warning,
				.code = crimson::RendererDiagnosticCode::GoldenCaptureUnsupported,
				.message = "Capture unsupported",
				.detail = "Readback is not stable on this backend.",
				.subsystem = "golden",
				.resource_name = "GoldenCapturePass",
				.frame_index = 11,
		},
	};
	source.frame_graph_dump = "FrameSetupPass -> OpaquePbrPass -> PresentPass";

	const quader::ui::ViewportDiagnosticsSnapshot kMapped =
			quader::ui::viewport_diagnostics_from_crimson(source);

	EXPECT_EQ(kMapped.renderer_name, QStringLiteral("SyntheticBackend"));
	EXPECT_EQ(kMapped.frame.width, 1920);
	EXPECT_EQ(kMapped.frame.height, 1080);
	EXPECT_EQ(kMapped.frame.viewport_count, 4);
	EXPECT_EQ(kMapped.passes.size(), 1);
	EXPECT_EQ(kMapped.passes[0].pass_name, QStringLiteral("OpaquePbrPass"));
	EXPECT_EQ(kMapped.passes[0].resource_read_count, 2);
	EXPECT_EQ(kMapped.passes[0].resource_write_count, 1);
	EXPECT_EQ(kMapped.passes[0].draw_call_count, 3);
	EXPECT_EQ(kMapped.passes[0].draw_packet_count, 5);
	EXPECT_EQ(kMapped.passes[0].cpu_time_us, 17ULL);
	EXPECT_EQ(kMapped.counters.size(), 1);
	EXPECT_EQ(kMapped.counters[0].domain, QStringLiteral("Upload"));
	EXPECT_EQ(kMapped.counters[0].name, QStringLiteral("uploaded_vertex_bytes"));
	EXPECT_EQ(kMapped.counters[0].value, 128ULL);
	EXPECT_EQ(kMapped.counters[0].unit, QStringLiteral("Bytes"));
	EXPECT_EQ(kMapped.diagnostics.size(), 1);
	EXPECT_EQ(kMapped.diagnostics[0].severity, QStringLiteral("Warning"));
	EXPECT_EQ(kMapped.diagnostics[0].code, QStringLiteral("GoldenCaptureUnsupported"));
	EXPECT_EQ(kMapped.diagnostics[0].subsystem, QStringLiteral("golden"));
	EXPECT_EQ(kMapped.diagnostics[0].resource_name, QStringLiteral("GoldenCapturePass"));
	EXPECT_EQ(kMapped.diagnostics[0].frame_index, 11ULL);
	EXPECT_EQ(kMapped.frame_graph_dump, QStringLiteral("FrameSetupPass -> OpaquePbrPass -> PresentPass"));
}

} // namespace
