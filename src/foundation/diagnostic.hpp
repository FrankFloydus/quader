/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace quader::foundation {

/// Severity level for developer-facing diagnostics.
enum class Severity {
	/// Informational diagnostic with no failure implied.
	Info,
	/// Recoverable or noteworthy condition.
	Warning,
	/// Error condition that should block the current operation.
	Error,
};

/// One diagnostic message with a severity.
struct Diagnostic {
	/// Diagnostic severity.
	Severity severity = Severity::Info;
	/// Human-readable diagnostic message.
	std::string message;
};

/**
 * Ordered collection of diagnostics.
 *
 * The list owns diagnostic messages and exposes read-only spans and iterators
 * for callers that need to inspect or forward them.
 */
class DiagnosticList final {
public:
	/// Container type used to store diagnostics.
	using container_type = std::vector<Diagnostic>;
	/// Constant iterator over stored diagnostics.
	using const_iterator = container_type::const_iterator;

	/**
	 * Append a diagnostic.
	 *
	 * @param diagnostic Diagnostic to take ownership of.
	 */
	void add(Diagnostic diagnostic) {
		diagnostics_.push_back(std::move(diagnostic));
	}

	/**
	 * Append a diagnostic from severity and message parts.
	 *
	 * @param severity Severity assigned to the diagnostic.
	 * @param message Message to store.
	 */
	void add(Severity severity, std::string message) {
		add(Diagnostic{ severity, std::move(message) });
	}

	/// Remove all diagnostics.
	void clear() noexcept {
		diagnostics_.clear();
	}

	/**
	 * Check whether no diagnostics are stored.
	 *
	 * @return True when the list is empty.
	 */
	[[nodiscard]] bool empty() const noexcept {
		return diagnostics_.empty();
	}

	/**
	 * Return the number of stored diagnostics.
	 *
	 * @return Diagnostic count.
	 */
	[[nodiscard]] std::size_t size() const noexcept {
		return diagnostics_.size();
	}

	/**
	 * Check whether the list contains any error diagnostic.
	 *
	 * @return True when at least one diagnostic has `Severity::Error`.
	 */
	[[nodiscard]] bool has_errors() const noexcept {
		for (const auto &diagnostic : diagnostics_) {
			if (diagnostic.severity == Severity::Error) {
				return true;
			}
		}

		return false;
	}

	/**
	 * Return all diagnostics as a borrowed span.
	 *
	 * @return Span valid until the list is mutated or destroyed.
	 */
	[[nodiscard]] std::span<const Diagnostic> diagnostics() const noexcept {
		return diagnostics_;
	}

	/**
	 * Access a diagnostic by index.
	 *
	 * @param index Zero-based diagnostic index.
	 * @return Diagnostic at `index`.
	 */
	[[nodiscard]] const Diagnostic &operator[](std::size_t index) const noexcept {
		return diagnostics_[index];
	}

	/**
	 * Return an iterator to the first diagnostic.
	 *
	 * @return Constant begin iterator.
	 */
	[[nodiscard]] const_iterator begin() const noexcept {
		return diagnostics_.begin();
	}

	/**
	 * Return an iterator one past the last diagnostic.
	 *
	 * @return Constant end iterator.
	 */
	[[nodiscard]] const_iterator end() const noexcept {
		return diagnostics_.end();
	}

private:
	container_type diagnostics_;
};

} // namespace quader::foundation
