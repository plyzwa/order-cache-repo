// Use the Google Test framework.

#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <iostream>

#include "OrderCache.h"
#include "gtest/gtest.h"

class OrderCacheTest : public ::testing::Test
{
protected:
    OrderCache cache;

    void SetUp() override
    {
        // Initialize test fixtures here if needed
    }
};

// ====================================== PHASE 1 STEP0 ======================================
TEST_F(OrderCacheTest, ExampleAddOrder)
{
    cache.addOrder(Order{"OrdId1", "SecId1", "Buy", 1000, "User1", "CompanyA"});
    ASSERT_EQ(cache.getAllOrders().size(), 1);
}

// ====================================== PHASE 1 STEP1 ======================================
TEST_F(OrderCacheTest, AddAndRetrieveSingleOrder)
{
    cache.addOrder(Order{"OrdId1", "SecId1", "Buy", 1000, "User1", "CompanyA"});
    cache.addOrder(Order{"OrdId1", "SecId2", "Sell", 200, "User2", "CompanyC"});

    ASSERT_EQ(cache.getAllOrders().size(), 1);
}

TEST_F(OrderCacheTest, RejectInvalidOrder)
{
    cache.addOrder(Order{"OrdId1", "SecId1", "Buy", 500, "User1", "CompanyA"});
    cache.addOrder(Order{"OrdId1", "SecId2", "Sell", 0, "User2", "CompanyA"});
    cache.addOrder(Order{"OrdId3", "SecId3", "ORD", 0, "User3", "CompanyA"});

    ASSERT_EQ(cache.getAllOrders().size(), 1);
}

TEST_F(OrderCacheTest, CancelExistingOrder)
{
    cache.addOrder(Order{"OrdId1", "SecId1", "Buy", 500, "User1", "CompanyA"});
    ASSERT_EQ(cache.getAllOrders().size(), 1);
    
    cache.cancelOrder("OrdId1");
    ASSERT_EQ(cache.getAllOrders().size(), 0);
}

TEST_F(OrderCacheTest, CancelNonExistingOrder)
{
    cache.addOrder(Order{"OrdId1", "SecId1", "Buy", 500, "User1", "CompanyA"});
    cache.addOrder(Order{"OrdId2", "SecId2", "Sell", 200, "User2", "CompanyA"});
    cache.addOrder(Order{"OrdId3", "SecId3", "Buy", 300, "User3", "CompanyA"});
    cache.cancelOrder("OrdId4");

    ASSERT_EQ(cache.getAllOrders().size(), 3);
}

// ====================================== PHASE 1 STEP2 ======================================

TEST_F(OrderCacheTest, CancelOrdersForUser_RemovesOnlyTargetUser)
{
    cache.addOrder(Order{"Ord1", "SecA", "Buy", 100, "User1", "Comp1"});
    cache.addOrder(Order{"Ord2", "SecB", "Sell", 200, "User2", "Comp2"});
    cache.addOrder(Order{"Ord3", "SecA", "Sell", 300, "User1", "Comp1"});

    cache.cancelOrdersForUser("User1");

    const auto remaining = cache.getAllOrders();
    ASSERT_EQ(remaining.size(), 1);
    EXPECT_EQ(remaining[0].orderId(), "Ord2");
    EXPECT_EQ(remaining[0].user(), "User2");
}

TEST_F(OrderCacheTest, CancelOrdersForUser_NonExistentUserIsNoOp)
{
    cache.addOrder(Order{"Ord1", "SecA", "Buy", 100, "User1", "Comp1"});
    
    EXPECT_NO_THROW(cache.cancelOrdersForUser("UnknownUser"));
    EXPECT_EQ(cache.getAllOrders().size(), 1);
}

