#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "parallelOrderBook.h"
using namespace std;

// Define external Order object
#define OrderExt Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>

// Check if two order pointers are equal in values
void isOrderEqual(OrderExt* expected, OrderExt* actual) {
    EXPECT_EQ(expected->orderID, actual->orderID);
    EXPECT_EQ(expected->userID, actual->userID);
    EXPECT_EQ(expected->priceTicks, actual->priceTicks);
    EXPECT_EQ(expected->quantity, actual->quantity);
    EXPECT_EQ(expected->side, actual->side);
    EXPECT_EQ(expected->symbolID, actual->symbolID);
}

// Helper function to wait for order processing
void waitForProcessing(int numMil = 100) {
    this_thread::sleep_for(std::chrono::milliseconds(numMil));
    // Drain remote free queues to reclaim memory from worker thread deallocations
    orderBook.drainRemoteFreeQueues();
}

// Test fixture for Single Thread OrderBook tests
// Automatically sets up, starts and shuts down the order book
class OrderBookTestSingleThread : public ::testing::Test {
    protected:
        void SetUp() override {
            // Start the order book system
            orderBook.start();
        }
    
        void TearDown() override {
            // Shutdown the order book system
            orderBook.shutdown();
        }
    
        // Template parameters: NumWorkers=1, MaxSymbols=10, MaxOrders=1000
        OrderBook<1, 10, 1000> orderBook;
};

// Test fixture for Four Thread OrderBook tests
// Automatically sets up, starts and shuts down the order book
class OrderBookTestFourThread : public ::testing::Test {
    protected:
        void SetUp() override {
            // Start the order book system
            orderBook.start();
        }
    
        void TearDown() override {
            // Shutdown the order book system
            orderBook.shutdown();
        }
    
        // Template parameters: NumWorkers=1, MaxSymbols=10, MaxOrders=1000
        OrderBook<4, 10, 1000> orderBook;
};

// Test registering a symbol and submitting a buy order
TEST_F(OrderBookTestSingleThread, HandlesValidBuyOrderSubmission) {
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
    OrderExt expected(nullptr, nullptr, result->first, 1, Side::BUY, symbolID, nullptr, 100, priceToTicks(150.0), OrderType::ADD);
    
    // Verify order details using isOrderEqual
    isOrderEqual(&expected, result->second);
}

// Test registering a symbol and submitting a sell order
TEST_F(OrderBookTestSingleThread, HandlesValidSellOrderSubmission) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit first buy order
    auto result = orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();
    
    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);
    
    // Create expected orders to compare against
    OrderExt expected(nullptr, nullptr, result->first, 1, Side::SELL, symbolID, nullptr, 100, priceToTicks(150.0), OrderType::ADD);
    
    // Verify order details using isOrderEqual
    isOrderEqual(&expected, result->second);
}

// Test order ID progression for same price orders
TEST_F(OrderBookTestSingleThread, HandlesOrderIDProgressionSamePrice) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit three buy orders
    auto result1 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result3 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_TRUE(result3.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);
    EXPECT_NE(result3->second, nullptr);
    
    // Verify order ID progression
    EXPECT_EQ(1, result2->second->orderID - result1->second->orderID);
    EXPECT_EQ(1, result3->second->orderID - result2->second->orderID);
}

// Test order ID progression for different price orders
TEST_F(OrderBookTestSingleThread, HandlesOrderIDProgressionDifferentPrice) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit three buy orders
    auto result1 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 160.0);
    auto result3 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 170.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_TRUE(result3.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);
    EXPECT_NE(result3->second, nullptr);
    
    // Verify order ID progression
    EXPECT_EQ(1, result2->second->orderID - result1->second->orderID);
    EXPECT_EQ(1, result3->second->orderID - result2->second->orderID);
}

// Test multiple symbol registration
TEST_F(OrderBookTestSingleThread, HandlesMultipleSymbols) {
    // Register symbol
    string symbolName1 = "AAPL";
    uint16_t symbolID1 = orderBook.registerSymbol(symbolName1);
    
    // Register another symbol
    string symbolName2 = "AMZN";
    uint16_t symbolID2 = orderBook.registerSymbol(symbolName2);

    // Make sure ID progesses
    EXPECT_EQ(1, symbolID2 - symbolID1);
}

// Test duplicate symbol registration
TEST_F(OrderBookTestSingleThread, HandlesDuplicateSymbols) {
    // Register symbol
    string symbolName1 = "AAPL";
    uint16_t symbolID1 = orderBook.registerSymbol(symbolName1);
    
    // Register the same symbol
    string symbolName2 = "AAPL";
    uint16_t symbolID2 = orderBook.registerSymbol(symbolName2);

    // Make sure IDs are equal
    EXPECT_EQ(symbolID1, symbolID2);
}

// Test invalid symbol order submission
TEST_F(OrderBookTestSingleThread, HandlesInvalidSymbolOrder) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit order to invalid symbol
    auto result = orderBook.submitOrder(1, symbolID+1, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();
    
    // Verify expected result
    EXPECT_FALSE(result.has_value());
}

