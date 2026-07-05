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

#include "crimson/diagnostics/renderer_diagnostics_snapshot.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace quader::benchmarks {

/// Runtime configuration shared by benchmark executables.
struct BenchmarkRunConfig {
	/// Number of untimed warmup iterations.
	std::uint32_t warmup_iterations = 3;
	/// Number of measured iterations.
	std::uint32_t measured_iterations = 30;
	/// Size of the synthetic benchmark fixture.
	std::uint32_t fixture_size = 10000;
};

/// Result row emitted by benchmark helpers.
struct BenchmarkResult {
	/// Benchmark name.
	std::string name;
	/// Number of measured iterations.
	std::uint32_t iterations = 0;
	/// Median elapsed time in microseconds.
	std::uint64_t median_us = 0;
	/// Minimum elapsed time in microseconds.
	std::uint64_t min_us = 0;
	/// Maximum elapsed time in microseconds.
	std::uint64_t max_us = 0;
	/// Renderer counters captured from the final measured iteration.
	std::vector<crimson::RendererCounter> counters;
};

/// Callable benchmark body that returns renderer counters for one iteration.
using BenchmarkBody = std::function<std::vector<crimson::RendererCounter>()>;

/**
 * Parse benchmark command-line options.
 *
 * @param argc Argument count from `main`.
 * @param argv Argument vector from `main`.
 * @return Benchmark configuration with invalid numeric values clamped to defaults.
 */
[[nodiscard]] BenchmarkRunConfig parse_benchmark_config(int argc, char **argv);
/**
 * Run a benchmark body with warmup and measured iterations.
 *
 * @param name Benchmark name.
 * @param config Benchmark run configuration.
 * @param body Callable measured by the benchmark harness.
 * @return Timing result and counters from the final measured iteration.
 */
[[nodiscard]] BenchmarkResult run_benchmark(
		std::string name,
		const BenchmarkRunConfig &config,
		const BenchmarkBody &body);
/**
 * Print a benchmark result as one JSONL record.
 *
 * @param result Result to print.
 */
void print_jsonl(const BenchmarkResult &result);

/**
 * Run the Crimson draw-packet pipeline benchmark.
 *
 * @param config Benchmark run configuration.
 * @return Benchmark timing result.
 */
[[nodiscard]] BenchmarkResult run_crimson_packet_pipeline_benchmark(const BenchmarkRunConfig &config);
/**
 * Run the Crimson render-mesh upload benchmark.
 *
 * @param config Benchmark run configuration.
 * @return Benchmark timing result.
 */
[[nodiscard]] BenchmarkResult run_crimson_upload_benchmark(const BenchmarkRunConfig &config);

} // namespace quader::benchmarks
