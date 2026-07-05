#pragma once

#include "ui/viewport/viewport_render_host.hpp"

#include <QObject>
#include <QString>

#include <optional>

namespace quader::ui {

struct ViewportDiagnosticsSummary {
    bool available = false;
    QString renderer_name;
    ViewportFrameStats frame;
    int pass_count = 0;
    int counter_count = 0;
    int diagnostic_count = 0;
    int warning_count = 0;
    int error_count = 0;
    QString status_text;
    QString tooltip_text;
};

class ViewportDiagnosticsService final : public QObject {
    Q_OBJECT

public:
    explicit ViewportDiagnosticsService(QObject* parent = nullptr);

    void attach_render_host(IViewportRenderHost& render_host) noexcept;
    void detach_render_host(const IViewportRenderHost& render_host) noexcept;
    void clear_render_host() noexcept;

    void refresh();

    [[nodiscard]] const std::optional<ViewportDiagnosticsSnapshot>& latest_snapshot() const noexcept;
    [[nodiscard]] ViewportDiagnosticsSummary summary() const;
    [[nodiscard]] bool has_provider() const noexcept;

Q_SIGNALS:
    void diagnostics_changed();
    void diagnostics_unavailable();
    void summary_changed();

private:
    void clear_snapshot_and_notify(bool emit_unavailable);

    IViewportRenderHost* render_host_ = nullptr;
    std::optional<ViewportDiagnosticsSnapshot> latest_snapshot_;
};

} // namespace quader::ui