// Test price tick precision
TEST_F(OrderBookTestSingleThread, HandlesPriceTickPrecision) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit first buy order
    auto result1 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0001);
    
    // Wait for processing
    waitForProcessing();
    
    // Verify that the order was submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_NE(result1->second, nullptr);
    
    // Create expected order to compare against
    OrderExt expected1(nullptr, nullptr, result1->first, 1, Side::BUY, symbolID, nullptr, 100, priceToTicks(150.0001), OrderType::ADD);
    
    // Verify order details using isOrderEqual
    isOrderEqual(&expected1, result1->second);

    // Submit second buy order
    auto result2 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.00009);

    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result2.has_value());
    EXPECT_NE(result2->second, nullptr);

    // Create expected order to compare against
    OrderExt expected2(nullptr, nullptr, result2->first, 1, Side::BUY, symbolID, nullptr, 100, priceToTicks(150.0001), OrderType::ADD);
    
    // Verify order details using isOrderEqual
    isOrderEqual(&expected2, result2->second);
}

// Test order ID encoding by symbol
TEST_F(OrderBookTestSingleThread, HandlesOrderIDEncoding) {
    // Register symbols
    string symbolName1 = "AAPL";
    uint16_t symbolID1 = orderBook.registerSymbol(symbolName1);
    string symbolName2 = "AMZN";
    uint16_t symbolID2 = orderBook.registerSymbol(symbolName2);
    
    // Submit three buy orders
    auto result1 = orderBook.submitOrder(1, symbolID1, Side::BUY, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID2, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);
    
    // Verify order ID encding
    EXPECT_EQ(symbolID1, result1->second->orderID >> 48);
    EXPECT_EQ(symbolID2, result2->second->orderID >> 48);
    EXPECT_NE(result1->second->orderID >> 48, result2->second->orderID >> 48);
}

// Test zero price order sumbission
TEST_F(OrderBookTestSingleThread, HandlesZeroPriceOrder) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit order to invalid symbol
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 0.0);
    
    // Wait for processing
    waitForProcessing();
    
    // Verify expected result
    EXPECT_FALSE(result.has_value());
}

// Test negative price order sumbission
TEST_F(OrderBookTestSingleThread, HandlesNegativePriceOrder) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit order to invalid symbol
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, -10.0);
    
    // Wait for processing
    waitForProcessing();
    
    // Verify expected result
    EXPECT_FALSE(result.has_value());
}

// Test zero quantity order sumbission
TEST_F(OrderBookTestSingleThread, HandlesZeroQuantityOrder) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit order to invalid symbol
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 0, 150.0);
    
    // Wait for processing
    waitForProcessing();
    
    // Verify expected result
    EXPECT_FALSE(result.has_value());
}

// Test negtaive quantity order sumbission
TEST_F(OrderBookTestSingleThread, HandlesNegativeQuantityOrder) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit order to invalid symbol
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, -10, 150.0);
    
    // Wait for processing
    waitForProcessing();
    
    // Verify expected result
    EXPECT_FALSE(result.has_value());
}

// Test node assignment and order type transition
TEST_F(OrderBookTestSingleThread, HandlesNodeAssignmentAndTypeTransition) {
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
    
    // After processing, order type should change to CANCEL
    EXPECT_EQ(OrderType::CANCEL, result->second->type);
    
    // Check node assignment
    EXPECT_NE(result->second->node, nullptr);
    
    // Check its memory block
    EXPECT_NE(result->second->node->memoryBlock, nullptr);
}

// Test removing a single order
TEST_F(OrderBookTestSingleThread, HandlesSingleOrderRemoval) {
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

    // Retrieve order
    OrderExt* order = result->second;

    // Retrieve price level
    auto priceLevel = order->symbol->buyPrices.lookup(1500000);
    
    // Verify that price level has an order
    EXPECT_EQ(priceLevel->numOrders.load(), 1);

    // Cancel order
    bool wasRemoved = orderBook.cancelOrder(order);

    // Wait for processing
    waitForProcessing();

    // Verify that order was successfuly removed
    EXPECT_TRUE(wasRemoved);

    // Verify that price level no longer has this order
    EXPECT_EQ(priceLevel->numOrders.load(), 0);
}

// Test removing a null order
TEST_F(OrderBookTestSingleThread, HandlesNullRemoval) {
    // Attempt to remove a nullptr
    bool wasRemoved = orderBook.cancelOrder(nullptr);

    // Wait for processing
    waitForProcessing();

    // Verify that order was successfuly removed
    EXPECT_FALSE(wasRemoved);
}

// Test removing multiple orders on the same price level
TEST_F(OrderBookTestSingleThread, HandlesMultipleOrderRemovalSameLevel) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit all orders
    auto result1 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result3 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_TRUE(result3.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);
    EXPECT_NE(result3->second, nullptr);

    // Retrieve orders
    OrderExt* order1 = result1->second;
    OrderExt* order2 = result2->second;
    OrderExt* order3 = result3->second;

    // Retrieve price level
    auto priceLevel = order1->symbol->buyPrices.lookup(1500000);
    
    // Verify that price level has all orders
    EXPECT_EQ(priceLevel->numOrders.load(), 3);

    // Cancel first order
    bool wasRemoved = orderBook.cancelOrder(order1);

    // Wait for processing
    waitForProcessing();

    // Verify that order was successfuly removed
    EXPECT_TRUE(wasRemoved);

    // Verify that price level no longer has this order
    EXPECT_EQ(priceLevel->numOrders.load(), 2);

    // Cancel second order
    wasRemoved = orderBook.cancelOrder(order2);

    // Wait for processing
    waitForProcessing();

    // Verify that order was successfuly removed
    EXPECT_TRUE(wasRemoved);

    // Verify that price level no longer has this order
    EXPECT_EQ(priceLevel->numOrders.load(), 1);

    // Cancel third order
    wasRemoved = orderBook.cancelOrder(order3);

    // Wait for processing
    waitForProcessing();

    // Verify that order was successfuly removed
    EXPECT_TRUE(wasRemoved);

    // Verify that price level no longer has this order
    EXPECT_EQ(priceLevel->numOrders.load(), 0);
}

