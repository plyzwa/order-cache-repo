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


TEST_F(OrderCacheTest, ExampleAddOrder)
{
    cache.addOrder(Order{"OrdId1", "SecId1", "Buy", 1000, "User1", "CompanyA"});
    ASSERT_EQ(cache.getAllOrders().size(), 1);
}

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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
