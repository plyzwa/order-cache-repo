// Write your unit tests for the OrderCache implementation here.
// Use the Google Test framework.

#include <string>
#include <vector>
#include "OrderCache.h"
#include "gtest/gtest.h"

class OrderCacheTest : public ::testing::Test {
protected:
    OrderCache cache;

    void SetUp() override {
        // Initialize test fixtures here if needed
    }
};

// Example test to get you started — replace with your own comprehensive tests

TEST_F(OrderCacheTest, ExampleAddOrder) {
    cache.addOrder(Order{"OrdId1", "SecId1", "Buy", 1000, "User1", "CompanyA"});
    ASSERT_EQ(cache.getAllOrders().size(), 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