// Test removing multiple orders on different price levels
TEST_F(OrderBookTestSingleThread, HandlesMultipleOrderRemovalDiffLevel) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit all orders
    auto result1 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 140.0);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result3 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 160.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_TRUE(result3.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);
    EXPECT_NE(result3->second, nullptr);

    // Retrieve orders
    OrderExt* order1 = result1->second;
    OrderExt* order2 = result2->second;
    OrderExt* order3 = result3->second;

    // Retrieve price levels
    auto priceLevel1 = order1->symbol->buyPrices.lookup(1400000);
    auto priceLevel2 = order1->symbol->buyPrices.lookup(1500000);
    auto priceLevel3 = order1->symbol->buyPrices.lookup(1600000);
    
    // Verify expected info
    EXPECT_EQ(priceLevel1->numOrders.load(), 1);
    EXPECT_EQ(priceLevel2->numOrders.load(), 1);
    EXPECT_EQ(priceLevel3->numOrders.load(), 1);

    // Cancel first order
    bool wasRemoved = orderBook.cancelOrder(order1);

    // Wait for processing
    waitForProcessing();

    // Verify that order was successfuly removed
    EXPECT_TRUE(wasRemoved);

    // Verify expected info
    EXPECT_EQ(priceLevel1->numOrders.load(), 0);
    EXPECT_EQ(priceLevel2->numOrders.load(), 1);
    EXPECT_EQ(priceLevel3->numOrders.load(), 1);

    // Cancel second order
    wasRemoved = orderBook.cancelOrder(order2);

    // Wait for processing
    waitForProcessing();

    // Verify that order was successfuly removed
    EXPECT_TRUE(wasRemoved);

    // Verify expected info
    EXPECT_EQ(priceLevel1->numOrders.load(), 0);
    EXPECT_EQ(priceLevel2->numOrders.load(), 0);
    EXPECT_EQ(priceLevel3->numOrders.load(), 1);

    // Cancel third order
    wasRemoved = orderBook.cancelOrder(order3);

    // Wait for processing
    waitForProcessing();

    // Verify that order was successfuly removed
    EXPECT_TRUE(wasRemoved);

    // Verify expected info
    EXPECT_EQ(priceLevel1->numOrders.load(), 0);
    EXPECT_EQ(priceLevel2->numOrders.load(), 0);
    EXPECT_EQ(priceLevel3->numOrders.load(), 0);
}

// Test reassigning best bid with no fall back price level
TEST_F(OrderBookTestSingleThread, HandlesReassignBestBidNoFallback) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit all orders
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Retrieve order
    OrderExt* order = result->second;

    // Retrieve symbol
    auto symbol = order->symbol;
    
    // Verify expected info
    EXPECT_EQ(symbol->bestBidTicks.load(), 1500000);

    // Cancel order
    orderBook.cancelOrder(order);
    
    // Wait for processing
    waitForProcessing();

    // Add sell order to reset best buy price
    // Needs to be at a price that would have matched
    orderBook.submitOrder(1, symbolID, Side::SELL, 100, 140.0);

    // Wait for processing
    waitForProcessing();

    // Verify expected info
    // Expected to fall back all the way to 0
    EXPECT_EQ(symbol->bestBidTicks.load(), 0);
}

// Test reassigning best bid with one fallbacks
TEST_F(OrderBookTestSingleThread, HandlesReassignBestBidWithFallback) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit all orders
    auto result1 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 149.95);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);

    // Retrieve orders
    OrderExt* order1 = result1->second;
    OrderExt* order2 = result2->second;

    // Retrieve symbol
    auto symbol = order1->symbol;
    
    // Verify expected info
    EXPECT_EQ(symbol->bestBidTicks.load(), 1500000);

    // Cancel first order
    orderBook.cancelOrder(order1);

    // Wait for processing
    waitForProcessing();

    // Add sell order to reset best buy price
    // Needs to be at a price that would have matched
    auto sellOrder = orderBook.submitOrder(1, symbolID, Side::SELL, 100, 149.99);

    // Wait for processing
    waitForProcessing();

    // Verify expected info
    EXPECT_EQ(symbol->bestBidTicks.load(), 1499500);
}

// Test reassigning best ask with no fall back price level
TEST_F(OrderBookTestSingleThread, HandlesReassignBestAskNoFallback) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit all orders
    auto result = orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Retrieve order
    OrderExt* order = result->second;

    // Retrieve symbol
    auto symbol = order->symbol;
    
    // Verify expected info
    EXPECT_EQ(symbol->bestAskTicks.load(), 1500000);

    // Cancel order
    orderBook.cancelOrder(order);

    // Wait for processing
    waitForProcessing();

    // Add sell order to reset best buy price
    // Needs to be at a price that would have matched
    orderBook.submitOrder(1, symbolID, Side::BUY, 100, 160.0);

    // Wait for processing
    waitForProcessing();

    // Verify expected info
    // Expected to fall back all the way to UINT64_MAX
    EXPECT_EQ(symbol->bestAskTicks.load(), UINT64_MAX);
}

