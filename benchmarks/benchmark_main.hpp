#pragma once

#include "crimson/diagnostics/renderer_diagnostics_snapshot.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace quader::benchmarks {

struct BenchmarkRunConfig {
	std::uint32_t warmup_iterations = 3;
	std::uint32_t measured_iterations = 30;
	std::uint32_t fixture_size = 10000;
};

struct BenchmarkResult {
	std::string name;
	std::uint32_t iterations = 0;
	std::uint64_t median_us = 0;
	std::uint64_t min_us = 0;
	std::uint64_t max_us = 0;
	std::vector<crimson::RendererCounter> counters;
};

using BenchmarkBody = std::function<std::vector<crimson::RendererCounter>()>;

[[nodiscard]] BenchmarkRunConfig parse_benchmark_config(int argc, char **argv);
[[nodiscard]] BenchmarkResult run_benchmark(
		std::string name,
		const BenchmarkRunConfig &config,
		const BenchmarkBody &body);
void print_jsonl(const BenchmarkResult &result);

[[nodiscard]] BenchmarkResult run_crimson_packet_pipeline_benchmark(const BenchmarkRunConfig &config);
[[nodiscard]] BenchmarkResult run_crimson_upload_benchmark(const BenchmarkRunConfig &config);

} // namespace quader::benchmarks
