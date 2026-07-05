#include "io/io_diagnostic.hpp"

#include <utility>

namespace quader::io {

void IoDiagnosticList::add(IoDiagnostic diagnostic) {
	diagnostics_.push_back(std::move(diagnostic));
}

void IoDiagnosticList::add(IoSeverity severity, IoDiagnosticCode code, std::string message) {
	add(IoDiagnostic{ severity, code, std::move(message) });
}

void IoDiagnosticList::add_error(IoDiagnosticCode code, std::string message) {
	add(IoSeverity::Error, code, std::move(message));
}

void IoDiagnosticList::append(const IoDiagnosticList &diagnostics) {
	diagnostics_.insert(diagnostics_.end(), diagnostics.begin(), diagnostics.end());
}

void IoDiagnosticList::clear() noexcept {
	diagnostics_.clear();
}

bool IoDiagnosticList::empty() const noexcept {
	return diagnostics_.empty();
}

bool IoDiagnosticList::has_errors() const noexcept {
	for (const auto &diagnostic : diagnostics_) {
		if (diagnostic.severity == IoSeverity::Error) {
			return true;
		}
	}

	return false;
}

std::size_t IoDiagnosticList::size() const noexcept {
	return diagnostics_.size();
}

std::span<const IoDiagnostic> IoDiagnosticList::diagnostics() const noexcept {
	return diagnostics_;
}

const IoDiagnostic &IoDiagnosticList::operator[](std::size_t index) const noexcept {
	return diagnostics_[index];
}

IoDiagnosticList::const_iterator IoDiagnosticList::begin() const noexcept {
	return diagnostics_.begin();
}

IoDiagnosticList::const_iterator IoDiagnosticList::end() const noexcept {
	return diagnostics_.end();
}

quader::foundation::Severity to_foundation_severity(IoSeverity severity) noexcept {
	switch (severity) {
		case IoSeverity::Info:
			return quader::foundation::Severity::Info;
		case IoSeverity::Warning:
			return quader::foundation::Severity::Warning;
		case IoSeverity::Error:
			return quader::foundation::Severity::Error;
	}

	return quader::foundation::Severity::Error;
}

quader::foundation::Diagnostic to_foundation_diagnostic(const IoDiagnostic &diagnostic) {
	return quader::foundation::Diagnostic{
		to_foundation_severity(diagnostic.severity),
		diagnostic.message,
	};
}

quader::foundation::DiagnosticList to_foundation_diagnostics(
		const IoDiagnosticList &diagnostics) {
	quader::foundation::DiagnosticList result;
	for (const auto &diagnostic : diagnostics) {
		result.add(to_foundation_diagnostic(diagnostic));
	}

	return result;
}

} // namespace quader::io