// Test reassigning best ask with one fallbacks
TEST_F(OrderBookTestSingleThread, HandlesReassignBestAskWithFallback) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit all orders
    auto result1 = orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.05);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);

    // Retrieve orders
    OrderExt* order1 = result1->second;
    OrderExt* order2 = result2->second;

    // Retrieve symbol
    auto symbol = order1->symbol;
    
    // Verify expected info
    EXPECT_EQ(symbol->bestAskTicks.load(), 1500000);

    // Cancel first order
    orderBook.cancelOrder(order1);

    // Wait for processing
    waitForProcessing();

    // Add sell order to reset best buy price
    // Needs to be at a price that would have matched
    auto sellOrder = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.01);

    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(sellOrder.has_value());
    EXPECT_NE(sellOrder->second, nullptr);

    // Verify expected info
    EXPECT_EQ(symbol->bestAskTicks.load(), 1500500);
}

// Test removing orders across symbols
TEST_F(OrderBookTestSingleThread, HandlesDuplicateOrderRemoval) {
    // Register first symbol
    string symbolName1 = "AAPL";
    uint16_t symbolID1 = orderBook.registerSymbol(symbolName1);

    // Register second symbol
    string symbolName2 = "AMZN";
    uint16_t symbolID2 = orderBook.registerSymbol(symbolName2);
    
    // Submit buy orders
    auto result1 = orderBook.submitOrder(1, symbolID1, Side::BUY, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID2, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);

    // Retrieve orders
    OrderExt* order1 = result1->second;
    OrderExt* order2 = result2->second;

    // Retrieve price levels
    auto priceLevel1 = order1->symbol->buyPrices.lookup(1500000);
    auto priceLevel2 = order2->symbol->buyPrices.lookup(1500000);

    // Verify expected state
    EXPECT_EQ(priceLevel1->numOrders.load(), 1);
    EXPECT_EQ(priceLevel2->numOrders.load(), 1);

    // Cancel first order
    bool wasRemoved = orderBook.cancelOrder(order1);

    // Wait for processing
    waitForProcessing();

    // Verify that order was successfuly removed
    EXPECT_TRUE(wasRemoved);

    // Verify expected state
    EXPECT_EQ(priceLevel1->numOrders.load(), 0);
    EXPECT_EQ(priceLevel2->numOrders.load(), 1);

    // Cancel second order
    wasRemoved = orderBook.cancelOrder(order2);

    // Wait for processing
    waitForProcessing();

    // Verify that order was successfuly removed
    EXPECT_TRUE(wasRemoved);

    // Verify expected state
    EXPECT_EQ(priceLevel1->numOrders.load(), 0);
    EXPECT_EQ(priceLevel2->numOrders.load(), 0);
}

// Test reassigning best bids across symbols
// Basically ensuring that it is isolated between symbols
TEST_F(OrderBookTestSingleThread, HandlesBestBidCrossSymbol) {
    // Register symbol
    string symbolName1 = "AAPL";
    uint16_t symbolID1 = orderBook.registerSymbol(symbolName1);

    // Register symbol
    string symbolName2 = "AMZN";
    uint16_t symbolID2 = orderBook.registerSymbol(symbolName2);
    
    // Submit all orders
    auto result1 = orderBook.submitOrder(1, symbolID1, Side::BUY, 100, 100.0);
    auto result2 = orderBook.submitOrder(1, symbolID2, Side::BUY, 100, 150.0);
    orderBook.submitOrder(1, symbolID1, Side::BUY, 100, 99.95);
    orderBook.submitOrder(1, symbolID2, Side::BUY, 100, 149.95);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);

    // Retrieve orders
    OrderExt* order1 = result1->second;
    OrderExt* order2 = result2->second;

    // Retrieve symbols
    auto symbol1 = order1->symbol;
    auto symbol2 = order2->symbol;
    
    // Verify expected info
    EXPECT_EQ(symbol1->bestBidTicks.load(), 1000000);
    EXPECT_EQ(symbol2->bestBidTicks.load(), 1500000);

    // Cancel first order
    orderBook.cancelOrder(order1);

    // Wait for processing
    waitForProcessing();

    // Add sell order to reset best buy price on Symbol 1
    // Needs to be at a price that would have matched
    auto result3 = orderBook.submitOrder(1, symbolID1, Side::SELL, 100, 99.99);

    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result3.has_value());
    EXPECT_NE(result3->second, nullptr);

    // Verify expected info
    EXPECT_EQ(symbol1->bestBidTicks.load(), 999500);
    EXPECT_EQ(symbol2->bestBidTicks.load(), 1500000);

    // Cancel second order
    orderBook.cancelOrder(order2);

    // Wait for processing
    waitForProcessing();

    // Add sell order to reset best buy price on Symbol 2
    // Needs to be at a price that would have matched
    auto result4 = orderBook.submitOrder(1, symbolID2, Side::SELL, 100, 149.99);

    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result4.has_value());
    EXPECT_NE(result4->second, nullptr);

    // Verify expected info
    EXPECT_EQ(symbol1->bestBidTicks.load(), 999500);
    EXPECT_EQ(symbol2->bestBidTicks.load(), 1499500);
}

