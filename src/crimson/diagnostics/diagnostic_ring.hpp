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

#include "crimson/renderer_diagnostics.hpp"

#include <cstddef>
#include <vector>

namespace crimson {

/// Fixed-capacity ring buffer of renderer diagnostics.
class DiagnosticRing final {
public:
	/**
	 * Create a diagnostic ring.
	 *
	 * @param capacity Maximum number of diagnostics retained.
	 */
	explicit DiagnosticRing(std::size_t capacity = 256);

	/**
	 * Push a diagnostic, overwriting the oldest entry when full.
	 *
	 * @param diagnostic Diagnostic to store.
	 */
	void push(RendererDiagnostic diagnostic);
	/**
	 * Return diagnostics in oldest-to-newest order.
	 *
	 * @return Copied diagnostic list.
	 */
	[[nodiscard]] std::vector<RendererDiagnostic> snapshot() const;
	/// Remove all retained diagnostics.
	void clear() noexcept;

private:
	std::vector<RendererDiagnostic> diagnostics_;
	std::size_t capacity_ = 0;
	std::size_t next_index_ = 0;
	bool wrapped_ = false;
};

/**
 * Build a bounded ring from an existing diagnostic list.
 *
 * @param diagnostics Diagnostics copied into the ring.
 * @param capacity Maximum number of diagnostics retained.
 * @return Ring containing the most recent diagnostics up to `capacity`.
 */
[[nodiscard]] DiagnosticRing make_diagnostic_ring_from_recent(
		const std::vector<RendererDiagnostic> &diagnostics,
		std::size_t capacity = 256);

} // namespace crimson
