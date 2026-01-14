#include <gtest/gtest.h>
#include <fstream>
#include <chrono>
#include "../../../parallelOrderBook/parallelOrderBook.h"
using namespace std;

// Case Six does 100,000 submissions on 100 different price levels
// This is done in a fixed sequence going 1 to 100 and then looping again
// The 100 price levels is fairly arbitrary but works to show what we need
// This operation has a 60% chance to be a buy, and a 40% chance to be a sell
// The decisions are pregenerated so that the results can be replicated
// Due to matching below this case ends up having fairly strange behavior

// Define external Order object
#define OrderExt Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>

// Base template for test fixtures with different worker counts
template<size_t NumWorkers>
class OrderBookTimingCaseSixBase : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        symbolID = orderBook.registerSymbol(symbolName);

        // Load decisions
        loadDecisions("../caseFive/decisions.txt");
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

        // Create price levels
        // N MUST be divisible by numLevels
        // This is also done to create 100 Buy orders per price level
        int numLevels = 100;
        for (int i = 0; i < numLevels; i++) {
            for (int j = 0; j < 100; j++) {
                orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0 + float(i));
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
                    orderBook.submitOrder(1, symbolID, Side::BUY, 100, 150.0 + float(j));
                } else {
                    orderBook.submitOrder(1, symbolID, Side::SELL, 100, 150.0 + float(j));
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
using OrderBookTimingCaseSix1 = OrderBookTimingCaseSixBase<1>;
using OrderBookTimingCaseSix2 = OrderBookTimingCaseSixBase<2>;
using OrderBookTimingCaseSix3 = OrderBookTimingCaseSixBase<3>;
using OrderBookTimingCaseSix4 = OrderBookTimingCaseSixBase<4>;
using OrderBookTimingCaseSix5 = OrderBookTimingCaseSixBase<5>;
using OrderBookTimingCaseSix6 = OrderBookTimingCaseSixBase<6>;
using OrderBookTimingCaseSix7 = OrderBookTimingCaseSixBase<7>;
using OrderBookTimingCaseSix8 = OrderBookTimingCaseSixBase<8>;
using OrderBookTimingCaseSix9 = OrderBookTimingCaseSixBase<9>;
using OrderBookTimingCaseSix10 = OrderBookTimingCaseSixBase<10>;
using OrderBookTimingCaseSix11 = OrderBookTimingCaseSixBase<11>;
using OrderBookTimingCaseSix12 = OrderBookTimingCaseSixBase<12>;
using OrderBookTimingCaseSix13 = OrderBookTimingCaseSixBase<13>;
using OrderBookTimingCaseSix14 = OrderBookTimingCaseSixBase<14>;
using OrderBookTimingCaseSix15 = OrderBookTimingCaseSixBase<15>;
using OrderBookTimingCaseSix16 = OrderBookTimingCaseSixBase<16>;
using OrderBookTimingCaseSix17 = OrderBookTimingCaseSixBase<17>;
using OrderBookTimingCaseSix18 = OrderBookTimingCaseSixBase<18>;
using OrderBookTimingCaseSix19 = OrderBookTimingCaseSixBase<19>;
using OrderBookTimingCaseSix20 = OrderBookTimingCaseSixBase<20>;
using OrderBookTimingCaseSix21 = OrderBookTimingCaseSixBase<21>;
using OrderBookTimingCaseSix22 = OrderBookTimingCaseSixBase<22>;
using OrderBookTimingCaseSix23 = OrderBookTimingCaseSixBase<23>;
using OrderBookTimingCaseSix24 = OrderBookTimingCaseSixBase<24>;
using OrderBookTimingCaseSix25 = OrderBookTimingCaseSixBase<25>;
using OrderBookTimingCaseSix26 = OrderBookTimingCaseSixBase<26>;
using OrderBookTimingCaseSix27 = OrderBookTimingCaseSixBase<27>;
using OrderBookTimingCaseSix28 = OrderBookTimingCaseSixBase<28>;
using OrderBookTimingCaseSix29 = OrderBookTimingCaseSixBase<29>;
using OrderBookTimingCaseSix30 = OrderBookTimingCaseSixBase<30>;
using OrderBookTimingCaseSix31 = OrderBookTimingCaseSixBase<31>;
using OrderBookTimingCaseSix32 = OrderBookTimingCaseSixBase<32>;

// Define tests for each worker count
TEST_P(OrderBookTimingCaseSix1, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix2, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix3, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix4, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix5, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix6, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix7, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix8, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix9, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix10, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix11, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix12, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix13, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix14, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix15, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix16, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix17, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix18, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix19, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix20, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix21, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix22, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix23, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix24, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix25, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix26, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix27, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix28, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix29, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix30, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix31, Run) { runTest("../data/caseSixLockless.csv"); }
TEST_P(OrderBookTimingCaseSix32, Run) { runTest("../data/caseSixLockless.csv"); }

// Instantiate each test suite with 100 runs (0-99)
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix1, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix2, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix3, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix4, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix5, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix6, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix7, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix8, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix9, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix10, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix11, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix12, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix13, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix14, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix15, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix16, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix17, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix18, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix19, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix20, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix21, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix22, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix23, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix24, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix25, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix26, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix27, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix28, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix29, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix30, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix31, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseSix32, ::testing::Range(0, 100));

// Run all tests
int main(int argc, char **argv) {
    // Open data file
    // This will override an existing file by the same name
    ofstream file("../data/caseSixLockless.csv");

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