// Test a match at the same price and quantity with buy being submitted first and then sell
TEST_F(OrderBookTestSingleThread, HandlesSimpleEqualMatchBuySell) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit buy order
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Retrieve symbol
    auto symbol = result->second->symbol;
    
    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);

    // Wait for processing
    waitForProcessing();
    
    // Retrieve price levels
    auto buyLevel = symbol->buyPrices.lookup(1500000);
    auto sellLevel = symbol->sellPrices.lookup(1500000);

    // Verify expected results
    // Buy Level is expected to have 0 orders
    EXPECT_EQ(buyLevel->numOrders.load(), 0);
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(sellLevel, nullptr);
}

// Test a match at the same price and quantity with sell being submitted first and then buy
TEST_F(OrderBookTestSingleThread, HandlesSimpleEqualMatchSellBuy) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit buy order
    auto result = orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Retrieve symbol
    auto symbol = result->second->symbol;
    
    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

    // Wait for processing
    waitForProcessing();
    
    // Retrieve price levels
    auto buyLevel = symbol->buyPrices.lookup(1500000);
    auto sellLevel = symbol->sellPrices.lookup(1500000);

    // Verify expected results
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(buyLevel, nullptr);
    // Buy Level is expected to have 0 orders
    EXPECT_EQ(sellLevel->numOrders, 0);
}

// Test a match at the same price buy higher buy qauntity
TEST_F(OrderBookTestSingleThread, HandlesMoreBuyEqualMatch) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit buy order
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 125, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Retrieve buy order
    OrderExt* order = result->second;

    // Retrieve symbol
    auto symbol = order->symbol;
    
    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);

    // Wait for processing
    waitForProcessing();
    
    // Retrieve price levels
    auto buyLevel = symbol->buyPrices.lookup(1500000);
    auto sellLevel = symbol->sellPrices.lookup(1500000);

    // Verify expected results
    // Buy Level is expected to have 1 order
    // The order should have not gotten deleted
    EXPECT_EQ(buyLevel->numOrders.load(), 1);
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(sellLevel, nullptr);
    // Make sure the order's quantity decreased
    EXPECT_EQ(order->quantity, 25);
}

// Test a match at the same price buy higher sell qauntity
TEST_F(OrderBookTestSingleThread, HandlesMoreSellEqualMatch) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit buy order
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);
    
    // Submit matching sell order
    result = orderBook.submitOrder(1, symbolID, Side::SELL, 125, 150.0);

    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Retrieve sell order
    OrderExt* order = result->second;

    // Retrieve symbol
    auto symbol = order->symbol;
    
    // Retrieve price levels
    auto buyLevel = symbol->buyPrices.lookup(1500000);
    auto sellLevel = symbol->sellPrices.lookup(1500000);

    // Verify expected results
    // Buy Level is expected to have 0 orders
    EXPECT_EQ(buyLevel->numOrders.load(), 0);
    // Sell Level should have gotten created and should have retained one order
    // This is because the order only got partially matched
    EXPECT_EQ(sellLevel->numOrders.load(), 1);
    // Make sure the sell order's quantity decreased
    EXPECT_EQ(order->quantity, 25);
}

// Test a match at a different price and the same quantity
TEST_F(OrderBookTestSingleThread, HandlesSimplePriceCrossMatch) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit buy order
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 160.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Retrieve symbol
    auto symbol = result->second->symbol;
    
    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);

    // Wait for processing
    waitForProcessing();
    
    // Retrieve price levels
    auto buyLevel = symbol->buyPrices.lookup(1600000);
    auto sellLevel = symbol->sellPrices.lookup(1500000);

    // Verify expected results
    // Buy Level is expected to have 0 orders
    EXPECT_EQ(buyLevel->numOrders.load(), 0);
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(sellLevel, nullptr);
    // Order should have been matched at 150, but there is no good way to check this
}

// Test a match at the same price and three orders matching the other's quantity
TEST_F(OrderBookTestSingleThread, HandlesMultipleEqualMatch) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit buy order
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result1 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_NE(result->second, nullptr);
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);

    // Retrieve symbol
    auto symbol = result->second->symbol;
    
    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::SELL, 300, 150.0);

    // Wait for processing
    waitForProcessing();
    
    // Retrieve price levels
    auto buyLevel = symbol->buyPrices.lookup(1500000);
    auto sellLevel = symbol->sellPrices.lookup(1500000);

    // Verify expected results
    // Buy Level is expected to have 0 orders
    EXPECT_EQ(buyLevel->numOrders.load(), 0);
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(sellLevel, nullptr);
}

// Test a match at the same price and three orders exceeding the other's quantity
TEST_F(OrderBookTestSingleThread, HandlesMultiplePartialMatch) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit buy order
    auto result1 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_NE(result->second, nullptr);
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);

    // Retrieve 3rd buy order
    OrderExt* order = result->second;

    // Retrieve symbol
    auto symbol = order->symbol;
    
    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::SELL, 250, 150.0);

    // Wait for processing
    waitForProcessing();
    
    // Retrieve price levels
    auto buyLevel = symbol->buyPrices.lookup(1500000);
    auto sellLevel = symbol->sellPrices.lookup(1500000);

    // Verify expected results
    // Buy Level is expected to have 1 order
    EXPECT_EQ(buyLevel->numOrders.load(), 1);
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(sellLevel, nullptr);
    // Make sure the 3rd buy order's quantity decreased
    EXPECT_EQ(order->quantity, 50);
}

