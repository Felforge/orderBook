#include <gtest/gtest.h>
#include <fstream>
#include <chrono>
#include "../../parallelOrderBook/parallelOrderBook.h"
using namespace std;

// Case Six uses realistic HFT market data with 100,000 orders
// Each order has varying price and size based on realistic stock price movements
// Uses Geometric Brownian Motion for price simulation

// Define external Order object
#define OrderExt Order<DEFAULT_RING_SIZE, PRICE_TABLE_BUCKETS>

// Structure to hold market data
struct MarketOrder {
    bool isBuy;      // true=buy, false=sell
    double price;    // Order price
    uint32_t size;   // Order size in shares
};

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

        // Load market data
        loadMarketData("market_data.txt");
    }

    // Template parameters: NumWorkers, MaxSymbols=1, MaxOrders=1e6
    OrderBook<NumWorkers, 1, 1000000> orderBook;
    uint16_t symbolID;

    // Vector to hold market orders
    vector<MarketOrder> marketOrders;

    // Load market data from file
    void loadMarketData(const string& filename) {
        ifstream file(filename);

        // Make sure file opens
        if (!file.is_open()) {
            cerr << "Failed to open market data file" << endl;
            return;
        }

        // Skip header lines (lines starting with #)
        string line;
        while (getline(file, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Parse the line: order_type price size
            istringstream iss(line);
            int orderType;
            double price;
            uint32_t size;

            if (iss >> orderType >> price >> size) {
                MarketOrder order;
                order.isBuy = (orderType == 1);
                order.price = price;
                order.size = size;
                marketOrders.push_back(order);
            }
        }

        // Close file
        file.close();

        cout << "Loaded " << marketOrders.size() << " market orders" << endl;
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

        // Number of orders
        int N = marketOrders.size();

        // Get start time
        auto start = chrono::high_resolution_clock::now();

        // Submit all orders with realistic prices and sizes
        for (int i = 0; i < N; i++) {
            const auto& order = marketOrders[i];
            if (order.isBuy) {
                orderBook.submitOrder(1, symbolID, Side::BUY, order.size, order.price);
            } else {
                orderBook.submitOrder(1, symbolID, Side::SELL, order.size, order.price);
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