TEST_F(OrderCacheTest, CancelOrdersForSecIdWithMinimumQty_FiltersCorrectly)
{
    // Target security SecA
    cache.addOrder(Order{"Ord1", "SecA", "Buy", 100, "User1", "Comp1"});  // exactly 100 -> removed
    cache.addOrder(Order{"Ord2", "SecA", "Sell", 250, "User2", "Comp2"}); // > 100 -> removed
    cache.addOrder(Order{"Ord3", "SecA", "Buy", 50, "User3", "Comp1"});   // < 100 -> preserved
    // Other security SecB
    cache.addOrder(Order{"Ord4", "SecB", "Sell", 500, "User4", "Comp2"}); // different SecId -> preserved

    cache.cancelOrdersForSecIdWithMinimumQty("SecA", 100);

    const auto remaining = cache.getAllOrders();
    ASSERT_EQ(remaining.size(), 2);

    // Verify surviving orders are Ord3 and Ord4
    bool foundOrd3 = false;
    bool foundOrd4 = false;
    for (const auto& order : remaining)
    {
        if (order.orderId() == "Ord3") foundOrd3 = true;
        if (order.orderId() == "Ord4") foundOrd4 = true;
    }
    EXPECT_TRUE(foundOrd3);
    EXPECT_TRUE(foundOrd4);
}

TEST_F(OrderCacheTest, CancelOrdersForSecIdWithMinimumQty_NonExistentSecurityIsNoOp)
{
    cache.addOrder(Order{"Ord1", "SecA", "Buy", 100, "User1", "Comp1"});
    
    EXPECT_NO_THROW(cache.cancelOrdersForSecIdWithMinimumQty("SecUnknown", 50));
    EXPECT_EQ(cache.getAllOrders().size(), 1);
}

// ====================================== PHASE 2 STEP3 ======================================

TEST_F(OrderCacheTest, Matching_ReadmeExample1)
{
    cache.addOrder(Order{"OrdId1", "SecId1", "Buy",  1000, "User1", "CompanyA"});
    cache.addOrder(Order{"OrdId2", "SecId2", "Sell", 3000, "User2", "CompanyB"});
    cache.addOrder(Order{"OrdId3", "SecId1", "Sell",  500, "User3", "CompanyA"});
    cache.addOrder(Order{"OrdId4", "SecId2", "Buy",   600, "User4", "CompanyC"});
    cache.addOrder(Order{"OrdId5", "SecId2", "Buy",   100, "User5", "CompanyB"});
    cache.addOrder(Order{"OrdId6", "SecId3", "Buy",  1000, "User6", "CompanyD"});
    cache.addOrder(Order{"OrdId7", "SecId2", "Buy",  2000, "User7", "CompanyE"});
    cache.addOrder(Order{"OrdId8", "SecId2", "Sell", 5000, "User8", "CompanyE"});

    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecId1"), 0);
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecId2"), 2700);
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecId3"), 0);

    EXPECT_EQ(cache.getAllOrders().size(), 8);
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecId2"), 2700);
}

TEST_F(OrderCacheTest, Matching_ReadmeExample2)
{
    cache.addOrder(Order{"OrdId1",  "SecId1", "Sell",  100, "User10", "Company2"});
    cache.addOrder(Order{"OrdId2",  "SecId3", "Sell",  200, "User8",  "Company2"});
    cache.addOrder(Order{"OrdId3",  "SecId1", "Buy",   300, "User13", "Company2"});
    cache.addOrder(Order{"OrdId4",  "SecId2", "Sell",  400, "User12", "Company2"});
    cache.addOrder(Order{"OrdId5",  "SecId3", "Sell",  500, "User7",  "Company2"});
    cache.addOrder(Order{"OrdId6",  "SecId3", "Buy",   600, "User3",  "Company1"});
    cache.addOrder(Order{"OrdId7",  "SecId1", "Sell",  700, "User10", "Company2"});
    cache.addOrder(Order{"OrdId8",  "SecId1", "Sell",  800, "User2",  "Company1"});
    cache.addOrder(Order{"OrdId9",  "SecId2", "Buy",   900, "User6",  "Company2"});
    cache.addOrder(Order{"OrdId10", "SecId2", "Sell", 1000, "User5",  "Company1"});
    cache.addOrder(Order{"OrdId11", "SecId1", "Sell", 1100, "User13", "Company2"});
    cache.addOrder(Order{"OrdId12", "SecId2", "Buy",  1200, "User9",  "Company2"});
    cache.addOrder(Order{"OrdId13", "SecId1", "Sell", 1300, "User1",  "Company1"});

    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecId1"), 300);
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecId2"), 1000);
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecId3"), 600);
}