// Verify FIFO ordering on the buy side
TEST_F(OrderBookTestSingleThread, HandlesFIFOOrderingBuy) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit buy orders
    auto result1 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);

    // Retrieve orders
    OrderExt* order1 = result1->second;
    OrderExt* order2 = result2->second;

    // Retrieve symbol
    auto symbol = order1->symbol;

    // Retrieve price levels
    auto buyLevel = symbol->buyPrices.lookup(1500000);
    auto sellLevel = symbol->sellPrices.lookup(1500000);
    
    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::SELL, 50, 150.0);

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    // Buy Level is expected to have 2 orders
    EXPECT_EQ(buyLevel->numOrders.load(), 2);
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(sellLevel, nullptr);
    // Make sure the 1st buy order's quantity decreased
    EXPECT_EQ(order1->quantity, 50);

    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    // Buy Level is expected to have 1 order
    EXPECT_EQ(buyLevel->numOrders.load(), 1);
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(sellLevel, nullptr);
    // Make sure the 2nd buy order's quantity decreased
    EXPECT_EQ(order2->quantity, 50);
}

// Verify that price change can break FIFO ordering
TEST_F(OrderBookTestSingleThread, HandlesPriceChangeBreaksFIFO) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit buy orders
    auto result1 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 149.9);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 149.95);
    auto result3 = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_TRUE(result3.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);
    EXPECT_NE(result3->second, nullptr);

    // Retrieve orders
    OrderExt* order1 = result1->second;
    OrderExt* order2 = result2->second;
    OrderExt* order3 = result3->second;

    // Retrieve symbol
    auto symbol = order1->symbol;

    // Retrieve price levels
    auto buyLevel1 = symbol->buyPrices.lookup(1499000);
    auto buyLevel2 = symbol->buyPrices.lookup(1499500);
    auto buyLevel3 = symbol->buyPrices.lookup(1500000);
    auto sellLevel = symbol->sellPrices.lookup(1500000);
    
    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::SELL, 50, 140.0);

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    // First Buy Level is expected to have 1 order
    EXPECT_EQ(buyLevel1->numOrders.load(), 1);
    // Second Buy Level is expected to have 1 order
    EXPECT_EQ(buyLevel2->numOrders.load(), 1);
    // Third Buy Level is expected to have 1 order
    EXPECT_EQ(buyLevel3->numOrders.load(), 1);
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(sellLevel, nullptr);
    // Make sure the 3rd buy order's quantity decreased
    EXPECT_EQ(order3->quantity, 50);

    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::SELL, 100, 140.0);

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    // First Buy Level is expected to have 1 order
    EXPECT_EQ(buyLevel1->numOrders.load(), 1);
    // Second Buy Level is expected to have 1 order
    EXPECT_EQ(buyLevel2->numOrders.load(), 1);
    // Third Buy Level is expected to have 0 orders
    EXPECT_EQ(buyLevel3->numOrders.load(), 0);
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(sellLevel, nullptr);
    // Make sure the 1st buy order's quantity decreased
    EXPECT_EQ(order2->quantity, 50);

    // Submit matching sell order
    orderBook.submitOrder(1, symbolID, Side::SELL, 100, 140.0);

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    // First Buy Level is expected to have 1 order
    EXPECT_EQ(buyLevel1->numOrders.load(), 1);
    // Second Buy Level is expected to have 1 order
    EXPECT_EQ(buyLevel2->numOrders.load(), 0);
    // Third Buy Level is expected to have 0 orders
    EXPECT_EQ(buyLevel3->numOrders.load(), 0);
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(sellLevel, nullptr);
    // Make sure the 2nd buy order's quantity decreased
    EXPECT_EQ(order1->quantity, 50);
}

// Verify FIFO ordering on the sell side
TEST_F(OrderBookTestSingleThread, HandlesFIFOOrderingSell) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit sell orders
    auto result1 = orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);
    
    // Wait for processing
    waitForProcessing();

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);

    // Retrieve orders
    OrderExt* order1 = result1->second;
    OrderExt* order2 = result2->second;

    // Retrieve symbol
    auto symbol = order1->symbol;

    // Retrieve price levels
    auto buyLevel = symbol->buyPrices.lookup(1500000);
    auto sellLevel = symbol->sellPrices.lookup(1500000);
    
    // Submit matching buy order
    auto result3 = orderBook.submitOrder(1, symbolID, Side::BUY, 50, 150.0);

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(buyLevel, nullptr);
    // Buy Level is expected to have 2 orders
    EXPECT_EQ(sellLevel->numOrders.load(), 2);
    // Make sure the 1st sell order's quantity decreased
    EXPECT_EQ(order1->quantity, 50);

    // Submit matching buy order
    orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    // Sell Level is expected to be nullptr as it never got created
    EXPECT_EQ(buyLevel, nullptr);
    // Buy Level is expected to have 2 orders
    EXPECT_EQ(sellLevel->numOrders.load(), 1);
    // Make sure the 2nd sell order's quantity decreased
    EXPECT_EQ(order2->quantity, 50);
}


// Test submitting 100 orders concurrently
TEST_F(OrderBookTestFourThread, HandlesConcurrentOrderSubmission) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);

    // Submit first order
    // This is done to be able to get the price level pointer
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Submit 99 more orders
    for (int i = 0; i < 99; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);
    }

    // Wait for processing
    waitForProcessing();

    // Retrieve Price Level
    auto buyLevel = result->second->symbol->buyPrices.lookup(1500000);

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 100);
}

