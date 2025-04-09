#include <iostream>
#include <chrono>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include "naiveOrderBook.h"

// For timing purposes
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

double getRandomPrice() {
    double randNum = 0.0;
    while (randNum == 0.0) {
        randNum = rand() % 10001;
    }
    return randNum / 100;
}

int getRandomQuantity() {
    int randNum = 0;
    while (randNum == 0) {
        randNum = rand() % 101;
    }
    return randNum;
}

std::string getRandomOrderType() {
    int randNum = rand() % 2;
    if (randNum == 0) {
        return "BUY";
    } else {
        return "SELL";
    }
}

std::string formatTable(int t1, int t2, int t3) {
    int totalT = t1 / 1e3; // Total for 10 orders
    int totalK = t2 / 1e3; // Total for 1000 orders
    int totalHK = t3 / 1e3; // Total for 100000 orders
    double latencyT = totalT / 1e1; // Latency for 10
    double latencyK = totalK / 1e3; // Latency for 1000
    double latencyHK = totalHK / 1e5; // Latency for 100000
    int throughputT = 1 / (latencyT / 1e6); // Throughput for 10
    int throughputK = 1 / (latencyK / 1e6); // Throughput for 1000
    int throughputHK = 1 / (latencyHK / 1e6); // Throughput for 100000
    
    // Put all data onto string to be put into md file later
    std::string optStr = "| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |\n";
    optStr += "| :-----------: |  :-----------: |  :-----------: |  :-----------: |\n";
    optStr += "| 10 | " + std::to_string(totalT) + " | " + std::to_string(latencyT) + " | " + std::to_string(throughputT) + " |\n";
    optStr += "| 1000 | " + std::to_string(totalK) + " | " + std::to_string(latencyK) + " | " + std::to_string(throughputK) + " |\n";
    optStr += "| 100000 | " + std::to_string(totalHK) + " | " + std::to_string(latencyHK) + " | " + std::to_string(throughputHK) + " |\n\n";
    
    // Return final string
    return optStr;
}

std::string createAddOrderTimeTable() {
    OrderBook orderBook = OrderBook();
    std::string optStr;

    auto t1 = high_resolution_clock::now(); // initial time

    // Up to 10 orders
    for (int i = 0; i < 10; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
    }
    auto t2 = high_resolution_clock::now(); // time after 10

    // Up to 1000 orders
    for (int i = 0; i < 990; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
    }
    auto t3 = high_resolution_clock::now(); // time after 1000

    // Up to 100000 orders
    for (int i = 0; i < 99000; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
    }
    auto t4 = high_resolution_clock::now(); // time after 100000

    // Return formatted string
    std::string opt = formatTable((t2 - t1).count(), (t3 - t1).count(), (t4 - t1).count());
    opt += "Clearly the naive linked-list implementation becomes extremely inefficient at adding orders as the order count increases\n\n";
    return formatTable((t2 - t1).count(), (t3 - t1).count(), (t4 - t1).count());
}

std::string createRemoveHeadTimeTable() {
    OrderBook orderBook = OrderBook();
    std::string optStr;
    int orderCount = 0;

    // Create order lists
    // Everything will be put in buy to make benchmarking more straightforward
    for (int i = 0; i < 100000; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
    }

    auto t1 = high_resolution_clock::now(); // initial time

    // Up to 10 orders
    for (int i = 0; i < 10; i++) {
        orderBook.removeOrder(orderCount, "BUY", false);
        orderCount += 1;
    }
    auto t2 = high_resolution_clock::now(); // time after 10

    // Up to 1000 orders
    for (int i = 0; i < 990; i++) {
        orderBook.removeOrder(orderCount, "BUY", false);
        orderCount += 1;
    }
    auto t3 = high_resolution_clock::now(); // time after 1000

    // Up to 100000 orders
    for (int i = 0; i < 99000; i++) {
        orderBook.removeOrder(orderCount, "BUY", false);
        orderCount += 1;
    }
    auto t4 = high_resolution_clock::now(); // time after 100000

    // Return formatted string
    std::string opt = formatTable((t2 - t1).count(), (t3 - t1).count(), (t4 - t1).count());
    opt += "Latency and throughput are fairly consistent when removing from the head\n\n";
    return opt;
}

std::string createRemoveTailTimeTable() {
    OrderBook orderBook = OrderBook();
    std::string optStr;
    int orderCount = 100000;

    // Create order lists
    // Everything will be put in buy to make benchmarking more straightforward
    for (int i = 0; i < 100000; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
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
    std::string opt = formatTable((t2 - t1).count(), (t3 - t1).count(), (t4 - t1).count());
    std::cout << opt;
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
        // outputFile << "## Adding Order Efficiency Table\n\n";
        // outputFile << createAddOrderTimeTable();
        // outputFile << "## Reamoving Head Order Efficiency Table\n\n";
        // outputFile << createRemoveHeadTimeTable();
        outputFile << "## Reamoving Tail Order Efficiency Table\n\n";
        outputFile << createRemoveTailTimeTable();

        // Close the file
        outputFile.close();
    } else {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    return 0;
}