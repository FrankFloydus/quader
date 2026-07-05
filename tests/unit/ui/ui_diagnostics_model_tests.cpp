/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/models/diagnostics_item_model.hpp"
#include "ui/services/viewport_diagnostics_service.hpp"

#include <gtest/gtest.h>

#include <QAbstractItemModelTester>
#include <QApplication>
#include <QModelIndex>
#include <QString>
#include <QVariant>

#include <cstdio>
#include <optional>

namespace {

struct FakeRenderHost final : quader::ui::IViewportRenderHost {
	std::optional<quader::ui::ViewportDiagnosticsSnapshot> diagnostics;

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
				.width = 1920,
				.height = 1080,
				.viewport_count = 4,
				.fps = 59.5,
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
						.frame_index = 7,
				},
		},
		.frame_graph_dump = QStringLiteral("FrameSetupPass -> OpaquePbrPass -> PresentPass"),
	};
}

QModelIndex section(quader::ui::DiagnosticsItemModel &model, int row) {
	return model.index(row, static_cast<int>(quader::ui::DiagnosticsItemColumn::Name));
}

QModelIndex child(
		quader::ui::DiagnosticsItemModel &model,
		const QModelIndex &parent,
		int row,
		quader::ui::DiagnosticsItemColumn column = quader::ui::DiagnosticsItemColumn::Name) {
	return model.index(row, static_cast<int>(column), parent);
}

TEST(UiDiagnosticsModel, EmptyModelHasStableSectionsAndUnavailableSummary) {
	quader::ui::ViewportDiagnosticsService service;
	quader::ui::DiagnosticsItemModel model(service);
	QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	EXPECT_EQ(model.rowCount(), 4);
	EXPECT_EQ(model.columnCount(), 3);
	EXPECT_EQ(section(model, 0).data(Qt::DisplayRole).toString(), QStringLiteral("Summary"));
	EXPECT_EQ(section(model, 1).data(Qt::DisplayRole).toString(), QStringLiteral("Render Passes"));
	EXPECT_EQ(section(model, 2).data(Qt::DisplayRole).toString(), QStringLiteral("Counters"));
	EXPECT_EQ(section(model, 3).data(Qt::DisplayRole).toString(), QStringLiteral("Diagnostics"));
	EXPECT_EQ(model.rowCount(section(model, 0)), 1);
	EXPECT_EQ(child(model, section(model, 0), 0).data(Qt::DisplayRole).toString(), QStringLiteral("Status"));
	EXPECT_FALSE((model.flags(child(model, section(model, 0), 0)) & Qt::ItemIsEditable) != 0);
}

TEST(UiDiagnosticsModel, SnapshotModelExposesSectionsRowsRolesAndParentage) {
	FakeRenderHost host;
	host.diagnostics = make_snapshot();
	quader::ui::ViewportDiagnosticsService service;
	service.attach_render_host(host);
	quader::ui::DiagnosticsItemModel model(service);
	QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	service.refresh();

	const QModelIndex kSummary = section(model, 0);
	const QModelIndex kPasses = section(model, 1);
	const QModelIndex kCounters = section(model, 2);
	const QModelIndex kDiagnostics = section(model, 3);

	EXPECT_EQ(model.rowCount(kSummary), 8);
	EXPECT_EQ(model.rowCount(kPasses), 1);
	EXPECT_EQ(model.rowCount(kCounters), 1);
	EXPECT_EQ(model.rowCount(kDiagnostics), 1);
	EXPECT_EQ(model.parent(child(model, kPasses, 0)), kPasses);

	const QModelIndex kPass = child(model, kPasses, 0);
	EXPECT_EQ(kPass.data(Qt::DisplayRole).toString(), QStringLiteral("OpaquePbrPass"));
	EXPECT_EQ(kPass.data(quader::ui::diagnostics_item_roles::kNodeKindRole).value<quader::ui::DiagnosticsNodeKind>(),
			quader::ui::DiagnosticsNodeKind::RenderPass);
	EXPECT_TRUE(kPass.data(quader::ui::diagnostics_item_roles::kCopyTextRole).toString().contains(QStringLiteral("OpaquePbrPass")));

	const QModelIndex kCounterValue = child(model, kCounters, 0, quader::ui::DiagnosticsItemColumn::Value);
	EXPECT_EQ(kCounterValue.data(Qt::DisplayRole).toString(), QStringLiteral("128 Bytes"));

	const QModelIndex kDiagnostic = child(model, kDiagnostics, 0);
	EXPECT_EQ(kDiagnostic.data(quader::ui::diagnostics_item_roles::kSeverityRole).toString(), QStringLiteral("Warning"));
	EXPECT_EQ(kDiagnostic.data(quader::ui::diagnostics_item_roles::kCategoryRole).toString(), QStringLiteral("golden"));
	EXPECT_EQ(kDiagnostic.data(quader::ui::diagnostics_item_roles::kFrameIndexRole).toULongLong(), 7ULL);
	EXPECT_TRUE(kDiagnostic.data(quader::ui::diagnostics_item_roles::kCopyTextRole)
					.toString()
					.contains(QStringLiteral("No active readback path")));
}

TEST(UiDiagnosticsModel, CopyTextHelpersIncludeUsefulDiagnostics) {
	FakeRenderHost host;
	host.diagnostics = make_snapshot();
	quader::ui::ViewportDiagnosticsService service;
	service.attach_render_host(host);
	service.refresh();

	quader::ui::DiagnosticsItemModel model(service);
	const QModelIndex kDiagnostic = child(model, section(model, 3), 0);
	const QString kDiagnosticCopy = model.copy_text_for_index(kDiagnostic);
	EXPECT_TRUE(kDiagnosticCopy.contains(QStringLiteral("GoldenCaptureUnsupported")));
	EXPECT_TRUE(kDiagnosticCopy.contains(QStringLiteral("GoldenCapturePass")));

	const QString kAllCopy = model.copy_text_for_all();
	EXPECT_TRUE(kAllCopy.contains(QStringLiteral("Render Passes")));
	EXPECT_TRUE(kAllCopy.contains(QStringLiteral("uploaded_vertex_bytes")));
	EXPECT_TRUE(kAllCopy.contains(QStringLiteral("Capture unsupported")));
}

} // namespace
