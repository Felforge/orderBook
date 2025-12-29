#include <gtest/gtest.h>
#include <chrono>
#include <random>
#include "../lockingOrderBook/lockingOrderBook.h"
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
    // Note, this approach will underestimate total runtime but for large N it is negligible
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
    // Note, this approach will underestimate total runtime but for large N it is negligible
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

// Time 100,000 submissions on the same price level
// Roughly 70% will be submits and 30% will be cancels
// Note that this uses different random decisions every time
// For the actual timing these decisions will be pre-generated for replicable results
TEST_F(OrderBookSixteenThread, Handles100KSamePriceLevelMixedOps) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);

    // Number of orders;
    int N = 1e5;

    // Setup Random Number Generator
    // Decisions will be pre-generated to eliminate overhead
    vector<bool> decisions(N);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 99);

    // Generate decisions
    // true will be submit, false will be cancel
    for (int i = 0; i < N; i++) {
        decisions[i] = (dis(gen) < 70);
    }

    // Create vector to track orders
    // Reserve space to lower overhead
    vector<OrderExt*> orders;
    orders.reserve(N);

    // Submit and track 1000 initial orders
    for (int i = 0; i < 1000; i++) {
        auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
        if (result) {
            orders.push_back(result->second);
        }
    }
    
    // Get start time
    auto start = chrono::high_resolution_clock::now();
    
    // Submit all orders
    for (int i = 0; i < N; i++) {
        if (decisions[i]) {
            auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
            if (result) {
                orders.push_back(result->second);
            }
        } else {
            // Technically this should have an empty check but I added 1000 initial
            // It would be astronomically low odds to fail here
            orderBook.cancelOrder(orders.back());
            orders.pop_back();
        }
    }

    // Wait for workers to finish
    // Note, this approach will underestimate total runtime but for large N it is negligible
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

// Time 100,000 submissions on 100 different price levels
// Roughly 70% will be submits and 30% will be cancels
// Note that this uses different random decisions every time
// For the actual timing these decisions will be pre-generated for replicable results
TEST_F(OrderBookSixteenThread, Handles100KDiffPriceLevelMixedOps) {
    // Register symbol
    string symbolName = "AAPL";
    uint16_t symbolID = orderBook.registerSymbol(symbolName);

    // Number of orders;
    int N = 1e5;

    // Setup Random Number Generator
    // Decisions will be pre-generated to eliminate overhead
    vector<bool> decisions(N);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 99);

    // Generate decisions
    // true will be submit, false will be cancel
    for (int i = 0; i < N; i++) {
        decisions[i] = (dis(gen) < 70);
    }

    // Create vector to track orders
    // Reserve space to lower overhead
    vector<OrderExt*> orders;
    orders.reserve(N);

    // Create price levels
    // N MUST be divisible by numLevels
    // This is also used to create 100 initial orders per price level
    int numLevels = 100;
    for (int i = 0; i < numLevels; i++) {
        for (int j = 0; j < 100; j++) {
            auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0 + float(i));
            OrderExt* order = result->second;
            orders.push_back(order);
        }
    }

    // Calculate number of orders per price level
    int perLevel = N / numLevels;
    
    // Get start time
    auto start = chrono::high_resolution_clock::now();

    // Submit all orders
    for (int i = 0; i < perLevel; i++) {
        for (int j = 0; j < numLevels; j++) {
            // This formula gets us linear indexing
            // [0 - 999] * 100 + [0 - 99] -> [0 - 99,999]
            if (decisions[i*numLevels + j]) {
                auto result = orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0 + float(j));
                if (result) {
                    orders.push_back(result->second);
                }
            } else {
                // Technically this should have an empty check but I added 1000 initial
                // It would be astronomically low odds to fail here
                orderBook.cancelOrder(orders.back());
                orders.pop_back();
            }
        }
    }

    // Wait for workers to finish
    // Note, this approach will underestimate total runtime but for large N it is negligible
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