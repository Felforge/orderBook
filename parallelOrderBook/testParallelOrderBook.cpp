#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "parallelOrderBook.h"
using namespace std;

// Define external Order object
#define OrderExt Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>

// Check if two order pointers are equal in values
void isOrderEqual(Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>* expected, Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>* actual) {
    EXPECT_EQ(expected->orderID, actual->orderID);
    EXPECT_EQ(expected->userID, actual->userID);
    EXPECT_EQ(expected->priceTicks, actual->priceTicks);
    EXPECT_EQ(expected->quantity.load(), actual->quantity.load());
    EXPECT_EQ(expected->side, actual->side);
    EXPECT_EQ(expected->symbolID, actual->symbolID);
}

// Helper function to wait for order processing
void waitForProcessing() {
    this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Test registering a symbol and submitting a buy order
TEST(OrderBookTest, HandlesValidBuyOrderSubmission) {
    // Create Order Book
    // Template parameters: NumWorkers=1, MaxSymbols=10, MaxOrders=1000
    OrderBook<1, 10, 1000> orderBook;

    // Start the order book system
    orderBook.start();

    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit first buy order
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();
    
    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);
    
    // Create expected orders to compare against
    OrderExt expected1(nullptr, result->first, 1, Side::BUY, symbolID, nullptr, 100, priceToTicks(150.0), OrderType::ADD);
    
    // Verify order details using isOrderEqual
    isOrderEqual(&expected1, result->second);

    // Shutdown the order book system
    orderBook.shutdown();
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}