/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "benchmark_main.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace quader::benchmarks {
namespace {

[[nodiscard]] std::uint32_t parse_uint_arg(
		int argc,
		char **argv,
		std::string_view name,
		std::uint32_t fallback) {
	const std::string kPrefix = std::string(name) + "=";
	for (int index = 1; index < argc; ++index) {
		const std::string_view kArg(argv[index]);
		if (kArg.rfind(kPrefix, 0) != 0) {
			continue;
		}
		try {
			return static_cast<std::uint32_t>(std::stoul(std::string(kArg.substr(kPrefix.size()))));
		} catch (...) {
			return fallback;
		}
	}
	return fallback;
}

[[nodiscard]] std::string escape_json(std::string_view value) {
	std::string escaped;
	escaped.reserve(value.size());
	for (const char kC : value) {
		if (kC == '"' || kC == '\\') {
			escaped.push_back('\\');
		}
		escaped.push_back(kC);
	}
	return escaped;
}

} // namespace

BenchmarkRunConfig parse_benchmark_config(int argc, char **argv) {
	BenchmarkRunConfig config;
	config.warmup_iterations = parse_uint_arg(argc, argv, "--warmup", config.warmup_iterations);
	config.measured_iterations = parse_uint_arg(argc, argv, "--iterations", config.measured_iterations);
	config.fixture_size = parse_uint_arg(argc, argv, "--fixture-size", config.fixture_size);
	config.measured_iterations = std::max<std::uint32_t>(1, config.measured_iterations);
	config.fixture_size = std::max<std::uint32_t>(1, config.fixture_size);
	return config;
}

BenchmarkResult run_benchmark(std::string name, const BenchmarkRunConfig &config, const BenchmarkBody &body) {
	for (std::uint32_t iteration = 0; iteration < config.warmup_iterations; ++iteration) {
		(void)body();
	}

	std::vector<std::uint64_t> samples;
	samples.reserve(config.measured_iterations);
	std::vector<crimson::RendererCounter> counters;
	for (std::uint32_t iteration = 0; iteration < config.measured_iterations; ++iteration) {
		const auto kStart = std::chrono::steady_clock::now();
		counters = body();
		const auto kElapsed = std::chrono::steady_clock::now() - kStart;
		samples.push_back(static_cast<std::uint64_t>(
				std::chrono::duration_cast<std::chrono::microseconds>(kElapsed).count()));
	}

	std::sort(samples.begin(), samples.end());
	return BenchmarkResult{
		.name = std::move(name),
		.iterations = config.measured_iterations,
		.median_us = samples[samples.size() / 2],
		.min_us = samples.front(),
		.max_us = samples.back(),
		.counters = std::move(counters),
	};
}

void print_jsonl(const BenchmarkResult &result) {
	std::cout << "{\"name\":\"" << escape_json(result.name)
			  << "\",\"iterations\":" << result.iterations
			  << ",\"median_us\":" << result.median_us
			  << ",\"min_us\":" << result.min_us
			  << ",\"max_us\":" << result.max_us
			  << ",\"counters\":{";
	for (std::size_t index = 0; index < result.counters.size(); ++index) {
		const crimson::RendererCounter &counter = result.counters[index];
		if (index != 0) {
			std::cout << ',';
		}
		std::cout << "\"" << escape_json(std::string(crimson::renderer_counter_domain_name(counter.domain)))
				  << "." << escape_json(counter.name) << "\":" << counter.value;
	}
	std::cout << "}}\n";
}

} // namespace quader::benchmarks

int main(int argc, char **argv) {
	const quader::benchmarks::BenchmarkRunConfig kConfig =
			quader::benchmarks::parse_benchmark_config(argc, argv);
	quader::benchmarks::print_jsonl(quader::benchmarks::run_crimson_packet_pipeline_benchmark(kConfig));
	quader::benchmarks::print_jsonl(quader::benchmarks::run_crimson_upload_benchmark(kConfig));
	return 0;
}