TEST_F(OrderCacheTest, Matching_ReadmeExample3)
{
    cache.addOrder(Order{"OrdId1",  "SecId3", "Sell",  100, "User1", "Company1"});
    cache.addOrder(Order{"OrdId2",  "SecId3", "Sell",  200, "User3", "Company2"});
    cache.addOrder(Order{"OrdId3",  "SecId1", "Buy",   300, "User2", "Company1"});
    cache.addOrder(Order{"OrdId4",  "SecId3", "Sell",  400, "User5", "Company2"});
    cache.addOrder(Order{"OrdId5",  "SecId2", "Sell",  500, "User2", "Company1"});
    cache.addOrder(Order{"OrdId6",  "SecId2", "Buy",   600, "User3", "Company2"});
    cache.addOrder(Order{"OrdId7",  "SecId2", "Sell",  700, "User1", "Company1"});
    cache.addOrder(Order{"OrdId8",  "SecId1", "Sell",  800, "User2", "Company1"});
    cache.addOrder(Order{"OrdId9",  "SecId1", "Buy",   900, "User5", "Company2"});
    cache.addOrder(Order{"OrdId10", "SecId1", "Sell", 1000, "User1", "Company1"});
    cache.addOrder(Order{"OrdId11", "SecId2", "Sell", 1100, "User6", "Company2"});

    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecId1"), 900);
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecId2"), 600);
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecId3"), 0);
}

// ====================================== PHASE 2 STEP4 ======================================

TEST_F(OrderCacheTest, Matching_EmptyAndNonExistentSecurity)
{
    EXPECT_EQ(cache.getMatchingSizeForSecurity(""), 0);
    EXPECT_EQ(cache.getMatchingSizeForSecurity("NonExistentSec"), 0);

    cache.addOrder(Order{"Ord1", "Sec1", "Buy", 100, "User1", "CompA"});
    EXPECT_EQ(cache.getMatchingSizeForSecurity(""), 0);
    EXPECT_EQ(cache.getMatchingSizeForSecurity("Sec2"), 0);
}

TEST_F(OrderCacheTest, Matching_SameSideOnlyReturnsZero)
{
    cache.addOrder(Order{"Ord1", "Sec1", "Buy", 100, "User1", "CompA"});
    cache.addOrder(Order{"Ord2", "Sec1", "Buy", 200, "User2", "CompB"});
    cache.addOrder(Order{"Ord3", "Sec1", "Buy", 300, "User3", "CompC"});

    EXPECT_EQ(cache.getMatchingSizeForSecurity("Sec1"), 0);
}

TEST_F(OrderCacheTest, Matching_OppositeSidesSameCompanyReturnsZero)
{
    cache.addOrder(Order{"Ord1", "Sec1", "Buy", 500, "User1", "CompanySame"});
    cache.addOrder(Order{"Ord2", "Sec1", "Sell", 500, "User2", "CompanySame"});
    cache.addOrder(Order{"Ord3", "Sec1", "Sell", 200, "User3", "CompanySame"});

    EXPECT_EQ(cache.getMatchingSizeForSecurity("Sec1"), 0);
}

TEST_F(OrderCacheTest, Matching_OneToManyPartialFillsWithCompanyFilter)
{
    // 1 large Buy for 1000 from CompA
    cache.addOrder(Order{"Buy1", "SecX", "Buy", 1000, "UserA", "CompA"});

    // 4 smaller Sells
    cache.addOrder(Order{"Sell1", "SecX", "Sell", 200, "UserB", "CompB"}); // matches 200
    cache.addOrder(Order{"Sell2", "SecX", "Sell", 300, "UserC", "CompC"}); // matches 300
    cache.addOrder(Order{"Sell3", "SecX", "Sell", 400, "UserA2", "CompA"}); // SKIPPED (same company)
    cache.addOrder(Order{"Sell4", "SecX", "Sell", 150, "UserD", "CompD"}); // matches 150

    // Total matched: 200 + 300 + 150 = 650
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecX"), 650);
}

