/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "tools/tool_context.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace quader::tools {

ToolContext::ToolContext(quader::document::Document &document,
		quader::commands::CommandHistory &command_history) noexcept
		: document_(document),
		  command_history_(command_history) {
}

const quader::document::Document &ToolContext::document() const noexcept {
	return document_;
}

quader::commands::CommandHistory &ToolContext::command_history() noexcept {
	return command_history_;
}

const quader::commands::CommandHistory &ToolContext::command_history() const noexcept {
	return command_history_;
}

quader::commands::CommandResult ToolContext::execute_command(
		std::unique_ptr<quader::commands::ICommand> command) {
	auto result = command_history_.execute(std::move(command), document_);
	if (result.is_applied() && after_command_applied_) {
		after_command_applied_();
	}
	return result;
}

quader::commands::CommandResult ToolContext::preview_object_transforms(
		std::span<const ToolContextTransformPreviewEdit> edits) {
	if (edits.empty()) {
		return quader::commands::CommandResult::rejected("preview transform batch has no object edits");
	}

	std::vector<quader::document::ObjectId> seen;
	seen.reserve(edits.size());
	bool changed = false;
	for (const ToolContextTransformPreviewEdit &edit : edits) {
		if (!edit.object.is_valid()) {
			return quader::commands::CommandResult::rejected("preview transform batch contains an invalid object id");
		}
		if (!quader::document::is_finite(edit.transform)) {
			return quader::commands::CommandResult::rejected("preview transform batch contains a non-finite transform");
		}
		if (std::find(seen.begin(), seen.end(), edit.object) != seen.end()) {
			return quader::commands::CommandResult::rejected("preview transform batch contains duplicate object ids");
		}
		seen.push_back(edit.object);

		const auto *object = document_.find_mesh_object(edit.object);
		if (object == nullptr) {
			return quader::commands::CommandResult::rejected("preview transform batch references an unknown object");
		}
		changed = changed || !(object->transform == edit.transform);
	}

	for (const ToolContextTransformPreviewEdit &edit : edits) {
		auto result = document_.set_preview_transform(edit.object, edit.transform);
		if (!result) {
			return quader::commands::CommandResult::rejected(std::move(result).error().diagnostic);
		}
	}

	if (changed && after_preview_mutation_applied_) {
		after_preview_mutation_applied_();
	}
	return quader::commands::CommandResult::applied();
}

void ToolContext::set_after_command_applied(std::function<void()> callback) {
	after_command_applied_ = std::move(callback);
}

void ToolContext::set_after_preview_mutation_applied(std::function<void()> callback) {
	after_preview_mutation_applied_ = std::move(callback);
}

} // namespace quader::tools
