#include <gtest/gtest.h>
#include <fstream>
#include <chrono>
#include "../../../lockingOrderBook/lockingOrderBook.h"
using namespace std;

// Case Two distributes 100,000 orders across 100 different price levels
// This is done in a fixed sequence going 1 to 100 and then looping again
// The 100 price levels is fairly arbitrary but works to show what we need

// Define external Order object
#define OrderExt Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>

// Base template for test fixtures with different worker counts
template<size_t NumWorkers>
class OrderBookTimingCaseTwoBase : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        symbolID = orderBook.registerSymbol(symbolName);
    }

    // Template parameters: NumWorkers, MaxSymbols=1, MaxOrders=1e6
    OrderBook<NumWorkers, 1, 1000000> orderBook;
    uint16_t symbolID;

    // Helper method to run the timing test
    void runTest(const string& csvFile) {
        // Open file in append mode
        ofstream file(csvFile, ios::app);

        if (!file.is_open()) {
            cerr << "Failed to open CSV file" << endl;
            FAIL();
            return;
        }

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

        // Print results to file
        file << NumWorkers << "," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        file.close();
    }
};

// Create specific test fixtures for each worker count (1-32)
using OrderBookTimingCaseTwo1 = OrderBookTimingCaseTwoBase<1>;
using OrderBookTimingCaseTwo2 = OrderBookTimingCaseTwoBase<2>;
using OrderBookTimingCaseTwo3 = OrderBookTimingCaseTwoBase<3>;
using OrderBookTimingCaseTwo4 = OrderBookTimingCaseTwoBase<4>;
using OrderBookTimingCaseTwo5 = OrderBookTimingCaseTwoBase<5>;
using OrderBookTimingCaseTwo6 = OrderBookTimingCaseTwoBase<6>;
using OrderBookTimingCaseTwo7 = OrderBookTimingCaseTwoBase<7>;
using OrderBookTimingCaseTwo8 = OrderBookTimingCaseTwoBase<8>;
using OrderBookTimingCaseTwo9 = OrderBookTimingCaseTwoBase<9>;
using OrderBookTimingCaseTwo10 = OrderBookTimingCaseTwoBase<10>;
using OrderBookTimingCaseTwo11 = OrderBookTimingCaseTwoBase<11>;
using OrderBookTimingCaseTwo12 = OrderBookTimingCaseTwoBase<12>;
using OrderBookTimingCaseTwo13 = OrderBookTimingCaseTwoBase<13>;
using OrderBookTimingCaseTwo14 = OrderBookTimingCaseTwoBase<14>;
using OrderBookTimingCaseTwo15 = OrderBookTimingCaseTwoBase<15>;
using OrderBookTimingCaseTwo16 = OrderBookTimingCaseTwoBase<16>;
using OrderBookTimingCaseTwo17 = OrderBookTimingCaseTwoBase<17>;
using OrderBookTimingCaseTwo18 = OrderBookTimingCaseTwoBase<18>;
using OrderBookTimingCaseTwo19 = OrderBookTimingCaseTwoBase<19>;
using OrderBookTimingCaseTwo20 = OrderBookTimingCaseTwoBase<20>;
using OrderBookTimingCaseTwo21 = OrderBookTimingCaseTwoBase<21>;
using OrderBookTimingCaseTwo22 = OrderBookTimingCaseTwoBase<22>;
using OrderBookTimingCaseTwo23 = OrderBookTimingCaseTwoBase<23>;
using OrderBookTimingCaseTwo24 = OrderBookTimingCaseTwoBase<24>;
using OrderBookTimingCaseTwo25 = OrderBookTimingCaseTwoBase<25>;
using OrderBookTimingCaseTwo26 = OrderBookTimingCaseTwoBase<26>;
using OrderBookTimingCaseTwo27 = OrderBookTimingCaseTwoBase<27>;
using OrderBookTimingCaseTwo28 = OrderBookTimingCaseTwoBase<28>;
using OrderBookTimingCaseTwo29 = OrderBookTimingCaseTwoBase<29>;
using OrderBookTimingCaseTwo30 = OrderBookTimingCaseTwoBase<30>;
using OrderBookTimingCaseTwo31 = OrderBookTimingCaseTwoBase<31>;
using OrderBookTimingCaseTwo32 = OrderBookTimingCaseTwoBase<32>;

// Define tests for each worker count
TEST_P(OrderBookTimingCaseTwo1, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo2, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo3, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo4, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo5, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo6, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo7, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo8, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo9, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo10, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo11, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo12, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo13, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo14, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo15, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo16, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo17, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo18, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo19, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo20, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo21, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo22, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo23, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo24, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo25, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo26, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo27, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo28, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo29, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo30, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo31, Run) { runTest("./data/caseTwoLocking.csv"); }
TEST_P(OrderBookTimingCaseTwo32, Run) { runTest("./data/caseTwoLocking.csv"); }

// Instantiate each test suite with 100 runs (0-99)
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo1, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo2, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo3, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo4, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo5, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo6, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo7, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo8, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo9, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo10, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo11, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo12, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo13, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo14, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo15, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo16, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo17, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo18, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo19, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo20, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo21, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo22, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo23, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo24, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo25, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo26, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo27, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo28, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo29, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo30, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo31, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseTwo32, ::testing::Range(0, 100));

// Run all tests
int main(int argc, char **argv) {
    // Open data file
    // This will override an existing file by the same name
    ofstream file("./data/caseTwoLockless.csv");

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        return 1;
    }

    // Write header
    file << "num_threads,runtime_microseconds,throughput_ops_sec" << endl;

    // Close file
    file.close();
    
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
