#pragma once

#include <benchmark/benchmark.h>
#include "OrderCache.h"
#include "OrderCacheHelpers.h"
#include "RedisClient.h"



namespace Benchmark
{


struct BenchmarkResult
{
    std::string name;
    std::size_t operations{0};
    double durationMs{0.0};
    double opsPerSec{0.0};
    double avgLatencyNs{0.0};
};


BenchmarkResult benchmarkIngestion(const std::size_t orderCount);
BenchmarkResult benchmarkQueryLatency(const std::size_t bookSize, const std::size_t iterations);

static void printBenchmarkResult(const BenchmarkResult& res);
static void printBenchmarkHeader();
static void printBenchmarkFooter();










} // Benchmark




