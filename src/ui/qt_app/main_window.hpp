#pragma once

#include "ui/actions/editor_state_snapshot.hpp"
#include "ui/qt_app/ui_context.hpp"
#include "ui/viewport/viewport_types.hpp"

#include <memory>

#include <QMainWindow>

class QLabel;
class QCloseEvent;

namespace quader::ui {

class IViewportRenderHost;
class PanelHost;
class ViewportController;
class ViewportWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(UiContext& context, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void build_menus();
    void build_central_area();
    void build_dock_hosts();
    void build_status_ui();
    void connect_actions();
    void connect_notifications();
    void connect_viewport();
    void restore_workspace();
    void save_workspace();
    void reset_workspace();
    [[nodiscard]] EditorStateSnapshot editor_state_with_viewport() const;
    void update_stats_label(const ViewportFrameStats& stats);
    void update_diagnostics_label();

    UiContext& context_;
    std::unique_ptr<IViewportRenderHost> viewport_render_host_;
    std::unique_ptr<ViewportController> viewport_controller_;
    PanelHost* panel_host_ = nullptr;
    ViewportWidget* viewport_ = nullptr;
    QLabel* renderer_label_ = nullptr;
    QLabel* stats_label_ = nullptr;
    QLabel* diagnostics_label_ = nullptr;
    bool quad_viewports_enabled_ = false;
};

} // namespace quader::ui
