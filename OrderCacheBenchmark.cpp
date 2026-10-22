#include "OrderCacheBenchmark.h"
#include "OrderCacheHelpers.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <string>

// ============================================================================
// Manual Harness Definitions (Matching OrderCacheBenchmark.h)
// ============================================================================

namespace Benchmark
{

using Clock = std::chrono::steady_clock;

void printBenchmarkHeader()
{
    std::cout << "========================================================================================\n"
              << "                       ORDER CACHE CORE ENGINE BENCHMARK                                \n"
              << "========================================================================================\n";
}

void printBenchmarkFooter()
{
    std::cout << "========================================================================================\n";
}

void printBenchmarkResult(const BenchmarkResult& res)
{
    std::cout << std::left << std::setw(32) << res.name
              << " | Ops: " << std::setw(10) << res.operations
              << " | Time: " << std::setw(8) << std::fixed << std::setprecision(2) << res.durationMs << " ms"
              << " | Throughput: " << std::setw(12) << std::fixed << std::setprecision(0) << res.opsPerSec << " ops/s"
              << " | Avg Latency: " << std::setw(8) << std::fixed << std::setprecision(1) << res.avgLatencyNs << " ns\n";
}

BenchmarkResult benchmarkIngestion(const std::size_t orderCount)
{
    OrderCache cache;
    std::vector<Order> orders;
    orders.reserve(orderCount);

    for (std::size_t i = 0; i < orderCount; ++i)
    {
        orders.emplace_back(
            "Ord_" + std::to_string(i),
            "Sec_" + std::to_string(i % 10),
            (i % 2 == 0) ? "Buy" : "Sell",
            100,
            "User_" + std::to_string(i % 50),
            "Comp_" + std::to_string(i % 5)
        );
    }

    const auto start = Clock::now();
    for (auto& order : orders)
    {
        cache.addOrder(std::move(order));
    }
    const auto end = Clock::now();

    const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
    const double elapsedNs = std::chrono::duration<double, std::nano>(end - start).count();

    return {
        "Ingestion (" + std::to_string(orderCount) + " orders)",
        orderCount,
        elapsedMs,
        (orderCount / (elapsedMs / 1000.0)),
        (elapsedNs / orderCount)
    };
}

BenchmarkResult benchmarkQueryLatency(const std::size_t bookSize, const std::size_t iterations)
{
    OrderCache cache;
    for (std::size_t i = 0; i < bookSize; ++i)
    {
        cache.addOrder(Order{
            "Seed_" + std::to_string(i),
            "SecTarget",
            (i % 2 == 0) ? "Buy" : "Sell",
            10,
            "User_" + std::to_string(i % 10),
            "Comp_" + std::to_string(i % 3)
        });
    }

    volatile unsigned int dummy = cache.getMatchingSizeForSecurity("SecTarget");
    (void)dummy;

    const auto start = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i)
    {
        dummy = cache.getMatchingSizeForSecurity("SecTarget");
    }
    const auto end = Clock::now();

    const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
    const double elapsedNs = std::chrono::duration<double, std::nano>(end - start).count();

    return {
        "Query O(K) (" + std::to_string(bookSize) + " book depth)",
        iterations,
        elapsedMs,
        (iterations / (elapsedMs / 1000.0)),
        (elapsedNs / iterations)
    };
}

} // namespace Benchmark

// ============================================================================
// Google Benchmark Suite
// ============================================================================

// 1. In-Memory Baseline: Order Object Instantiation Overhead
static void BM_OrderCreation(benchmark::State& state)
{
    for (auto _ : state)
    {
        Order order("Ord_1", "SecA", "Buy", 100, "User1", "CompanyA");
        benchmark::DoNotOptimize(order);
    }
}
BENCHMARK(BM_OrderCreation);

// 2. Cache Ingestion Throughput (Parameterized from 1k to 100k)
static void BM_OrderCacheIngestion(benchmark::State& state)
{
    const std::size_t orderCount = state.range(0);

    for (auto _ : state)
    {
        state.PauseTiming();
        std::vector<Order> batch;
        batch.reserve(orderCount);
        for (std::size_t i = 0; i < orderCount; ++i)
        {
            batch.emplace_back(
                "Ord_" + std::to_string(i),
                "Sec_" + std::to_string(i % 10),
                (i % 2 == 0) ? "Buy" : "Sell",
                100,
                "User_" + std::to_string(i % 50),
                "Comp_" + std::to_string(i % 5)
            );
        }
        OrderCache cache;
        state.ResumeTiming();

        for (auto& ord : batch)
        {
            cache.addOrder(std::move(ord));
        }
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * orderCount);
}
BENCHMARK(BM_OrderCacheIngestion)->RangeMultiplier(10)->Range(1'000, 100'000);

