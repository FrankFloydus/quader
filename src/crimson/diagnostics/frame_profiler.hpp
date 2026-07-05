#pragma once

#include "crimson/frame/frame_stats.hpp"

#include <chrono>
#include <cstdint>

namespace crimson {

class ScopedCpuTimer final {
public:
    explicit ScopedCpuTimer(std::uint64_t& destination) noexcept;
    ~ScopedCpuTimer();

    ScopedCpuTimer(const ScopedCpuTimer&) = delete;
    ScopedCpuTimer& operator=(const ScopedCpuTimer&) = delete;

private:
    std::uint64_t& destination_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace crimson
