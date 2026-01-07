#include <gtest/gtest.h>
#include <fstream>
#include <chrono>
#include "../../parallelOrderBook/parallelOrderBook.h"
using namespace std;

// Time 1,000,000 order submissions on the same price level with one thread
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesOneThread) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=1, MaxSymbols=1, MaxOrders=1e6
        OrderBook<1, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "1," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 1,000,000 order submissions on the same price level with two threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwoThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=2, MaxSymbols=1, MaxOrders=1e6
        OrderBook<2, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "2," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with three threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesThreeThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=3, MaxSymbols=1, MaxOrders=1e6
        OrderBook<3, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "3," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with four threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesFourThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=4, MaxSymbols=1, MaxOrders=1e6
        OrderBook<4, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "4," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with five threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesFiveThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=5, MaxSymbols=1, MaxOrders=1e6
        OrderBook<5, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "5," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with six threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesSixThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=6, MaxSymbols=1, MaxOrders=1e6
        OrderBook<6, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "6," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with seven threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesSevenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=7, MaxSymbols=1, MaxOrders=1e6
        OrderBook<7, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "7," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with eight threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesEightThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=8, MaxSymbols=1, MaxOrders=1e6
        OrderBook<8, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "8," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with nine threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesNineThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=9, MaxSymbols=1, MaxOrders=1e6
        OrderBook<9, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "9," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with ten threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=10, MaxSymbols=1, MaxOrders=1e6
        OrderBook<10, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "10," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with eleven threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesElevenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=11, MaxSymbols=1, MaxOrders=1e6
        OrderBook<11, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "11," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twelve threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwelveThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=12, MaxSymbols=1, MaxOrders=1e6
        OrderBook<12, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "12," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with thirteen threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesThirteenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=13, MaxSymbols=1, MaxOrders=1e6
        OrderBook<13, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "13," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with fourteen threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesFourteenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=14, MaxSymbols=1, MaxOrders=1e6
        OrderBook<14, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "14," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with fifteen threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesFifteenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=15, MaxSymbols=1, MaxOrders=1e6
        OrderBook<15, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "15," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with sixteen threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesSixteenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=16, MaxSymbols=1, MaxOrders=1e6
        OrderBook<16, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "16," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with seventeen threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesSeventeenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=17, MaxSymbols=1, MaxOrders=1e6
        OrderBook<17, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "17," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with eighteen threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesEighteenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=18, MaxSymbols=1, MaxOrders=1e6
        OrderBook<18, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "18," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with nineteen threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesNineteenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=19, MaxSymbols=1, MaxOrders=1e6
        OrderBook<19, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "19," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twenty threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwentyThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=20, MaxSymbols=1, MaxOrders=1e6
        OrderBook<20, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "20," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twenty-one threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwentyOneThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=21, MaxSymbols=1, MaxOrders=1e6
        OrderBook<21, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "21," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twenty-two threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwentyTwoThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=22, MaxSymbols=1, MaxOrders=1e6
        OrderBook<22, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "22," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twenty-three threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwentyThreeThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=23, MaxSymbols=1, MaxOrders=1e6
        OrderBook<23, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "23," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twenty-four threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwentyFourThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=24, MaxSymbols=1, MaxOrders=1e6
        OrderBook<24, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "24," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twenty-five threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwentyFiveThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=25, MaxSymbols=1, MaxOrders=1e6
        OrderBook<25, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "25," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twenty-six threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwentySixThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=26, MaxSymbols=1, MaxOrders=1e6
        OrderBook<26, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "26," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twenty-seven threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwentySevenThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=27, MaxSymbols=1, MaxOrders=1e6
        OrderBook<27, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "27," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twenty-eight threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwentyEightThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=28, MaxSymbols=1, MaxOrders=1e6
        OrderBook<28, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "28," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with twenty-nine threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesTwentyNineThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=29, MaxSymbols=1, MaxOrders=1e6
        OrderBook<29, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "29," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with thirty threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesThirtyThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=30, MaxSymbols=1, MaxOrders=1e6
        OrderBook<30, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "30," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with thirty-one threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesThirtyOneThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=31, MaxSymbols=1, MaxOrders=1e6
        OrderBook<31, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "31," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Time 100,000 order submissions on the same price level with thirty-two threads
// Run it 1000 times to deal with anomalies
TEST(OrderBookTimingCaseOne, HandlesThirtyTwoThreads) {
    // Open file in append mode to add multiple runs
    ofstream file("../data/caseOneLockless.csv", ios::app);

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        FAIL();
        return;
    }

    // Number of times to run the test
    int numRuns = 1e3;

    for (int i = 0; i < numRuns; i++) {
        // Create order book
        // Template parameters: NumWorkers=32, MaxSymbols=1, MaxOrders=1e6
        OrderBook<32, 1, 1000000> orderBook;

        // Start order book
        orderBook.start();

        // Register symbol
        string symbolName = "AAPL";
        uint16_t symbolID = orderBook.registerSymbol(symbolName);

        // Number of orders;
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
        file << "32," << totalTime << "," << (N * 1e6 / totalTime) << endl;

        // Order book instance will be killed on loop end
    }

    // Close file
    file.close();
}

// Create file header and run all tests
int main(int argc, char **argv) {
    // Open data file
    // This will override an existing file by the same name
    ofstream file("../data/caseOneLockless.csv");

    // Check if file is open
    if (!file.is_open()) {
        cerr << "Failed to open CSV file" << endl;
        return;
    }

    // Write header
    file << "num_threads,runtime_microseconds,throughput_ops_sec" << endl;

    // Close file
    file.close();

    // Run tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}