// Test submitting 100 orders concurrently to different price levels
TEST_F(OrderBookTestFourThread, HandlesConcurrentOrderSubmissionDiffPrice) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit first order
    // This is done to be able to get the symbol pointer
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Submit 99 more orders
    for (double i = 1.0; i < 100.0; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0 + i);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);
    }
    
    // Wait for processing
    waitForProcessing();
    
    // Retrieve Symbol
    auto symbol = result->second->symbol;

    // Check all price levels
    for (int i = 0; i < 100; i++) {
        auto buyLevel = symbol->buyPrices.lookup(1500000 + 10000 * i);
        EXPECT_EQ(buyLevel->numOrders.load(), 1);
    }
}

// Test cancelling 100 orders concurrently
TEST_F(OrderBookTestFourThread, HandlesConcurrentOrderCancellation) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);

    // Create vector to hold orders
    vector<OrderExt*> orders;
    
    // Submit first order
    // This is done to be able to get the price level pointer
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Store order
    orders.push_back(result->second);

    // Submit and store 99 more orders
    for (int i = 0; i < 99; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);

        orders.push_back(result->second);
    }
    
    // Wait for processing
    waitForProcessing();
    
    // Retrieve Price Level
    auto buyLevel = result->second->symbol->buyPrices.lookup(1500000);

    // Remove all 100 orders
    for (OrderExt* order : orders) {
        bool wasRemoved = orderBook.cancelOrder(order);

        // Verify that order was removed
        EXPECT_TRUE(wasRemoved);
    }

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 0);
}

// Test matching 200 orders concurrently
TEST_F(OrderBookTestFourThread, HandlesConcurrentOrderMatching) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit first order
    // This is done to be able to get the price level pointer
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Submit 99 more orders
    for (int i = 0; i < 99; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);
    }
    
    // Wait for processing
    waitForProcessing();
    
    // Retrieve Price Level
    auto buyLevel = result->second->symbol->buyPrices.lookup(1500000);

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 100);

    // Submit 100 sell orders
    for (int i = 0; i < 100; i++) {
        orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);
    }
    
    // Wait for processing
    waitForProcessing();

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 0);
}

// Test matching 200 orders concurrently on different price levels
TEST_F(OrderBookTestFourThread, HandlesConcurrentOrderMatchingDiffPrice) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit first order
    // This is done to be able to get the price level pointer
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Submit and store 99 more orders
    for (int i = 0; i < 99; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);
    }
    
    // Wait for processing
    waitForProcessing();
    
    // Retrieve Price Level
    auto buyLevel = result->second->symbol->buyPrices.lookup(1500000);

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 100);

    // Submit and store 100 sell orders
    for (double i = 0.0; i < 100.0; i++) {
        orderBook.submitOrder(1, symbolID, Side::SELL, 100, 50.0 + i);
    }
    
    // Wait for processing
    waitForProcessing();

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 0);
}

// Simultaneously: submit orders, cancel orders, and match orders
TEST_F(OrderBookTestFourThread, HandlesMixedConcurrentOperations) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);

    // Create vector to hold orders
    vector<OrderExt*> orders;

    // Submit first order
    // This is done to be able to get the price level pointer
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Store order
    orders.push_back(result->second);

    // Submit the other 49 orders
    for (int i = 1; i < 50; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);

        orders.push_back(result->second);
    }
    
    // Wait for processing
    waitForProcessing();

    // Retrieve symbol
    auto symbol = result->second->symbol;

    // Verify state
    auto buyLevel150 = symbol->buyPrices.lookup(1500000);
    EXPECT_EQ(buyLevel150->numOrders.load(), 50);

    // Submit orders at 148.0 (safe to cancel - won't be matched)
    vector<OrderExt*> safeOrders;
    for (int i = 0; i < 25; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 148.0);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);

        safeOrders.push_back(result->second);
    }

    // Wait for processing
    waitForProcessing();

    // Verify state
    auto buyLevel148 = symbol->buyPrices.lookup(1480000);
    EXPECT_EQ(buyLevel148->numOrders.load(), 25);
    EXPECT_EQ(buyLevel150->numOrders.load(), 50);

    // Mixed operations - submit new orders, cancel old ones, and match
    // Cancel safe orders (148.0 - won't be matched)
    for (int i = 0; i < 25; i++) {
        // Add to 149
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 149.0);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);

        orders.push_back(result->second);

        // Cancel an order from 148
        bool wasRemoved = orderBook.cancelOrder(safeOrders[i]);

        // Make sure order was removed
        EXPECT_TRUE(wasRemoved);

        // Send two matches (we need 50)
        orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);
        orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);
    }

    // Wait for all mixed operations to complete
    waitForProcessing();

    // Verify final state
    auto buyLevel149 = symbol->buyPrices.lookup(1490000);
    EXPECT_EQ(buyLevel148->numOrders.load(), 0);
    EXPECT_EQ(buyLevel149->numOrders.load(), 25);
    EXPECT_EQ(buyLevel150->numOrders.load(), 0);
}

