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

#include "crimson/frame/frame_stats.hpp"

#include <chrono>
#include <cstdint>

namespace crimson {

/// RAII CPU timer that writes elapsed microseconds into a counter.
class ScopedCpuTimer final {
public:
	/**
	 * Start a timer.
	 *
	 * @param destination Counter that receives elapsed microseconds at destruction.
	 */
	explicit ScopedCpuTimer(std::uint64_t &destination) noexcept;
	/// Stop the timer and write elapsed microseconds.
	~ScopedCpuTimer();

	/// Timers cannot be copied because they own one timing scope.
	ScopedCpuTimer(const ScopedCpuTimer &) = delete;
	/// Timers cannot be copied because they own one timing scope.
	ScopedCpuTimer &operator=(const ScopedCpuTimer &) = delete;

private:
	std::uint64_t &destination_;
	std::chrono::steady_clock::time_point start_;
};

} // namespace crimson
