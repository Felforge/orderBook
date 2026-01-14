#include <gtest/gtest.h>
#include <fstream>
#include <chrono>
#include "../../lockingOrderBook/lockingOrderBook.h"
using namespace std;

// Case Three does 100,000 operations on the same price level
// This operation has a 70% chance to be a submission, and a 30% chance to be a cancel
// The decisions are pregenerated so that the results can be replicated

// Define external Order object
#define OrderExt Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>

// Base template for test fixtures with different worker counts
template<size_t NumWorkers>
class OrderBookTimingCaseThreeBase : public ::testing::TestWithParam<int> {
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

        // Create vector to track orders
        // Reserve space to lower overhead
        vector<OrderExt*> orders;
        orders.reserve(N);

        // Submit and track 10,000 initial orders on our price level
        for (int i = 0; i < 10000; i++) {
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

        // Print results to file
        file << NumWorkers << "," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        file.close();
    }
};

// Create specific test fixtures for each worker count (1-32)
using OrderBookTimingCaseThree1 = OrderBookTimingCaseThreeBase<1>;
using OrderBookTimingCaseThree2 = OrderBookTimingCaseThreeBase<2>;
using OrderBookTimingCaseThree3 = OrderBookTimingCaseThreeBase<3>;
using OrderBookTimingCaseThree4 = OrderBookTimingCaseThreeBase<4>;
using OrderBookTimingCaseThree5 = OrderBookTimingCaseThreeBase<5>;
using OrderBookTimingCaseThree6 = OrderBookTimingCaseThreeBase<6>;
using OrderBookTimingCaseThree7 = OrderBookTimingCaseThreeBase<7>;
using OrderBookTimingCaseThree8 = OrderBookTimingCaseThreeBase<8>;
using OrderBookTimingCaseThree9 = OrderBookTimingCaseThreeBase<9>;
using OrderBookTimingCaseThree10 = OrderBookTimingCaseThreeBase<10>;
using OrderBookTimingCaseThree11 = OrderBookTimingCaseThreeBase<11>;
using OrderBookTimingCaseThree12 = OrderBookTimingCaseThreeBase<12>;
using OrderBookTimingCaseThree13 = OrderBookTimingCaseThreeBase<13>;
using OrderBookTimingCaseThree14 = OrderBookTimingCaseThreeBase<14>;
using OrderBookTimingCaseThree15 = OrderBookTimingCaseThreeBase<15>;
using OrderBookTimingCaseThree16 = OrderBookTimingCaseThreeBase<16>;
using OrderBookTimingCaseThree17 = OrderBookTimingCaseThreeBase<17>;
using OrderBookTimingCaseThree18 = OrderBookTimingCaseThreeBase<18>;
using OrderBookTimingCaseThree19 = OrderBookTimingCaseThreeBase<19>;
using OrderBookTimingCaseThree20 = OrderBookTimingCaseThreeBase<20>;
using OrderBookTimingCaseThree21 = OrderBookTimingCaseThreeBase<21>;
using OrderBookTimingCaseThree22 = OrderBookTimingCaseThreeBase<22>;
using OrderBookTimingCaseThree23 = OrderBookTimingCaseThreeBase<23>;
using OrderBookTimingCaseThree24 = OrderBookTimingCaseThreeBase<24>;
using OrderBookTimingCaseThree25 = OrderBookTimingCaseThreeBase<25>;
using OrderBookTimingCaseThree26 = OrderBookTimingCaseThreeBase<26>;
using OrderBookTimingCaseThree27 = OrderBookTimingCaseThreeBase<27>;
using OrderBookTimingCaseThree28 = OrderBookTimingCaseThreeBase<28>;
using OrderBookTimingCaseThree29 = OrderBookTimingCaseThreeBase<29>;
using OrderBookTimingCaseThree30 = OrderBookTimingCaseThreeBase<30>;
using OrderBookTimingCaseThree31 = OrderBookTimingCaseThreeBase<31>;
using OrderBookTimingCaseThree32 = OrderBookTimingCaseThreeBase<32>;

// Define tests for each worker count
TEST_P(OrderBookTimingCaseThree1, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree2, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree3, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree4, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree5, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree6, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree7, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree8, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree9, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree10, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree11, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree12, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree13, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree14, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree15, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree16, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree17, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree18, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree19, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree20, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree21, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree22, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree23, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree24, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree25, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree26, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree27, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree28, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree29, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree30, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree31, Run) { runTest("../data/caseThreeLocking.csv"); }
TEST_P(OrderBookTimingCaseThree32, Run) { runTest("../data/caseThreeLocking.csv"); }

// Instantiate each test suite with 100 runs (0-99)
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree1, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree2, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree3, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree4, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree5, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree6, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree7, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree8, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree9, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree10, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree11, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree12, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree13, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree14, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree15, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree16, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree17, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree18, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree19, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree20, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree21, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree22, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree23, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree24, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree25, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree26, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree27, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree28, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree29, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree30, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree31, ::testing::Range(0, 100));
INSTANTIATE_TEST_SUITE_P(RepeatedRuns, OrderBookTimingCaseThree32, ::testing::Range(0, 100));

// Run all tests
int main(int argc, char **argv) {
    // Open data file
    // This will override an existing file by the same name
    ofstream file("../data/caseThreeLocking.csv");

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
