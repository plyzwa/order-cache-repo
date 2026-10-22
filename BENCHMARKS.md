# OrderCache Performance Engineering & Benchmarking Report

Comprehensive performance evaluation of the `OrderCache` trading engine across in-memory operations, binary serialization protocols (Protocol Buffers vs. FlatBuffers), and distributed persistence via Redis.

---

## 1. Test Environment & Setup

- **Hardware:** 6-Core x86_64 CPU @ 2.375 GHz
- **Memory Architecture:** L1 Data 32 KiB (x6), L1 Instruction 32 KiB (x6), L2 Unified 512 KiB (x6), L3 Unified 4096 KiB (x2)
- **Compiler:** GCC / Clang (C++17) with `-O3 -DNDEBUG`
- **Benchmarking Framework:** Google Benchmark v1.8.3
- **Network / Transport:** TCP Loopback (`127.0.0.1:6379`) for Redis socket tests

---

## 2. In-Memory Engine Performance

The core order book utilizes an incremental aggregate tracking structure maintaining security and company volumes in real time.

| Benchmark Target | Order Depth | Operation Time (ns) | Throughput (ops/sec) |
|---|---|---|---|
| `BM_OrderCreation` (Object Overhead) | 1 | 24.6 ns | ~40.6M |
| `BM_OrderCacheIngestion` | 1,000 | 238 ns / order | 4.20M |
| `BM_OrderCacheIngestion` | 10,000 | 266 ns / order | 3.76M |
| `BM_OrderCacheIngestion` | 100,000 | 526 ns / order | 1.90M |
| `BM_QueryMatchingSize` | 1,000 | **14.2 ns** | **70.6M** |
| `BM_QueryMatchingSize` | 10,000 | **14.2 ns** | **70.6M** |
| `BM_QueryMatchingSize` | 100,000 | **14.2 ns** | **70.6M** |
| `BM_CancelSingleOrder` | 1,000 | 23.7 µs | 42.2K |
| `BM_CancelSingleOrder` | 10,000 | 298.3 µs | 3.35K |
| `BM_CancelOrdersForUser` | 1,000 | 75.9 ns / order | 13.18M |
| `BM_CancelOrdersForUser` | 10,000 | 83.0 ns / order | 12.05M |

### Key In-Memory Observations
1. **Strict $O(K)$ Query Scaling:** `BM_QueryMatchingSize` executes in a deterministic **14.2 ns** regardless of whether the order book depth is 1,000 or 100,000 orders. Tracking aggregates incrementally shifts match evaluation from an $O(N)$ scanning loop to a closed-form calculation bounded strictly by company count ($K$).
2. **Ingestion Bandwidth:** Add operations achieve **1.90M – 4.20M ops/sec**, comfortably sustaining institutional market-feed velocity.
3. **Cancellation Dynamics:** Single cancellation via key lookup maintains low microsecond latency, while bulk user cancellation processes orders at **>12M items/sec**.

---

## 3. Serialization Shootout: Protobuf vs. FlatBuffers

Evaluation of wire payload size, serialization latency, and object unpacking cost:

| Operation | Format | Time (ns) | Wire Size | Throughput (ops/sec) |
|---|---|---|---|---|
| Serialization | Protocol Buffers (Proto3) | 209 ns | **56 bytes** | 4.77M |
| Deserialization | Protocol Buffers (Proto3) | 267 ns | 56 bytes | 3.75M |
| Serialization | FlatBuffers | 122 ns | 116 bytes | 8.17M |
| Zero-Copy Direct Access | FlatBuffers (`verifyAndAccess`) | **25.1 ns** | 116 bytes | **39.90M** |
| Full Deserialization | FlatBuffers | 69.1 ns | 116 bytes | 14.48M |

### Serialization Trade-off Analysis
* **Wire Compactness (Protobuf Advantage):** Protobuf generates a **56-byte** payload versus FlatBuffers' **116-byte** footprint. Varint encoding and omitted internal alignment padding yield a **51.7% wire size reduction**, making Protobuf optimal for WAN distribution, external gateways, and space-constrained cold storage logs.
* **CPU Latency & Zero-Copy (FlatBuffers Advantage):** FlatBuffers serializes in **122 ns** (1.7x faster than Protobuf) and deserializes in **69.1 ns** (3.8x faster than Protobuf).
* **The In-Place Advantage:** Inspecting fields directly from a raw FlatBuffers byte stream (`BM_FlatBuffers_ZeroCopyAccess`) requires only **25.1 ns**—nearly matching native in-memory struct creation (**24.6 ns**). Eliminating memory allocations during reads makes FlatBuffers ideal for ultra-low-latency intra-host IPC, shared memory fabrics, and matching engine tick-to-trade hot paths.

---

## 4. Redis Distributed Persistence & Boundary Cost

Storage operations crossing the process boundary via loopback TCP socket (`127.0.0.1:6379`):

| Operation | Scale | Latency | Wall Clock | Throughput |
|---|---|---|---|---|
| `BM_Redis_Ping` | 1 | 15.5 µs CPU | 42.0 µs Wall | 64.3K ops/s |
| `BM_Redis_SaveOrder` | 1 | 17.5 µs CPU | 45.0 µs Wall | 57.2K ops/s |
| `BM_Redis_GetOrder` | 1 | 16.9 µs CPU | 44.7 µs Wall | 59.1K ops/s |
| `BM_Redis_ColdBootRecovery` | 100 orders | 1.78 ms CPU | 4.60 ms Wall | 56.0K items/s |
| `BM_Redis_ColdBootRecovery` | 512 orders | 9.80 ms CPU | 24.0 ms Wall | 52.3K items/s |
| `BM_Redis_ColdBootRecovery` | 1,000 orders | 17.8 ms CPU | 45.9 ms Wall | 56.3K items/s |

### Systems Architecture Insights
1. **The IPC / Network Boundary Penalty:**
   - In-memory insertion: **~0.24 µs** (238 ns)
   - Synchronous Redis write: **~45.0 µs**
   - Traversing the OS network stack and TCP loopback introduces a **~187x latency penalty** over local memory.
2. **Cold Boot Recovery Throughput:**
   - Sequential order hydration recovers at **~56K orders/sec** (loading 1,000 orders in ~45.9 ms).
   - For larger production datasets, recovery throughput can be scaled by pipeline batching (`MGET` or Redis transaction pipelines) rather than point-to-point roundtrips.
3. **Persistence Design Recommendation:**
   - In low-latency trading paths, persistence should operate asynchronously via lock-free ring buffers (disruptor pattern) or background worker threads, shielding the critical matching path from socket I/O jitter.