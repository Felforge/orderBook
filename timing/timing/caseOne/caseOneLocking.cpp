#include <gtest/gtest.h>
#include <fstream>
#include <chrono>
#include "../../../lockingOrderBook/lockingOrderBook.h"
using namespace std;

// Case One distributes 100,000 orders on the same price level

// Base template for test fixtures with different worker counts
template<size_t NumWorkers>
class OrderBookTimingCaseOneBase : public ::testing::TestWithParam<int> {
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

        // Number of orders
        int N = 1e5;

        // Get start time
        auto start = chrono::high_resolution_clock::now();

        // Submit all orders
        for (int j = 0; j < N; j++) {
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

        // Print results to file
        file << NumWorkers << "," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        file.close();
    }
};

// Create specific test fixtures for each worker count (1-32)
using OrderBookTimingCaseOne1 = OrderBookTimingCaseOneBase<1>;
using OrderBookTimingCaseOne2 = OrderBookTimingCaseOneBase<2>;
using OrderBookTimingCaseOne3 = OrderBookTimingCaseOneBase<3>;
using OrderBookTimingCaseOne4 = OrderBookTimingCaseOneBase<4>;
using OrderBookTimingCaseOne5 = OrderBookTimingCaseOneBase<5>;
using OrderBookTimingCaseOne6 = OrderBookTimingCaseOneBase<6>;
using OrderBookTimingCaseOne7 = OrderBookTimingCaseOneBase<7>;
using OrderBookTimingCaseOne8 = OrderBookTimingCaseOneBase<8>;
using OrderBookTimingCaseOne9 = OrderBookTimingCaseOneBase<9>;
using OrderBookTimingCaseOne10 = OrderBookTimingCaseOneBase<10>;
using OrderBookTimingCaseOne11 = OrderBookTimingCaseOneBase<11>;
using OrderBookTimingCaseOne12 = OrderBookTimingCaseOneBase<12>;
using OrderBookTimingCaseOne13 = OrderBookTimingCaseOneBase<13>;
using OrderBookTimingCaseOne14 = OrderBookTimingCaseOneBase<14>;
using OrderBookTimingCaseOne15 = OrderBookTimingCaseOneBase<15>;
using OrderBookTimingCaseOne16 = OrderBookTimingCaseOneBase<16>;
using OrderBookTimingCaseOne17 = OrderBookTimingCaseOneBase<17>;
using OrderBookTimingCaseOne18 = OrderBookTimingCaseOneBase<18>;
using OrderBookTimingCaseOne19 = OrderBookTimingCaseOneBase<19>;
using OrderBookTimingCaseOne20 = OrderBookTimingCaseOneBase<20>;
using OrderBookTimingCaseOne21 = OrderBookTimingCaseOneBase<21>;
using OrderBookTimingCaseOne22 = OrderBookTimingCaseOneBase<22>;
using OrderBookTimingCaseOne23 = OrderBookTimingCaseOneBase<23>;
using OrderBookTimingCaseOne24 = OrderBookTimingCaseOneBase<24>;
using OrderBookTimingCaseOne25 = OrderBookTimingCaseOneBase<25>;
using OrderBookTimingCaseOne26 = OrderBookTimingCaseOneBase<26>;
using OrderBookTimingCaseOne27 = OrderBookTimingCaseOneBase<27>;
using OrderBookTimingCaseOne28 = OrderBookTimingCaseOneBase<28>;
using OrderBookTimingCaseOne29 = OrderBookTimingCaseOneBase<29>;
using OrderBookTimingCaseOne30 = OrderBookTimingCaseOneBase<30>;
using OrderBookTimingCaseOne31 = OrderBookTimingCaseOneBase<31>;
using OrderBookTimingCaseOne32 = OrderBookTimingCaseOneBase<32>;

// Define tests for each worker count
TEST_P(OrderBookTimingCaseOne1, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne2, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne3, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne4, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne5, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne6, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne7, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne8, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne9, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne10, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne11, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne12, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne13, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne14, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne15, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne16, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne17, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne18, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne19, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne20, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne21, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne22, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne23, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne24, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne25, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne26, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne27, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne28, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne29, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne30, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne31, Run) { runTest("./data/caseOneLocking.csv"); }
TEST_P(OrderBookTimingCaseOne32, Run) { runTest("./data/caseOneLocking.csv"); }

// Instantiate each test suite with 100 runs (0-99)
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne1, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne2, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne3, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne4, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne5, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne6, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne7, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne8, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne9, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne10, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne11, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne12, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne13, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne14, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne15, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne16, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne17, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne18, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne19, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne20, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne21, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne22, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne23, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne24, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne25, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne26, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne27, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne28, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne29, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne30, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne31, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseOne32, ::testing::Range(0, 100));

// Run all tests
int main(int argc, char **argv) {
    // Open data file
    // This will override an existing file by the same name
    ofstream file("./data/caseOneLockless.csv");

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
