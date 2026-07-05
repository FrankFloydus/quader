#pragma once

#include "commands/command_result.hpp"
#include "document/document.hpp"

#include <memory>
#include <string_view>

namespace quader::commands {

class ICommand {
public:
	virtual ~ICommand() = default;

	[[nodiscard]] virtual std::string_view name() const noexcept = 0;
	[[nodiscard]] virtual CommandResult execute(quader::document::Document &document) = 0;
	[[nodiscard]] virtual CommandResult undo(quader::document::Document &document) = 0;

	[[nodiscard]] virtual bool can_merge_with(const ICommand &next) const noexcept;
	[[nodiscard]] virtual CommandResult merge_with(std::unique_ptr<ICommand> next);
};

} // namespace quader::commands
