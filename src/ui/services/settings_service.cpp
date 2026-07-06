/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/services/settings_service.hpp"

namespace quader::ui {
namespace {

constexpr auto kWorkspaceRoot = u"ui/workspace/v2";
constexpr auto kWorkspaceRootPrefix = u"ui/workspace/v2/";
constexpr auto kMainWindowGeometryKey = u"ui/workspace/v2/main_window/geometry";
constexpr auto kMainWindowStateKey = u"ui/workspace/v2/main_window/state";
constexpr auto kMainWindowStateVersionKey = u"ui/workspace/v2/main_window/state_version";
constexpr auto kViewportRoot = u"ui/workspace/v2/viewport/";
constexpr auto kViewportShowGridKey = u"show_grid";
constexpr auto kViewportShowOverlaysKey = u"show_overlays";
constexpr auto kViewportShowMeshGridKey = u"show_mesh_grid";
constexpr auto kPanelsRoot = u"ui/workspace/v2/panels/";

QString key_string(QStringView key) {
	return key.toString();
}

} // namespace

SettingsService::SettingsService(QSettings &settings) : settings_(settings) {
}

QVariant SettingsService::value(QStringView key, const QVariant &fallback) const {
	return settings_.value(key_string(key), fallback);
}

void SettingsService::set_value(QStringView key, const QVariant &value) {
	settings_.setValue(key_string(key), value);
}

void SettingsService::remove_group(QStringView group) {
	settings_.beginGroup(key_string(group));
	settings_.remove(QString());
	settings_.endGroup();
}

void SettingsService::sync() {
	settings_.sync();
}

QByteArray SettingsService::main_window_geometry() const {
	return settings_.value(QString::fromUtf16(kMainWindowGeometryKey)).toByteArray();
}

void SettingsService::set_main_window_geometry(const QByteArray &geometry) {
	settings_.setValue(QString::fromUtf16(kMainWindowGeometryKey), geometry);
}

QByteArray SettingsService::main_window_state() const {
	const int kStateVersion = settings_.value(QString::fromUtf16(kMainWindowStateVersionKey), 0).toInt();
	if (kStateVersion != kWorkspaceStateVersion) {
		return {};
	}

	return settings_.value(QString::fromUtf16(kMainWindowStateKey)).toByteArray();
}

void SettingsService::set_main_window_state(const QByteArray &state) {
	settings_.setValue(QString::fromUtf16(kMainWindowStateKey), state);
	settings_.setValue(QString::fromUtf16(kMainWindowStateVersionKey), kWorkspaceStateVersion);
}

QVariant SettingsService::viewport_value(QStringView key, const QVariant &fallback) const {
	return value(QString::fromUtf16(kViewportRoot) + key.toString(), fallback);
}

void SettingsService::set_viewport_value(QStringView key, const QVariant &value) {
	set_value(QString::fromUtf16(kViewportRoot) + key.toString(), value);
}

ViewportDisplaySettings SettingsService::viewport_display_settings() const {
	ViewportDisplaySettings settings;
	settings.show_grid = viewport_value(QString::fromUtf16(kViewportShowGridKey), settings.show_grid).toBool();
	settings.show_overlays = viewport_value(QString::fromUtf16(kViewportShowOverlaysKey), settings.show_overlays).toBool();
	settings.show_mesh_grid = viewport_value(QString::fromUtf16(kViewportShowMeshGridKey), settings.show_mesh_grid).toBool();
	return settings;
}

void SettingsService::set_viewport_display_settings(const ViewportDisplaySettings &settings) {
	set_viewport_value(QString::fromUtf16(kViewportShowGridKey), settings.show_grid);
	set_viewport_value(QString::fromUtf16(kViewportShowOverlaysKey), settings.show_overlays);
	set_viewport_value(QString::fromUtf16(kViewportShowMeshGridKey), settings.show_mesh_grid);
}

QVariant SettingsService::workspace_value(QStringView key, const QVariant &fallback) const {
	return value(QString::fromUtf16(kWorkspaceRootPrefix) + key.toString(), fallback);
}

void SettingsService::set_workspace_value(QStringView key, const QVariant &value) {
	set_value(QString::fromUtf16(kWorkspaceRootPrefix) + key.toString(), value);
}

bool SettingsService::panel_visible(PanelId id, bool fallback) const {
	const QString kEy = QString::fromUtf16(kPanelsRoot) + panel_id_name(id) + QStringLiteral("/visible");
	return settings_.value(kEy, fallback).toBool();
}

void SettingsService::set_panel_visible(PanelId id, bool visible) {
	const QString kEy = QString::fromUtf16(kPanelsRoot) + panel_id_name(id) + QStringLiteral("/visible");
	settings_.setValue(kEy, visible);
}

void SettingsService::reset_workspace() {
	remove_group(QString::fromUtf16(kWorkspaceRoot));
}

} // namespace quader::ui
