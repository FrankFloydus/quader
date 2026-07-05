#include "commands/command.hpp"

namespace quader::commands {

bool ICommand::can_merge_with(const ICommand &next) const noexcept {
	(void)next;
	return false;
}

CommandResult ICommand::merge_with(std::unique_ptr<ICommand> next) {
	(void)next;
	return CommandResult::not_mergeable();
}

} // namespace quader::commands
