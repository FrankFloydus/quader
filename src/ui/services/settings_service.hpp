#pragma once

#include "ui/panels/panel_id.hpp"

#include <QByteArray>
#include <QSettings>
#include <QStringView>
#include <QVariant>

namespace quader::ui {

class SettingsService final {
public:
    static constexpr int kWorkspaceStateVersion = 2;

    explicit SettingsService(QSettings& settings);

    [[nodiscard]] QVariant value(QStringView key, const QVariant& fallback = {}) const;
    void set_value(QStringView key, const QVariant& value);
    void remove_group(QStringView group);
    void sync();

    [[nodiscard]] QByteArray main_window_geometry() const;
    void set_main_window_geometry(const QByteArray& geometry);

    [[nodiscard]] QByteArray main_window_state() const;
    void set_main_window_state(const QByteArray& state);

    [[nodiscard]] QVariant viewport_value(QStringView key, const QVariant& fallback = {}) const;
    void set_viewport_value(QStringView key, const QVariant& value);

    [[nodiscard]] QVariant workspace_value(QStringView key, const QVariant& fallback = {}) const;
    void set_workspace_value(QStringView key, const QVariant& value);

    [[nodiscard]] bool panel_visible(PanelId id, bool fallback) const;
    void set_panel_visible(PanelId id, bool visible);

    void reset_workspace();

private:
    QSettings& settings_;
};

} // namespace quader::ui
