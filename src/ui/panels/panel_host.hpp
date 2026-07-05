#pragma once

#include "ui/panels/panel_context.hpp"
#include "ui/panels/panel_id.hpp"

#include <QDockWidget>
#include <QMainWindow>
#include <QObject>

#include <unordered_map>

namespace quader::ui {

class ActionRegistry;
class SettingsService;
enum class ActionId : std::uint16_t;

struct PanelSpec {
	PanelId id = PanelId::Scene;
	QString title;
	Qt::DockWidgetArea default_area = Qt::RightDockWidgetArea;
	Qt::DockWidgetAreas allowed_areas = Qt::AllDockWidgetAreas;
	bool default_visible = true;
};

class PanelHost final : public QObject {
	Q_OBJECT

public:
	explicit PanelHost(QMainWindow &main_window, PanelContext context, QObject *parent = nullptr);

	QDockWidget &add_panel(PanelSpec spec, QWidget *panel_widget);
	[[nodiscard]] QDockWidget *dock(PanelId id) const;

	void show_panel(PanelId id);
	void hide_panel(PanelId id);
	void bind_panel_visibility_action(PanelId panel, ActionRegistry &actions, ActionId action);
	void save_workspace(SettingsService &settings) const;
	void restore_workspace(SettingsService &settings);
	void reset_to_default_layout();

private:
	QMainWindow &main_window_;
	PanelContext context_;
	std::unordered_map<PanelId, QDockWidget *> docks_;
	std::unordered_map<PanelId, PanelSpec> specs_;
};

} // namespace quader::ui
