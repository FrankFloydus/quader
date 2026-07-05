#include "ui/panels/panel_host.hpp"

#include "foundation/assert.hpp"
#include "ui/actions/action_registry.hpp"
#include "ui/services/settings_service.hpp"

#include <QAction>
#include <QSignalBlocker>

#include <utility>

namespace quader::ui {

PanelHost::PanelHost(QMainWindow& main_window, PanelContext context, QObject* parent)
    : QObject(parent)
    , main_window_(main_window)
    , context_(context)
{
}

QDockWidget& PanelHost::add_panel(PanelSpec spec, QWidget* panel_widget)
{
    QUADER_ASSERT(panel_widget != nullptr);
    QUADER_ASSERT(docks_.find(spec.id) == docks_.end());

    auto* dock = new QDockWidget(spec.title, &main_window_);
    dock->setObjectName(panel_object_name(spec.id));
    dock->setAllowedAreas(spec.allowed_areas);
    dock->setWidget(panel_widget);

    main_window_.addDockWidget(spec.default_area, dock);
    dock->setVisible(spec.default_visible);
    docks_.emplace(spec.id, dock);
    specs_.emplace(spec.id, std::move(spec));
    return *dock;
}

QDockWidget* PanelHost::dock(PanelId id) const
{
    const auto it = docks_.find(id);
    return it == docks_.end() ? nullptr : it->second;
}

void PanelHost::show_panel(PanelId id)
{
    if (auto* panel_dock = dock(id)) {
        panel_dock->show();
        panel_dock->raise();
    }
}

void PanelHost::hide_panel(PanelId id)
{
    if (auto* panel_dock = dock(id)) {
        panel_dock->hide();
    }
}

void PanelHost::bind_panel_visibility_action(PanelId panel, ActionRegistry& actions, ActionId action_id)
{
    auto* panel_dock = dock(panel);
    QUADER_ASSERT(panel_dock != nullptr);

    QAction& action = actions.action(action_id);
    action.setCheckable(true);
    {
        const QSignalBlocker blocker(&action);
        action.setChecked(!panel_dock->isHidden());
    }

    connect(&action, &QAction::toggled, panel_dock, [this, panel](bool checked) {
        if (checked) {
            show_panel(panel);
        } else {
            hide_panel(panel);
        }
    });

    connect(panel_dock, &QDockWidget::visibilityChanged, &action, [&action](bool visible) {
        const QSignalBlocker blocker(&action);
        action.setChecked(visible);
    });
}

void PanelHost::save_workspace(SettingsService& settings) const
{
    for (const auto& [panel, panel_dock] : docks_) {
        if (panel_dock != nullptr) {
            settings.set_panel_visible(panel, !panel_dock->isHidden());
        }
    }
}

void PanelHost::restore_workspace(SettingsService& settings)
{
    for (const auto& [panel, panel_dock] : docks_) {
        if (panel_dock == nullptr) {
            continue;
        }

        const auto spec = specs_.find(panel);
        const bool fallback_visible = spec == specs_.end() ? !panel_dock->isHidden() : spec->second.default_visible;
        const bool visible = settings.panel_visible(panel, fallback_visible);
        if (visible) {
            panel_dock->show();
        } else {
            panel_dock->hide();
        }
    }
}

void PanelHost::reset_to_default_layout()
{
    auto* scene = dock(PanelId::Scene);
    auto* properties = dock(PanelId::Properties);
    auto* diagnostics = dock(PanelId::Diagnostics);

    if (scene != nullptr) {
        main_window_.addDockWidget(Qt::RightDockWidgetArea, scene);
        scene->show();
        scene->raise();
    }

    if (properties != nullptr) {
        main_window_.addDockWidget(Qt::RightDockWidgetArea, properties);
        if (scene != nullptr) {
            main_window_.splitDockWidget(scene, properties, Qt::Vertical);
        }
        properties->show();
    }

    if (diagnostics != nullptr) {
        main_window_.addDockWidget(Qt::BottomDockWidgetArea, diagnostics);
        diagnostics->hide();
    }
}

} // namespace quader::ui
