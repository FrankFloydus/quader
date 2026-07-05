/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "app/application.hpp"

#include "app/app_services.hpp"
#include "tools/box_tool.hpp"
#include "tools/tool.hpp"
#include "ui/actions/action_registry.hpp"
#include "ui/qt_app/main_window.hpp"
#include "ui/qt_app/qt_application_bootstrap.hpp"
#include "ui/style/fusion_style_policy.hpp"

#include <QApplication>

#include <memory>
#include <utility>

namespace quader::app {
namespace {

class ShellTool final : public quader::tools::ITool {
public:
	explicit ShellTool(quader::tools::ToolId id) noexcept
			: id_(id) {
	}

	[[nodiscard]] quader::tools::ToolId id() const noexcept override {
		return id_;
	}

private:
	quader::tools::ToolId id_;
};

void register_shell_tools(quader::tools::ToolManager &tool_manager) {
	(void)tool_manager.register_tool(std::make_unique<ShellTool>(quader::tools::ToolId::Select));
	(void)tool_manager.register_tool(std::make_unique<ShellTool>(quader::tools::ToolId::Move));
	(void)tool_manager.register_tool(std::make_unique<ShellTool>(quader::tools::ToolId::Rotate));
	(void)tool_manager.register_tool(std::make_unique<ShellTool>(quader::tools::ToolId::Scale));
	(void)tool_manager.register_tool(std::make_unique<quader::tools::BoxTool>());
	(void)tool_manager.set_active_tool(quader::tools::ToolId::Select);
}

} // namespace

AppServices::AppServices() : tool_manager(quader::tools::ToolContext{ document, command_history }), import_service(io_registry), export_service(io_registry), editor_state(document, command_history, tool_manager, io_registry), action_state_updater(actions, editor_state), settings(settings_store), document_ui(document, command_history, action_state_updater, notifications), import_ui(file_dialogs, io_registry, import_service, notifications), editor_actions(actions, document_ui, tool_manager, action_state_updater, notifications), ui_context{
			actions,
			action_state_updater,
			editor_state,
			settings,
			notifications,
			document_ui,
			import_ui,
			viewport_diagnostics,
			tool_manager,
		} {
	ui::register_standard_actions(actions);
	tool_manager.context().set_after_command_applied([this]() {
		document_ui.refresh_from_document();
	});
	tool_manager.set_after_active_tool_changed([this]() {
		action_state_updater.refresh();
	});
	register_shell_tools(tool_manager);
	action_state_updater.refresh();
}

Application::Application(QApplication &qt_app, ApplicationConfig config) : qt_app_(qt_app), config_(std::move(config)) {
	ui::apply_qt_application_metadata(
			qt_app_,
			ui::QtApplicationMetadata{
					QString::fromStdString(config_.application_name),
					QString::fromStdString(config_.organization_name),
			});

	const ui::FusionStylePolicy kFusionStyle;
	(void)kFusionStyle.apply(qt_app_);

	services_ = std::make_unique<AppServices>();
	main_window_ = std::make_unique<ui::MainWindow>(services_->ui_context);
}

Application::~Application() = default;

int Application::run() {
	main_window_->show();
	return qt_app_.exec();
}

} // namespace quader::app
