#include <gtest/gtest.h>
#include <fstream>
#include <chrono>
#include "../../lockingOrderBook/lockingOrderBook.h"
using namespace std;

// Case Four does 100,000 operations across 100 different price levels
// This is done in a fixed sequence going 1 to 100 and then looping again
// The 100 price levels is fairly arbitrary but works to show what we need
// This operation has a 70% chance to be a submission, and a 30% chance to be a cancel
// The decisions are pregenerated so that the results can be replicated

// Define external Order object
#define OrderExt Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>

// Base template for test fixtures with different worker counts
template<size_t NumWorkers>
class OrderBookTimingCaseFourBase : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        symbolID = orderBook.registerSymbol(symbolName);

        // Load decisions
        // This uses the same decisions as Case Three
        loadDecisions("../caseThree/decisions.txt");
    }

    // Template parameters: NumWorkers, MaxSymbols=1, MaxOrders=1e6
    OrderBook<NumWorkers, 1, 1000000> orderBook;
    uint16_t symbolID;

    // Vector to hold decisions
    vector<bool> decisions;

    // Load pre-generated decisions from file
    void loadDecisions(const string& filename) {
        ifstream file(filename);
        
        // Make sure file opens
        if (!file.is_open()) {
            cerr << "Failed to open decisions file" << endl;
            return;
        }
        
        // Pull decisions from file and convert to bool
        int decision;
        while (file >> decision) {
            decisions.push_back(decision == 1);
        }

        // Close file
        file.close();

        return;
    }

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

        // Print results to file
        file << NumWorkers << "," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        file.close();
    }
};

// Create specific test fixtures for each worker count (1-32)
using OrderBookTimingCaseFour1 = OrderBookTimingCaseFourBase<1>;
using OrderBookTimingCaseFour2 = OrderBookTimingCaseFourBase<2>;
using OrderBookTimingCaseFour3 = OrderBookTimingCaseFourBase<3>;
using OrderBookTimingCaseFour4 = OrderBookTimingCaseFourBase<4>;
using OrderBookTimingCaseFour5 = OrderBookTimingCaseFourBase<5>;
using OrderBookTimingCaseFour6 = OrderBookTimingCaseFourBase<6>;
using OrderBookTimingCaseFour7 = OrderBookTimingCaseFourBase<7>;
using OrderBookTimingCaseFour8 = OrderBookTimingCaseFourBase<8>;
using OrderBookTimingCaseFour9 = OrderBookTimingCaseFourBase<9>;
using OrderBookTimingCaseFour10 = OrderBookTimingCaseFourBase<10>;
using OrderBookTimingCaseFour11 = OrderBookTimingCaseFourBase<11>;
using OrderBookTimingCaseFour12 = OrderBookTimingCaseFourBase<12>;
using OrderBookTimingCaseFour13 = OrderBookTimingCaseFourBase<13>;
using OrderBookTimingCaseFour14 = OrderBookTimingCaseFourBase<14>;
using OrderBookTimingCaseFour15 = OrderBookTimingCaseFourBase<15>;
using OrderBookTimingCaseFour16 = OrderBookTimingCaseFourBase<16>;
using OrderBookTimingCaseFour17 = OrderBookTimingCaseFourBase<17>;
using OrderBookTimingCaseFour18 = OrderBookTimingCaseFourBase<18>;
using OrderBookTimingCaseFour19 = OrderBookTimingCaseFourBase<19>;
using OrderBookTimingCaseFour20 = OrderBookTimingCaseFourBase<20>;
using OrderBookTimingCaseFour21 = OrderBookTimingCaseFourBase<21>;
using OrderBookTimingCaseFour22 = OrderBookTimingCaseFourBase<22>;
using OrderBookTimingCaseFour23 = OrderBookTimingCaseFourBase<23>;
using OrderBookTimingCaseFour24 = OrderBookTimingCaseFourBase<24>;
using OrderBookTimingCaseFour25 = OrderBookTimingCaseFourBase<25>;
using OrderBookTimingCaseFour26 = OrderBookTimingCaseFourBase<26>;
using OrderBookTimingCaseFour27 = OrderBookTimingCaseFourBase<27>;
using OrderBookTimingCaseFour28 = OrderBookTimingCaseFourBase<28>;
using OrderBookTimingCaseFour29 = OrderBookTimingCaseFourBase<29>;
using OrderBookTimingCaseFour30 = OrderBookTimingCaseFourBase<30>;
using OrderBookTimingCaseFour31 = OrderBookTimingCaseFourBase<31>;
using OrderBookTimingCaseFour32 = OrderBookTimingCaseFourBase<32>;

// Define tests for each worker count
TEST_P(OrderBookTimingCaseFour1, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour2, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour3, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour4, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour5, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour6, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour7, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour8, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour9, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour10, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour11, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour12, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour13, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour14, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour15, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour16, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour17, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour18, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour19, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour20, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour21, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour22, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour23, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour24, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour25, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour26, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour27, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour28, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour29, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour30, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour31, Run) { runTest("../data/caseFourLocking.csv"); }
TEST_P(OrderBookTimingCaseFour32, Run) { runTest("../data/caseFourLocking.csv"); }

// Instantiate each test suite with 100 runs (0-99)
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour1, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour2, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour3, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour4, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour5, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour6, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour7, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour8, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour9, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour10, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour11, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour12, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour13, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour14, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour15, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour16, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour17, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour18, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour19, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour20, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour21, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour22, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour23, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour24, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour25, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour26, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour27, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour28, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour29, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour30, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour31, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFour32, ::testing::Range(0, 100));

// Run all tests
int main(int argc, char **argv) {
    // Open data file
    // This will override an existing file by the same name
    ofstream file("../data/caseFourLocking.csv");

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