// 3. getMatchingSizeForSecurity Latency across large order depths
static void BM_QueryMatchingSize(benchmark::State& state)
{
    const std::size_t bookSize = state.range(0);

    OrderCache cache;
    for (std::size_t i = 0; i < bookSize; ++i)
    {
        cache.addOrder(Order{
            "Seed_" + std::to_string(i),
            "SecBench",
            (i % 2 == 0) ? "Buy" : "Sell",
            10,
            "User_" + std::to_string(i % 10),
            "Comp_" + std::to_string(i % 3)
        });
    }

    for (auto _ : state)
    {
        unsigned int match = cache.getMatchingSizeForSecurity("SecBench");
        benchmark::DoNotOptimize(match);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_QueryMatchingSize)->RangeMultiplier(10)->Range(1'000, 100'000);

// 4. Order Cancellation Latency (Single Order Removal from Populated Cache)
static void BM_CancelSingleOrder(benchmark::State& state)
{
    const std::size_t bookSize = state.range(0);

    for (auto _ : state)
    {
        state.PauseTiming();
        OrderCache cache;
        for (std::size_t i = 0; i < bookSize; ++i)
        {
            cache.addOrder(Order{
                "Ord_" + std::to_string(i),
                "SecA",
                (i % 2 == 0) ? "Buy" : "Sell",
                100,
                "User_" + std::to_string(i % 10),
                "Comp_" + std::to_string(i % 3)
            });
        }
        state.ResumeTiming();

        cache.cancelOrder("Ord_500");
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CancelSingleOrder)->RangeMultiplier(10)->Range(1'000, 10'000);

// 5. Bulk User Cancellation (Iterative Pruning Performance)
static void BM_CancelOrdersForUser(benchmark::State& state)
{
    const std::size_t bookSize = state.range(0);

    for (auto _ : state)
    {
        state.PauseTiming();
        OrderCache cache;
        for (std::size_t i = 0; i < bookSize; ++i)
        {
            cache.addOrder(Order{
                "Ord_" + std::to_string(i),
                "SecA",
                (i % 2 == 0) ? "Buy" : "Sell",
                100,
                "TargetUser",
                "CompA"
            });
        }
        state.ResumeTiming();

        cache.cancelOrdersForUser("TargetUser");
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * bookSize);
}
BENCHMARK(BM_CancelOrdersForUser)->RangeMultiplier(10)->Range(1'000, 10'000);

// ============================================================================
// Serialization Shootout: Protobuf vs. FlatBuffers
// ============================================================================

// --- PROTOBUF BENCHMARKS ---

static void BM_Protobuf_Serialize(benchmark::State& state)
{
    const Order order("Ord_1001", "Sec_US_10Y", "Buy", 50000, "Trader_Alpha", "GoldmanSachs");
    std::size_t wireBytes = 0;

    for (auto _ : state)
    {
        std::string binary = Protobuff::serializeOrder(order);
        wireBytes = binary.size();
        benchmark::DoNotOptimize(binary);
    }

    state.counters["BytesPerOrder"] = static_cast<double>(wireBytes);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Protobuf_Serialize);

static void BM_Protobuf_Deserialize(benchmark::State& state)
{
    const Order order("Ord_1001", "Sec_US_10Y", "Buy", 50000, "Trader_Alpha", "GoldmanSachs");
    const std::string binary = Protobuff::serializeOrder(order);

    for (auto _ : state)
    {
        Order deserialized = Protobuff::deserializeOrder(binary);
        benchmark::DoNotOptimize(deserialized);
    }

    state.counters["BytesPerOrder"] = static_cast<double>(binary.size());
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Protobuf_Deserialize);

// --- FLATBUFFERS BENCHMARKS ---

static void BM_FlatBuffers_Serialize(benchmark::State& state)
{
    const Order order("Ord_1001", "Sec_US_10Y", "Buy", 50000, "Trader_Alpha", "GoldmanSachs");
    std::size_t wireBytes = 0;

    for (auto _ : state)
    {
        std::vector<uint8_t> buffer = FlatBuff::OrderFlatBufferAdapter::serialize(order);
        wireBytes = buffer.size();
        benchmark::DoNotOptimize(buffer);
    }

    state.counters["BytesPerOrder"] = static_cast<double>(wireBytes);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FlatBuffers_Serialize);

// Direct pointer inspection from byte buffer without memory allocation
static void BM_FlatBuffers_ZeroCopyAccess(benchmark::State& state)
{
    const Order order("Ord_1001", "Sec_US_10Y", "Buy", 50000, "Trader_Alpha", "GoldmanSachs");
    const std::vector<uint8_t> buffer = FlatBuff::OrderFlatBufferAdapter::serialize(order);

    for (auto _ : state)
    {
        const auto* flatOrder = FlatBuff::OrderFlatBufferAdapter::verifyAndAccess(buffer.data(), buffer.size());
        
        // Zero-copy direct read
        uint32_t qty = flatOrder->qty();
        benchmark::DoNotOptimize(qty);
    }

    state.counters["BytesPerOrder"] = static_cast<double>(buffer.size());
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FlatBuffers_ZeroCopyAccess);

// Full domain object reconstruction from FlatBuffers buffer
static void BM_FlatBuffers_Deserialize(benchmark::State& state)
{
    const Order order("Ord_1001", "Sec_US_10Y", "Buy", 50000, "Trader_Alpha", "GoldmanSachs");
    const std::vector<uint8_t> buffer = FlatBuff::OrderFlatBufferAdapter::serialize(order);

    for (auto _ : state)
    {
        Order deserialized = FlatBuff::OrderFlatBufferAdapter::deserialize(buffer.data(), buffer.size());
        benchmark::DoNotOptimize(deserialized);
    }

    state.counters["BytesPerOrder"] = static_cast<double>(buffer.size());
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FlatBuffers_Deserialize);

#include "RedisClient.h"

// ============================================================================
// Redis Distributed Storage Latency & Throughput
// ============================================================================

static void BM_Redis_Ping(benchmark::State& state)
{
    Redis::RedisClient client;
    if (!client.ping())
    {
        state.SkipWithError("Redis server not reachable at 127.0.0.1:6379");
        return;
    }

    for (auto _ : state)
    {
        bool ok = client.ping();
        benchmark::DoNotOptimize(ok);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Redis_Ping)->Unit(benchmark::kMicrosecond);

static void BM_Redis_SaveOrder(benchmark::State& state)
{
    Redis::RedisClient client;
    if (!client.ping())
    {
        state.SkipWithError("Redis server not reachable at 127.0.0.1:6379");
        return;
    }

    const Order order("Ord_Bench_Save", "Sec_US_10Y", "Buy", 50000, "Trader_Alpha", "GoldmanSachs");

    for (auto _ : state)
    {
        bool ok = client.saveOrder(order);
        benchmark::DoNotOptimize(ok);
    }

    // Cleanup benchmark key after execution
    client.removeOrder("Ord_Bench_Save");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Redis_SaveOrder)->Unit(benchmark::kMicrosecond);

static void BM_Redis_GetOrder(benchmark::State& state)
{
    Redis::RedisClient client;
    if (!client.ping())
    {
        state.SkipWithError("Redis server not reachable at 127.0.0.1:6379");
        return;
    }

    const Order order("Ord_Bench_Get", "Sec_US_10Y", "Buy", 50000, "Trader_Alpha", "GoldmanSachs");
    client.saveOrder(order);

    for (auto _ : state)
    {
        auto retrieved = client.getOrder("Ord_Bench_Get");
        benchmark::DoNotOptimize(retrieved);
    }

    client.removeOrder("Ord_Bench_Get");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Redis_GetOrder)->Unit(benchmark::kMicrosecond);

static void BM_Redis_ColdBootRecovery(benchmark::State& state)
{
    Redis::RedisClient client;
    if (!client.ping())
    {
        state.SkipWithError("Redis server not reachable at 127.0.0.1:6379");
        return;
    }

    const std::size_t orderCount = state.range(0);

    // Seed Redis store prior to measurement
    for (std::size_t i = 0; i < orderCount; ++i)
    {
        client.saveOrder(Order{
            "Recover_" + std::to_string(i),
            "SecA",
            (i % 2 == 0) ? "Buy" : "Sell",
            100,
            "User1",
            "CompA"
        });
    }

    for (auto _ : state)
    {
        OrderCache cache;
        const auto orderIds = client.getAllOrderIds();
        
        for (const auto& id : orderIds)
        {
            auto ord = client.getOrder(id);
            if (ord.has_value())
            {
                cache.addOrder(std::move(*ord));
            }
        }
        benchmark::ClobberMemory();
    }

    // Cleanup keys
    for (std::size_t i = 0; i < orderCount; ++i)
    {
        client.removeOrder("Recover_" + std::to_string(i));
    }

    state.SetItemsProcessed(state.iterations() * orderCount);
}
BENCHMARK(BM_Redis_ColdBootRecovery)->Range(100, 1'000)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();