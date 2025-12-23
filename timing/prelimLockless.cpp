#include <gtest/gtest.h>
#include <chrono>
#include "../parallelOrderBook/parallelOrderBook.h"
using namespace std;

// Define external Order object
#define OrderExt Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>

// Test fixture for Timing on Sixteen Threads
class OrderBookSixteenThread: public ::testing::Test {
    protected:
        void SetUp() override {
            // Start the order book system
            orderBook.start();
        }
    
        // Template parameters: NumWorkers=16, MaxSymbols=10, MaxOrders=1e6
        OrderBook<16, 1, 1000000> orderBook;
};

// Time 100,000 order submissions on the same price level
TEST_F(OrderBookSixteenThread, Handles100KSamePriceLevel) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);

    // Number of orders;
    int N = 1e5;
    
    // Get start time
    auto start = chrono::high_resolution_clock::now();
    
    // Submit all orders
    for (int i = 0; i < N; i++) {
        orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
    }

    // Wait for workers to finish
    while (!orderBook.isIdle()) {
        this_thread::sleep_for(chrono::microseconds(1));
    }

    // Get end time
    auto end = chrono::high_resolution_clock::now();

    // Calculate total runtime in microseconds
    int64_t totalTime = chrono::duration_cast<chrono::microseconds>(end - start).count();

    // Print results
    cout << "Runtime: " << totalTime << " µs" << endl;
    cout << "Throughput: " << (N * 1e6 / totalTime) << " ops/sec" << endl;
}

// Time 100,000 order submissions on 100 different price levels
TEST_F(OrderBookSixteenThread, Handles100KDiffPriceLevel) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);

    // Number of orders;
    int N = 1e5;

    // Create price levels
    // N MUST be divisible by numLevels
    int numLevels = 100;
    vector<OrderExt*> orders;
    for (int i = 0; i < numLevels; i++) {
        auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0 + float(i));
        OrderExt* order = result->second;
        orders.push_back(order);
    }

    // Clear price levels
    for (OrderExt* order: orders) {
        orderBook.cancelOrder(order);
    }

    // Calculate number of orders per price level
    int perLevel = N / numLevels;
    
    // Get start time
    auto start = chrono::high_resolution_clock::now();
    
    // Submit all orders
    for (int i = 0; i < perLevel; i++) {
        for (int j = 0; j < numLevels; j++) {
            orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0 + float(j));
        }
    }

    // Wait for workers to finish
    while (!orderBook.isIdle()) {
        this_thread::sleep_for(chrono::microseconds(1));
    }

    // Get end time
    auto end = chrono::high_resolution_clock::now();

    // Calculate total runtime in microseconds
    int64_t totalTime = chrono::duration_cast<chrono::microseconds>(end - start).count();

    // Print results
    cout << "Runtime: " << totalTime << " µs" << endl;
    cout << "Throughput: " << (N * 1e6 / totalTime) << " ops/sec" << endl;
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}