// Submit until pools are nearly full, then cancel all
// Submit again to verify memory was properly reclaimed
TEST_F(OrderBookTestFourThread, HandlesMemoryPoolStress) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);

    // Create vector to hold orders
    vector<OrderExt*> orders;

    // Submit first order
    // This is done to be able to get the price level pointer
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Store order
    orders.push_back(result->second);

    // Submit the other 999 orders to fill up memory pool
    for (int i = 1; i < 1000; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);

        orders.push_back(result->second);
    }
    
    // Wait for processing
    waitForProcessing();

    // Retrieve Price Level
    auto buyLevel = result->second->symbol->buyPrices.lookup(1500000);

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 1000);
    
    // Make sure pool is full
    EXPECT_THROW(orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0), bad_alloc);

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 1000);

    // Remove all 1000 orders
    for (OrderExt* order : orders) {
        bool wasRemoved = orderBook.cancelOrder(order);

        // Make sure order was removed
        EXPECT_TRUE(wasRemoved);
    }

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 0);

    // Drain the pool again
    for (int i = 0; i < 1000; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);
    }

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 1000);
}

// Heavy trading on 3 different symbols simultaneously
// Verify symbol isolation (no cross-contamination)
TEST_F(OrderBookTestFourThread, HandlesMultipleSymbolsConcurrent) {
    // Register symbols
    string symbolName1 = "AAPL";
    uint16_t symbolID1 = orderBook.registerSymbol(symbolName1);
    string symbolName2 = "AMZN";
    uint16_t symbolID2 = orderBook.registerSymbol(symbolName2);
    string symbolName3 = "MSFT";
    uint16_t symbolID3 = orderBook.registerSymbol(symbolName3);

    // Create vector to hold orders
    vector<OrderExt*> orders;
    
    // Submit first order for each
    // This is done to be able to get the price level pointer
    auto result1 = orderBook.submitOrder(1, symbolID1, Side::BUY, 100, 150.0);
    auto result2 = orderBook.submitOrder(1, symbolID2, Side::BUY, 100, 150.0);
    auto result3 = orderBook.submitOrder(1, symbolID3, Side::BUY, 100, 150.0);

    // Verify that the orders were submitted successfully
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_TRUE(result3.has_value());
    EXPECT_NE(result1->second, nullptr);
    EXPECT_NE(result2->second, nullptr);
    EXPECT_NE(result3->second, nullptr);

    // Store orders
    orders.push_back(result1->second);
    orders.push_back(result2->second);
    orders.push_back(result3->second);

    // Submit and store 99 more orders for each
    for (int i = 0; i < 99; i++) {
        result1 = orderBook.submitOrder(1, symbolID1, Side::BUY, 100, 150.0);
        result2 = orderBook.submitOrder(1, symbolID2, Side::BUY, 100, 150.0);
        result3 = orderBook.submitOrder(1, symbolID3, Side::BUY, 100, 150.0);

        // Verify that the orders were submitted successfully
        EXPECT_TRUE(result1.has_value());
        EXPECT_TRUE(result2.has_value());
        EXPECT_TRUE(result3.has_value());
        EXPECT_NE(result1->second, nullptr);
        EXPECT_NE(result2->second, nullptr);
        EXPECT_NE(result3->second, nullptr);

        orders.push_back(result1->second);
        orders.push_back(result2->second);
        orders.push_back(result3->second);
    }
    
    // Wait for processing
    waitForProcessing();
    
    // Retrieve Price Levels
    auto buyLevel1 = result1->second->symbol->buyPrices.lookup(1500000);
    auto buyLevel2 = result2->second->symbol->buyPrices.lookup(1500000);
    auto buyLevel3 = result3->second->symbol->buyPrices.lookup(1500000);

    // Verify expected results
    EXPECT_EQ(buyLevel1->numOrders.load(), 100);
    EXPECT_EQ(buyLevel2->numOrders.load(), 100);
    EXPECT_EQ(buyLevel3->numOrders.load(), 100);

    // Remove all 300 orders
    for (OrderExt* order : orders) {
        bool wasRemoved = orderBook.cancelOrder(order);

        // Make sure order was removed
        EXPECT_TRUE(wasRemoved);
    }

    // Wait for processing
    waitForProcessing();

    // Verify expected results
    EXPECT_EQ(buyLevel1->numOrders.load(), 0);
    EXPECT_EQ(buyLevel2->numOrders.load(), 0);
    EXPECT_EQ(buyLevel3->numOrders.load(), 0);
}

// Submit orders at different prices concurrently
// Match with one order and verify highest price wins
TEST_F(OrderBookTestFourThread, HandlesPriceTimePriority) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);
    
    // Submit first order
    // This is done to be able to get the price level pointer
    auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);

    // Verify that the order was submitted successfully
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result->second, nullptr);

    // Submit and store 99 more orders
    for (double i = 1.0; i < 100.0; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0 - i);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);
    }
    
    // Wait for processing
    waitForProcessing();

    // Retrieve Symbol
    auto symbol = result->second->symbol;
    
    // Retrieve Price Level
    auto buyLevel = result->second->symbol->buyPrices.lookup(1500000);

    // Verify expected results
    for (int i = 0; i < 100; i++)
    EXPECT_EQ(buyLevel->numOrders.load(), 1);

    // Submit and store 100 sell orders
    for (int i = 0; i < 100; i++) {
        result = orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);

        // Verify that the order was submitted successfully
        EXPECT_TRUE(result.has_value());
        EXPECT_NE(result->second, nullptr);
    }
    
    // Wait for processing
    waitForProcessing();

    // Verify expected results
    EXPECT_EQ(buyLevel->numOrders.load(), 0);
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}