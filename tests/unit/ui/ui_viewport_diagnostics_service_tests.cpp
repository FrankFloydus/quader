/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/services/viewport_diagnostics_service.hpp"

#include <gtest/gtest.h>

#include <QApplication>

#include <cstdio>
#include <optional>
#include <vector>

namespace {

struct FakeRenderHost final : quader::ui::IViewportRenderHost {
	std::optional<quader::ui::ViewportDiagnosticsSnapshot> diagnostics;
	int diagnostics_calls = 0;

	quader::ui::ViewportRenderResult initialize_surface(
			const quader::ui::NativeViewportSurface &,
			quader::ui::ViewportPixelSize,
			double) override {
		return quader::ui::ViewportRenderResult::success(QStringLiteral("FakeRenderer"));
	}

	quader::ui::ViewportRenderResult resize_surface(quader::ui::ViewportPixelSize, double) override {
		return quader::ui::ViewportRenderResult::success(QStringLiteral("FakeRenderer"));
	}

	quader::ui::ViewportRenderResult render_frame(const quader::ui::ViewportRenderRequest &) override {
		return quader::ui::ViewportRenderResult::success(QStringLiteral("FakeRenderer"));
	}

	void shutdown_surface() override {}

	std::optional<quader::ui::ViewportFrameStats> latest_frame_stats() const override {
		return std::nullopt;
	}

	std::optional<quader::ui::ViewportDiagnosticsSnapshot> latest_diagnostics() const override {
		++const_cast<FakeRenderHost *>(this)->diagnostics_calls;
		return diagnostics;
	}

	QString renderer_name() const override {
		return QStringLiteral("FakeRenderer");
	}
};

quader::ui::ViewportDiagnosticsSnapshot make_snapshot() {
	return quader::ui::ViewportDiagnosticsSnapshot{
		.renderer_name = QStringLiteral("FakeRenderer"),
		.frame = quader::ui::ViewportFrameStats{
				.width = 1280,
				.height = 720,
				.viewport_count = 1,
				.fps = 60.0,
		},
		.passes = {
				quader::ui::ViewportRenderPassStats{
						.pass_name = QStringLiteral("OpaquePbrPass"),
						.resource_read_count = 2,
						.resource_write_count = 1,
						.draw_call_count = 3,
						.draw_packet_count = 5,
						.cpu_time_us = 17,
				},
		},
		.counters = {
				quader::ui::ViewportRendererCounter{
						.domain = QStringLiteral("Upload"),
						.name = QStringLiteral("uploaded_vertex_bytes"),
						.value = 128,
						.unit = QStringLiteral("Bytes"),
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
						.frame_index = 42,
				},
				quader::ui::ViewportRendererDiagnosticRow{
						.severity = QStringLiteral("Error"),
						.code = QStringLiteral("BackendUnavailable"),
						.subsystem = QStringLiteral("device"),
						.message = QStringLiteral("Backend unavailable"),
						.frame_index = 43,
				},
		},
		.frame_graph_dump = QStringLiteral("FrameSetupPass -> OpaquePbrPass -> PresentPass"),
	};
}

TEST(UiViewportDiagnosticsService, RefreshWithoutProviderReportsUnavailable) {
	quader::ui::ViewportDiagnosticsService service;
	service.refresh();

	EXPECT_FALSE(service.has_provider());
	EXPECT_FALSE(service.latest_snapshot().has_value());
	const quader::ui::ViewportDiagnosticsSummary kSummary = service.summary();
	EXPECT_FALSE(kSummary.available);
	EXPECT_EQ(kSummary.status_text, QStringLiteral("Diagnostics: unavailable"));
}

TEST(UiViewportDiagnosticsService, AttachedProviderSnapshotIsCachedAndSummarized) {
	FakeRenderHost host;
	host.diagnostics = make_snapshot();

	quader::ui::ViewportDiagnosticsService service;
	service.attach_render_host(host);
	service.refresh();

	EXPECT_TRUE(service.has_provider());
	EXPECT_EQ(host.diagnostics_calls, 1);
	const auto kSnapshot = service.latest_snapshot();
	if (!kSnapshot.has_value()) {
		ADD_FAILURE() << "diagnostics snapshot was not cached";
		return;
	}
	EXPECT_EQ(kSnapshot->renderer_name, QStringLiteral("FakeRenderer"));

	const quader::ui::ViewportDiagnosticsSummary kSummary = service.summary();
	EXPECT_TRUE(kSummary.available);
	EXPECT_EQ(kSummary.renderer_name, QStringLiteral("FakeRenderer"));
	EXPECT_EQ(kSummary.pass_count, 1);
	EXPECT_EQ(kSummary.counter_count, 1);
	EXPECT_EQ(kSummary.diagnostic_count, 2);
	EXPECT_EQ(kSummary.warning_count, 1);
	EXPECT_EQ(kSummary.error_count, 1);
	EXPECT_TRUE(kSummary.status_text.contains(QStringLiteral("1 warning")));
	EXPECT_TRUE(kSummary.tooltip_text.contains(QStringLiteral("Capture unsupported")));
}

TEST(UiViewportDiagnosticsService, NullProviderSnapshotClearsCachedState) {
	FakeRenderHost host;
	host.diagnostics = make_snapshot();

	quader::ui::ViewportDiagnosticsService service;
	service.attach_render_host(host);
	service.refresh();
	EXPECT_TRUE(service.latest_snapshot().has_value());

	host.diagnostics.reset();
	service.refresh();
	EXPECT_FALSE(service.latest_snapshot().has_value());
	EXPECT_EQ(service.summary().status_text, QStringLiteral("Diagnostics: unavailable"));
}

TEST(UiViewportDiagnosticsService, DetachClearsProviderAndSnapshot) {
	FakeRenderHost host;
	host.diagnostics = make_snapshot();

	quader::ui::ViewportDiagnosticsService service;
	service.attach_render_host(host);
	service.refresh();
	EXPECT_TRUE(service.latest_snapshot().has_value());

	service.detach_render_host(host);
	EXPECT_FALSE(service.has_provider());
	EXPECT_FALSE(service.latest_snapshot().has_value());
}

} // namespace