TEST_F(OrderCacheTest, Matching_DynamicallyUpdatesAfterCancellations)
{
    cache.addOrder(Order{"Ord1", "SecY", "Buy", 500, "User1", "CompA"});
    cache.addOrder(Order{"Ord2", "SecY", "Sell", 500, "User2", "CompB"});
    cache.addOrder(Order{"Ord3", "SecY", "Sell", 300, "User3", "CompC"});

    // Initial match: 500 (Ord1 matched with Ord2)
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecY"), 500);

    // Cancel Ord2: Ord1 can now match against Ord3 for 300
    cache.cancelOrder("Ord2");
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecY"), 300);

    // Bulk cancel User1 (the buyer): match drops to 0
    cache.cancelOrdersForUser("User1");
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecY"), 0);
}

TEST_F(OrderCacheTest, AddOrder_ReAddAfterCancelSucceeds)
{
    cache.addOrder(Order{"OrdReuse", "SecZ", "Buy", 100, "User1", "CompA"});
    ASSERT_EQ(cache.getAllOrders().size(), 1);

    cache.cancelOrder("OrdReuse");
    ASSERT_EQ(cache.getAllOrders().size(), 0);

    // Re-add with same ID but different quantity
    cache.addOrder(Order{"OrdReuse", "SecZ", "Buy", 999, "User1", "CompA"});
    const auto orders = cache.getAllOrders();
    ASSERT_EQ(orders.size(), 1);
    EXPECT_EQ(orders[0].qty(), 999);
}

// ====================================== PHASE 3 STEP5 ======================================

TEST_F(OrderCacheTest, Matching_ScalesEfficientlyWithLargeOrderVolume)
{
    // Inject 100,000 orders across 3 companies for a single security
    constexpr unsigned int kOrderCount = 100000;
    for (unsigned int i = 0; i < kOrderCount; ++i)
    {
        const std::string id = "BulkOrd_" + std::to_string(i);
        if (i % 2 == 0)
        {
            cache.addOrder(Order{id, "SecScale", "Buy", 10, "User" + std::to_string(i), "CompA"});
        }
        else if (i % 4 == 1)
        {
            cache.addOrder(Order{id, "SecScale", "Sell", 10, "User" + std::to_string(i), "CompB"});
        }
        else
        {
            cache.addOrder(Order{id, "SecScale", "Sell", 10, "User" + std::to_string(i), "CompC"});
        }
    }

    // 50,000 Buys of 10 = 500,000 Buy qty from CompA
    // 25,000 Sells of 10 = 250,000 Sell qty from CompB
    // 25,000 Sells of 10 = 250,000 Sell qty from CompC
    // Total Sell qty = 500,000 (all outside CompA). Expected match = 500,000.
    EXPECT_EQ(cache.getMatchingSizeForSecurity("SecScale"), 500000);
}

TEST_F(OrderCacheTest, Benchmark_MatchingEngineLatency)
{
    constexpr unsigned int kOrders = 4000;
    for (unsigned int i = 0; i < kOrders; ++i)
    {
        const std::string id = "Bench_" + std::to_string(i);
        if (i % 2 == 0)
        {
            cache.addOrder(Order{id, "SecBench", "Buy", 10, "User" + std::to_string(i), "CompA"});
        }
        else if (i % 4 == 1)
        {
            cache.addOrder(Order{id, "SecBench", "Sell", 10, "User" + std::to_string(i), "CompB"});
        }
        else
        {
            cache.addOrder(Order{id, "SecBench", "Sell", 10, "User" + std::to_string(i), "CompC"});
        }
    }

    const auto start = std::chrono::high_resolution_clock::now();
    const unsigned int matched = cache.getMatchingSizeForSecurity("SecBench");
    const auto end = std::chrono::high_resolution_clock::now();

    const auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    EXPECT_EQ(matched, 20000);
    std::cout << "\n>>> [BENCHMARK RESULTS] <<<\n"
              << "Orders in Book: " << kOrders << "\n"
              << "Query Execution Time: " << durationUs << " us (" << durationUs / 1000.0 << " ms)\n"
              << "---------------------------\n";
}

