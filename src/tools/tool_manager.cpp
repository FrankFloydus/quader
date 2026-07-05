#include "tools/tool_manager.hpp"

#include <utility>

namespace quader::tools {

ToolManager::ToolManager(ToolContext context) : context_(context) {
}

bool ToolManager::register_tool(std::unique_ptr<ITool> tool) {
	if (!tool) {
		return false;
	}

	const auto kId = tool->id();
	if (tools_.contains(kId)) {
		return false;
	}

	tools_.emplace(kId, std::move(tool));
	return true;
}

bool ToolManager::has_tool(ToolId id) const noexcept {
	return tools_.contains(id);
}

std::optional<ToolId> ToolManager::active_tool_id() const noexcept {
	if (active_tool_ == nullptr) {
		return std::nullopt;
	}

	return active_tool_->id();
}

ITool *ToolManager::active_tool() noexcept {
	return active_tool_;
}

const ITool *ToolManager::active_tool() const noexcept {
	return active_tool_;
}

bool ToolManager::set_active_tool(ToolId id) {
	const auto kIterator = tools_.find(id);
	if (kIterator == tools_.end()) {
		return false;
	}

	auto *next_tool = kIterator->second.get();
	if (active_tool_ == next_tool) {
		return true;
	}

	if (active_tool_ != nullptr) {
		active_tool_->deactivate(context_);
	}

	active_tool_ = next_tool;
	active_tool_->activate(context_);
	return true;
}

void ToolManager::clear_active_tool() {
	if (active_tool_ == nullptr) {
		return;
	}

	active_tool_->deactivate(context_);
	active_tool_ = nullptr;
}

bool ToolManager::cancel_active_tool() {
	if (active_tool_ == nullptr) {
		return false;
	}

	active_tool_->cancel(context_);
	return true;
}

bool ToolManager::dispatch_pointer_event(const PointerEvent &event) {
	if (active_tool_ == nullptr) {
		return false;
	}

	return active_tool_->on_pointer_event(event, context_);
}

bool ToolManager::dispatch_key_event(const KeyEvent &event) {
	if (active_tool_ == nullptr) {
		return false;
	}

	return active_tool_->on_key_event(event, context_);
}

ToolPreview ToolManager::preview() const {
	if (active_tool_ == nullptr) {
		return {};
	}

	return active_tool_->preview();
}

ToolContext &ToolManager::context() noexcept {
	return context_;
}

const ToolContext &ToolManager::context() const noexcept {
	return context_;
}

} // namespace quader::tools
