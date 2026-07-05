#include "ui/services/settings_service.hpp"

namespace quader::ui {
namespace {

constexpr auto kWorkspaceRoot = u"ui/workspace/v2";
constexpr auto kWorkspaceRootPrefix = u"ui/workspace/v2/";
constexpr auto kMainWindowGeometryKey = u"ui/workspace/v2/main_window/geometry";
constexpr auto kMainWindowStateKey = u"ui/workspace/v2/main_window/state";
constexpr auto kMainWindowStateVersionKey = u"ui/workspace/v2/main_window/state_version";
constexpr auto kViewportRoot = u"ui/workspace/v2/viewport/";
constexpr auto kPanelsRoot = u"ui/workspace/v2/panels/";

QString key_string(QStringView key)
{
    return key.toString();
}

} // namespace

SettingsService::SettingsService(QSettings& settings)
    : settings_(settings)
{
}

QVariant SettingsService::value(QStringView key, const QVariant& fallback) const
{
    return settings_.value(key_string(key), fallback);
}

void SettingsService::set_value(QStringView key, const QVariant& value)
{
    settings_.setValue(key_string(key), value);
}

void SettingsService::remove_group(QStringView group)
{
    settings_.beginGroup(key_string(group));
    settings_.remove(QString());
    settings_.endGroup();
}

void SettingsService::sync()
{
    settings_.sync();
}

QByteArray SettingsService::main_window_geometry() const
{
    return settings_.value(QString::fromUtf16(kMainWindowGeometryKey)).toByteArray();
}

void SettingsService::set_main_window_geometry(const QByteArray& geometry)
{
    settings_.setValue(QString::fromUtf16(kMainWindowGeometryKey), geometry);
}

QByteArray SettingsService::main_window_state() const
{
    const int state_version = settings_.value(QString::fromUtf16(kMainWindowStateVersionKey), 0).toInt();
    if (state_version != kWorkspaceStateVersion) {
        return {};
    }

    return settings_.value(QString::fromUtf16(kMainWindowStateKey)).toByteArray();
}

void SettingsService::set_main_window_state(const QByteArray& state)
{
    settings_.setValue(QString::fromUtf16(kMainWindowStateKey), state);
    settings_.setValue(QString::fromUtf16(kMainWindowStateVersionKey), kWorkspaceStateVersion);
}

QVariant SettingsService::viewport_value(QStringView key, const QVariant& fallback) const
{
    return value(QString::fromUtf16(kViewportRoot) + key.toString(), fallback);
}

void SettingsService::set_viewport_value(QStringView key, const QVariant& value)
{
    set_value(QString::fromUtf16(kViewportRoot) + key.toString(), value);
}

QVariant SettingsService::workspace_value(QStringView key, const QVariant& fallback) const
{
    return value(QString::fromUtf16(kWorkspaceRootPrefix) + key.toString(), fallback);
}

void SettingsService::set_workspace_value(QStringView key, const QVariant& value)
{
    set_value(QString::fromUtf16(kWorkspaceRootPrefix) + key.toString(), value);
}

bool SettingsService::panel_visible(PanelId id, bool fallback) const
{
    const QString key = QString::fromUtf16(kPanelsRoot) + panel_id_name(id) + QStringLiteral("/visible");
    return settings_.value(key, fallback).toBool();
}

void SettingsService::set_panel_visible(PanelId id, bool visible)
{
    const QString key = QString::fromUtf16(kPanelsRoot) + panel_id_name(id) + QStringLiteral("/visible");
    settings_.setValue(key, visible);
}

void SettingsService::reset_workspace()
{
    remove_group(QString::fromUtf16(kWorkspaceRoot));
}

} // namespace quader::ui