TEST_F(OrderCacheTest, Benchmark_QueryLatencyUnderHeavyLoad)
{
    constexpr unsigned int kOrderCount = 100000;
    for (unsigned int i = 0; i < kOrderCount; ++i)
    {
        const std::string id = "BenchOrd_" + std::to_string(i);
        if (i % 2 == 0)
        {
            cache.addOrder(Order{id, "SecBench", "Buy", 10, "User" + std::to_string(i), "CompA"});
        }
        else if (i % 4 == 1)
        {
            cache.addOrder(Order{id, "SecBench", "Sell", 10, "User" + std::to_string(i), "CompB"});
        }
        else
        {
            cache.addOrder(Order{id, "SecBench", "Sell", 10, "User" + std::to_string(i), "CompC"});
        }
    }

    // Benchmark ONLY the query method execution time
    const auto start = std::chrono::high_resolution_clock::now();
    const unsigned int matched = cache.getMatchingSizeForSecurity("SecBench");
    const auto end = std::chrono::high_resolution_clock::now();

    const auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    EXPECT_EQ(matched, 500000);
    std::cout << "[ BENCHMARK ] getMatchingSizeForSecurity latency over 100,000 orders: "
              << durationNs << " ns (" << durationNs / 1'000.0 << " us)\n";

    // O(K) query over 3 companies should complete in well under 100 microseconds
    EXPECT_LT(durationNs, 100'000); 
}

// ====================================== PHASE 4 STEP6 ======================================

TEST_F(OrderCacheTest, Concurrency_ConcurrentWritersDisjointKeys)
{
    constexpr unsigned int kNumThreads = 8;
    constexpr unsigned int kOrdersPerThread = 1000;
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (unsigned int t = 0; t < kNumThreads; ++t)
    {
        threads.emplace_back([this, t]() {
            for (unsigned int i = 0; i < kOrdersPerThread; ++i)
            {
                const std::string ordId = "T" + std::to_string(t) + "_Ord_" + std::to_string(i);
                cache.addOrder(Order{ordId, "SecConcurrent", "Buy", 10, "User" + std::to_string(t), "Comp" + std::to_string(t)});
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(cache.getAllOrders().size(), kNumThreads * kOrdersPerThread);
}

TEST_F(OrderCacheTest, Concurrency_SimultaneousReadersAndWriters)
{
    constexpr unsigned int kWriters = 4;
    constexpr unsigned int kReaders = 4;
    constexpr unsigned int kOpsPerWriter = 500;
    std::atomic<bool> startFlag{false};
    std::atomic<bool> stopReaders{false};

    std::vector<std::thread> writers;
    std::vector<std::thread> readers;

    // Launch writer threads
    for (unsigned int w = 0; w < kWriters; ++w)
    {
        writers.emplace_back([this, &startFlag, w]() {
            while (!startFlag.load()) { std::this_thread::yield(); }

            for (unsigned int i = 0; i < kOpsPerWriter; ++i)
            {
                const std::string id = "RW_" + std::to_string(w) + "_" + std::to_string(i);
                const std::string side = (i % 2 == 0) ? "Buy" : "Sell";
                cache.addOrder(Order{id, "SecShared", side, 100, "User" + std::to_string(w), "Comp" + std::to_string(w)});
                
                if (i % 10 == 0)
                {
                    cache.cancelOrder(id);
                }
            }
        });
    }

    // Launch reader threads
    for (unsigned int r = 0; r < kReaders; ++r)
    {
        readers.emplace_back([this, &startFlag, &stopReaders]() {
            while (!startFlag.load()) { std::this_thread::yield(); }

            while (!stopReaders.load())
            {
                // Concurrently query matching size and snapshots
                cache.getMatchingSizeForSecurity("SecShared");
                cache.getAllOrders();
            }
        });
    }

    // Release all threads simultaneously
    startFlag.store(true);

    for (auto& w : writers)
    {
        w.join();
    }

    stopReaders.store(true);

    for (auto& r : readers)
    {
        r.join();
    }

    // Verify system state remains intact and queryable
    EXPECT_NO_THROW(cache.getMatchingSizeForSecurity("SecShared"));
    EXPECT_NO_THROW(cache.getAllOrders());
}

// ====================================== PHASE 5 STEP7 ======================================

TEST_F(OrderCacheTest, Scale_OneMillionOrdersStressBenchmark)
{
    constexpr unsigned int kTotalOrders = 1'000'000;
    std::cout << "\n>>> [1M ORDERS BENCHMARK] Preparing " << kTotalOrders << " orders in memory...\n";

    // Step 1: Pre-generate orders outside the timer so string allocations aren't measured
    std::vector<Order> ordersToInject;
    ordersToInject.reserve(kTotalOrders);

    for (unsigned int i = 0; i < kTotalOrders; ++i)
    {
        const std::string id = "Ord1M_" + std::to_string(i);
        const std::string sec = "Sec_" + std::to_string(i % 5);
        const std::string user = "User_" + std::to_string(i % 100);

        if (i % 2 == 0)
        {
            ordersToInject.emplace_back(id, sec, "Buy", 10, user, "CompA");
        }
        else if (i % 4 == 1)
        {
            ordersToInject.emplace_back(id, sec, "Sell", 10, user, "CompB");
        }
        else
        {
            ordersToInject.emplace_back(id, sec, "Sell", 10, user, "CompC");
        }
    }

    std::cout << ">>> Ingesting pre-allocated orders into OrderCache...\n";
    const auto ingestStart = std::chrono::high_resolution_clock::now();

    // Step 2: Measure ONLY the cache insertion throughput
    for (auto& order : ordersToInject)
    {
        cache.addOrder(std::move(order));
    }

    const auto ingestEnd = std::chrono::high_resolution_clock::now();
    const auto ingestDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(ingestEnd - ingestStart).count();

    std::cout << ">>> Pure Cache Ingestion completed in: " << ingestDurationMs << " ms ("
              << (kTotalOrders * 1000.0) / (ingestDurationMs > 0 ? ingestDurationMs : 1) << " ops/sec)\n";

    // Step 3: Benchmark isolated query latency against Sec_0
    const auto queryStart = std::chrono::high_resolution_clock::now();
    const unsigned int matched = cache.getMatchingSizeForSecurity("Sec_0");
    const auto queryEnd = std::chrono::high_resolution_clock::now();

    const auto queryDurationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(queryEnd - queryStart).count();

    std::cout << ">>> Query getMatchingSizeForSecurity(\"Sec_0\") completed in: "
              << queryDurationNs << " ns (" << queryDurationNs / 1'000.0 << " us)\n";

    EXPECT_EQ(matched, 1'000'000);
    EXPECT_LT(queryDurationNs, 50'000);
}

TEST_F(OrderCacheTest, Validation_RejectsAllEmptyFieldsAndMalformedSides)
{
    cache.addOrder(Order{"", "Sec1", "Buy", 100, "User1", "CompA"});        // Empty OrderId
    cache.addOrder(Order{"Ord1", "", "Buy", 100, "User1", "CompA"});        // Empty SecurityId
    cache.addOrder(Order{"Ord2", "Sec1", "", 100, "User1", "CompA"});       // Empty Side
    cache.addOrder(Order{"Ord3", "Sec1", "Buy", 100, "", "CompA"});         // Empty User
    cache.addOrder(Order{"Ord4", "Sec1", "Buy", 100, "User1", ""});         // Empty Company
    cache.addOrder(Order{"Ord5", "Sec1", "buy", 100, "User1", "CompA"});    // Lowercase side
    cache.addOrder(Order{"Ord6", "Sec1", "SELL", 100, "User1", "CompA"});   // Uppercase side
    cache.addOrder(Order{"Ord7", "Sec1", " Buy ", 100, "User1", "CompA"});  // Whitespace side

    EXPECT_EQ(cache.getAllOrders().size(), 0);
}

TEST_F(OrderCacheTest, OverflowProtection_HandlesLargeInstitutionalVolumesWithoutWrapping)
{
    constexpr unsigned int kHugeQty = 1'500'000'000;

    cache.addOrder(Order{"B1", "CorpBond10Y", "Buy",  kHugeQty, "Trader1", "DealerA"});
    cache.addOrder(Order{"B2", "CorpBond10Y", "Buy",  kHugeQty, "Trader2", "DealerB"});
    cache.addOrder(Order{"S1", "CorpBond10Y", "Sell", kHugeQty, "Trader3", "DealerC"});
    cache.addOrder(Order{"S2", "CorpBond10Y", "Sell", kHugeQty, "Trader4", "DealerD"});

    // TotalBuy (3B) + TotalSell (3B) = 6B, which exceeds 32-bit uint max (~4.29B).
    // Verifies internal 64-bit accumulators do not overflow during outsideBuys + outsideSells.
    EXPECT_EQ(cache.getMatchingSizeForSecurity("CorpBond10Y"), 3'000'000'000U);
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
