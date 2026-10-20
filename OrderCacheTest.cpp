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

// =================== PHASE 1 STEP0 =================== 
TEST_F(OrderCacheTest, ExampleAddOrder)
{
    cache.addOrder(Order{"OrdId1", "SecId1", "Buy", 1000, "User1", "CompanyA"});
    ASSERT_EQ(cache.getAllOrders().size(), 1);
}

// =================== PHASE 1 STEP1 =================== 
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

// =================== PHASE 1 STEP2 =================== 

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

TEST_F(OrderCacheTest, CancelOrdersForSecIdWithMinimumQty_NonExistentSecurityIsNoOp) {
    cache.addOrder(Order{"Ord1", "SecA", "Buy", 100, "User1", "Comp1"});
    
    EXPECT_NO_THROW(cache.cancelOrdersForSecIdWithMinimumQty("SecUnknown", 50));
    EXPECT_EQ(cache.getAllOrders().size(), 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
