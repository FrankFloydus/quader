/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/diagnostics/frame_profiler.hpp"

namespace crimson {

ScopedCpuTimer::ScopedCpuTimer(std::uint64_t &destination) noexcept
		: destination_(destination),
		  start_(std::chrono::steady_clock::now()) {
}

ScopedCpuTimer::~ScopedCpuTimer() {
	const auto kElapsed = std::chrono::steady_clock::now() - start_;
	destination_ = static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::microseconds>(kElapsed).count());
}

} // namespace crimson
