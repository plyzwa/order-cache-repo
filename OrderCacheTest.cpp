// Write your unit tests for the OrderCache implementation here.
// Use the Google Test framework.

#include <string>
#include <vector>
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

// ====================================== PHASE 2 STEP3 =================== ===================

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





















int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
