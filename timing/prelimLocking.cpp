#include <gtest/gtest.h>
#include <chrono>
#include "../lockingOrderBook/lockingOrderBook.h"
using namespace std;

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