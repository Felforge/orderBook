#include <gtest/gtest.h>
#include <fstream>
#include <chrono>
#include "../../lockingOrderBook/lockingOrderBook.h"
using namespace std;

// Case Five does 100,000 submissions on the same price level
// This operation has a 60% chance to be a buy, and a 40% chance to be a sell
// The decisions are pregenerated so that the results can be replicated

// Define external Order object
#define OrderExt Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>

// Base template for test fixtures with different worker counts
template<size_t NumWorkers>
class OrderBookTimingCaseFiveBase : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        symbolID = orderBook.registerSymbol(symbolName);

        // Load decisions
        loadDecisions("decisions.txt");
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
    
        // Get start time
        auto start = chrono::high_resolution_clock::now();
    
        // Submit all orders
        for (int i = 0; i < N; i++) {
            if (decisions[i]) {
                orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0);
            } else {
                orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0);
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
using OrderBookTimingCaseFive1 = OrderBookTimingCaseFiveBase<1>;
using OrderBookTimingCaseFive2 = OrderBookTimingCaseFiveBase<2>;
using OrderBookTimingCaseFive3 = OrderBookTimingCaseFiveBase<3>;
using OrderBookTimingCaseFive4 = OrderBookTimingCaseFiveBase<4>;
using OrderBookTimingCaseFive5 = OrderBookTimingCaseFiveBase<5>;
using OrderBookTimingCaseFive6 = OrderBookTimingCaseFiveBase<6>;
using OrderBookTimingCaseFive7 = OrderBookTimingCaseFiveBase<7>;
using OrderBookTimingCaseFive8 = OrderBookTimingCaseFiveBase<8>;
using OrderBookTimingCaseFive9 = OrderBookTimingCaseFiveBase<9>;
using OrderBookTimingCaseFive10 = OrderBookTimingCaseFiveBase<10>;
using OrderBookTimingCaseFive11 = OrderBookTimingCaseFiveBase<11>;
using OrderBookTimingCaseFive12 = OrderBookTimingCaseFiveBase<12>;
using OrderBookTimingCaseFive13 = OrderBookTimingCaseFiveBase<13>;
using OrderBookTimingCaseFive14 = OrderBookTimingCaseFiveBase<14>;
using OrderBookTimingCaseFive15 = OrderBookTimingCaseFiveBase<15>;
using OrderBookTimingCaseFive16 = OrderBookTimingCaseFiveBase<16>;
using OrderBookTimingCaseFive17 = OrderBookTimingCaseFiveBase<17>;
using OrderBookTimingCaseFive18 = OrderBookTimingCaseFiveBase<18>;
using OrderBookTimingCaseFive19 = OrderBookTimingCaseFiveBase<19>;
using OrderBookTimingCaseFive20 = OrderBookTimingCaseFiveBase<20>;
using OrderBookTimingCaseFive21 = OrderBookTimingCaseFiveBase<21>;
using OrderBookTimingCaseFive22 = OrderBookTimingCaseFiveBase<22>;
using OrderBookTimingCaseFive23 = OrderBookTimingCaseFiveBase<23>;
using OrderBookTimingCaseFive24 = OrderBookTimingCaseFiveBase<24>;
using OrderBookTimingCaseFive25 = OrderBookTimingCaseFiveBase<25>;
using OrderBookTimingCaseFive26 = OrderBookTimingCaseFiveBase<26>;
using OrderBookTimingCaseFive27 = OrderBookTimingCaseFiveBase<27>;
using OrderBookTimingCaseFive28 = OrderBookTimingCaseFiveBase<28>;
using OrderBookTimingCaseFive29 = OrderBookTimingCaseFiveBase<29>;
using OrderBookTimingCaseFive30 = OrderBookTimingCaseFiveBase<30>;
using OrderBookTimingCaseFive31 = OrderBookTimingCaseFiveBase<31>;
using OrderBookTimingCaseFive32 = OrderBookTimingCaseFiveBase<32>;

// Define tests for each worker count
TEST_P(OrderBookTimingCaseFive1, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive2, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive3, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive4, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive5, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive6, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive7, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive8, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive9, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive10, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive11, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive12, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive13, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive14, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive15, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive16, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive17, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive18, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive19, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive20, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive21, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive22, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive23, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive24, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive25, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive26, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive27, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive28, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive29, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive30, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive31, Run) { runTest("../data/caseFiveLocking.csv"); }
TEST_P(OrderBookTimingCaseFive32, Run) { runTest("../data/caseFiveLocking.csv"); }

// Instantiate each test suite with 100 runs (0-99)
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive1, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive2, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive3, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive4, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive5, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive6, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive7, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive8, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive9, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive10, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive11, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive12, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive13, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive14, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive15, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive16, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive17, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive18, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive19, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive20, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive21, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive22, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive23, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive24, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive25, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive26, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive27, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive28, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive29, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive30, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive31, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseFive32, ::testing::Range(0, 100));

// Run all tests
int main(int argc, char **argv) {
    // Open data file
    // This will override an existing file by the same name
    ofstream file("../data/caseFiveLocking.csv");

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
