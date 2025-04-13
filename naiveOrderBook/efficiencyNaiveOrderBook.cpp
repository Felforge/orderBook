#include <iostream>
#include <chrono>
#include <string>
#include <fstream>
#include "naiveOrderBook.h"

// for timing purposes
using std::chrono::high_resolution_clock;

std::string formatTable(std::chrono::high_resolution_clock::time_point t1, std::chrono::high_resolution_clock::time_point t2, std::chrono::high_resolution_clock::time_point t3, std::chrono::high_resolution_clock::time_point t4) {

    int64_t durationT = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count(); // total time for 10 orders
    int64_t durationK = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count(); // total time for 1000 orders
    int64_t durationHK = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count(); // total time for 100000 orders

    double latencyT = static_cast<double>(durationT) / 1e1;       // Latency for 10 orders
    double latencyK = static_cast<double>(durationK) / 1e4;     // Latency for 1000 orders
    double latencyHK = static_cast<double>(durationHK) / 1e6;  // Latency for 100000 orders

    double throughputT = 1e6 / latencyT;  // Throughput for 10 orders
    double throughputK = 1e6 / latencyK;  // Throughput for 1000 orders
    double throughputHK = 1e6 / latencyHK;  // Throughput for 100000 orders
    
    // Put all data onto string to be put into md file later
    std::string optStr = "| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |\n";
    optStr += "| :-----------: |  :-----------: |  :-----------: |  :-----------: |\n";
    optStr += "| 10 | " + std::to_string(durationT) + " | " + std::to_string(latencyT) + " | " + std::to_string(throughputT) + " |\n";
    optStr += "| 1000 | " + std::to_string(durationK) + " | " + std::to_string(latencyK) + " | " + std::to_string(throughputK) + " |\n";
    optStr += "| 100000 | " + std::to_string(durationHK) + " | " + std::to_string(latencyHK) + " | " + std::to_string(throughputHK) + " |\n\n";
    
    // Return final string
    return optStr;
}

std::string createAddOrderTimeTable() {
    OrderBook orderBook = OrderBook();
    std::string optStr;

    auto t1 = high_resolution_clock::now(); // initial time

    // Up to 10 orders
    for (int i = 0; i < 10; i++) {
        orderBook.addOrder(100.0, 50, "BUY");
    }
    auto t2 = high_resolution_clock::now(); // time after 10

    // Up to 1000 orders
    for (int i = 0; i < 990; i++) {
        orderBook.addOrder(100.0, 50, "BUY");
    }
    auto t3 = high_resolution_clock::now(); // time after 1000

    // Up to 100000 orders
    for (int i = 0; i < 99000; i++) {
        orderBook.addOrder(100.0, 50, "BUY");
    }
    auto t4 = high_resolution_clock::now(); // time after 100000

    // Return formatted string
    std::string opt = formatTable(t1, t2, t3, t4);
    opt += "Clearly the naive linked-list implementation becomes extremely inefficient at adding orders as the order count increases\n\n";
    std::cout << opt;
    return opt;
}

std::string createRemoveHeadTimeTable() {
    OrderBook orderBook = OrderBook();
    std::string optStr;

    // Create order lists
    // Everything will be put in buy to make benchmarking more straightforward
    for (int i = 0; i < 100000; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
    }

    auto t1 = high_resolution_clock::now(); // initial time

    // Up to 10 orders
    for (int i = 0; i < 10; i++) {
        orderBook.removeOrder(i, "BUY", false);
    }
    auto t2 = high_resolution_clock::now(); // time after 10

    // Up to 1000 orders
    for (int i = 10; i < 1000; i++) {
        orderBook.removeOrder(i, "BUY", false);
    }
    auto t3 = high_resolution_clock::now(); // time after 1000

    // Up to 100000 orders
    for (int i = 1000; i < 100000; i++) {
        orderBook.removeOrder(i, "BUY", false);
    }
    auto t4 = high_resolution_clock::now(); // time after 100000

    // Return formatted string
    std::string opt = formatTable(t1, t2, t3, t4);
    opt += "Latency and throughput are fairly consistent when removing from the head\n\n";
    return opt;
}

std::string createRemoveTailTimeTable() {
    OrderBook orderBook = OrderBook();
    std::string optStr;
    int orderCount = 99999;

    // Create order lists
    // Everything will be put in buy to make benchmarking more straightforward
    for (int i = 0; i < 100000; i++) {
        orderBook.addOrder(100.0, 50, "BUY");
    }

    auto t1 = high_resolution_clock::now(); // initial time

    // Up to 10 orders
    for (int i = 0; i < 10; i++) {
        orderBook.removeOrder(orderCount, "BUY", false);
        orderCount -= 1;
    }
    auto t2 = high_resolution_clock::now(); // time after 10

    // Up to 1000 orders
    for (int i = 0; i < 990; i++) {
        orderBook.removeOrder(orderCount, "BUY", false);
        orderCount -= 1;
    }
    auto t3 = high_resolution_clock::now(); // time after 1000

    // Up to 100000 orders
    for (int i = 0; i < 99000; i++) {
        orderBook.removeOrder(orderCount, "BUY", false);
        orderCount -= 1;
    }
    auto t4 = high_resolution_clock::now(); // time after 100000

    // Return formatted string
    std::string opt = formatTable(t1, t2, t3, t4);
    opt += "Latency gets continuously lower as the linked-list shortens so the average for 100,000 is much lower";
    return opt;
}

// Eliminate randomness to refine data?

int main() {
    // Create an ofstream object
    std::ofstream outputFile;

    // Open the file and clear content
    outputFile.open("naiveEfficiencyTable.md", std::ofstream::out | std::ofstream::trunc);

    // Check if the file was opened successfully
    if (outputFile.is_open()) {
        // Write data to the file
        outputFile << "# Naive Linked List Efficiency Data\n\n";
        createAddOrderTimeTable();
        // outputFile << "## Adding Order Efficiency Table\n\n" << createAddOrderTimeTable();
        // outputFile << "## Reamoving Head Order Efficiency Table\n\n" << createRemoveHeadTimeTable();
        // outputFile << "## Reamoving Tail Order Efficiency Table\n\n" << createRemoveTailTimeTable();

        // Close the file
        outputFile.close();
    } else {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    return 0;
}