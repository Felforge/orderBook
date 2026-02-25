#include <iostream>
#include <chrono>
#include <string>
#include <fstream>
#include "naiveOrderBook.h"

// for timing purposes
using std::chrono::high_resolution_clock;

std::string formatTable(std::chrono::high_resolution_clock::time_point t1, std::chrono::high_resolution_clock::time_point t2, std::chrono::high_resolution_clock::time_point t3, std::chrono::high_resolution_clock::time_point t4, std::chrono::high_resolution_clock::time_point t5, std::chrono::high_resolution_clock::time_point t6) {

    int64_t durationT = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();  // total time for 10 orders
    int64_t durationH = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t1).count();  // total time for 10 orders
    int64_t durationK = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t1).count();  // total time for 1000 orders
    int64_t durationTK = std::chrono::duration_cast<std::chrono::microseconds>(t5 - t1).count(); // total time for 10000 orders
    int64_t durationHK = std::chrono::duration_cast<std::chrono::microseconds>(t6 - t1).count(); // total time for 100000 orders

    double latencyT = static_cast<double>(durationT) / 1e1;       // Latency for 10 orders
    double latencyH = static_cast<double>(durationH) / 1e2;       // Latency for 100 orders
    double latencyK = static_cast<double>(durationK) / 1e3;       // Latency for 1000 orders
    double latencyTK = static_cast<double>(durationTK) / 1e4;     // Latency for 10000 orders
    double latencyHK = static_cast<double>(durationHK) / 1e5;     // Latency for 100000 orders

    double throughputT = 1e6 / latencyT;    // Throughput for 10 orders
    double throughputH = 1e6 / latencyH;    // Throughput for 100 orders
    double throughputK = 1e6 / latencyK;    // Throughput for 1000 orders
    double throughputTK = 1e6 / latencyTK;  // Throughput for 10000 orders
    double throughputHK = 1e6 / latencyHK;  // Throughput for 100000 orders
    
    // Put all data onto string to be put into md file later
    std::string optStr = "| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |\n";
    optStr += "| :-----------: |  :-----------: |  :-----------: |  :-----------: |\n";
    optStr += "| 10 | " + std::to_string(durationT) + " | " + std::to_string(latencyT) + " | " + std::to_string(throughputT) + " |\n";
    optStr += "| 100 | " + std::to_string(durationH) + " | " + std::to_string(latencyH) + " | " + std::to_string(throughputH) + " |\n";
    optStr += "| 1000 | " + std::to_string(durationK) + " | " + std::to_string(latencyK) + " | " + std::to_string(throughputK) + " |\n";
    optStr += "| 10000 | " + std::to_string(durationTK) + " | " + std::to_string(latencyTK) + " | " + std::to_string(throughputTK) + " |\n";
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
        orderBook.addOrder(100.0, 50, "BUY", false);
    }
    auto t2 = high_resolution_clock::now(); // time after 10

    // Up to 100 orders
    for (int i = 0; i < 90; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
    }
    auto t3 = high_resolution_clock::now(); // time after 100

    // Up to 1000 orders
    for (int i = 0; i < 900; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
    }
    auto t4 = high_resolution_clock::now(); // time after 1000

    // Up to 10000 orders
    for (int i = 0; i < 9000; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
    }
    auto t5 = high_resolution_clock::now(); // time after 10000

    // Up to 100000 orders
    for (int i = 0; i < 90000; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
    }
    auto t6 = high_resolution_clock::now(); // time after 100000

    // Return formatted string
    std::string opt = formatTable(t1, t2, t3, t4, t5, t6);
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

    // Up to 100 orders
    for (int i = 10; i < 100; i++) {
        orderBook.removeOrder(i, "BUY", false);
    }
    auto t3 = high_resolution_clock::now(); // time after 100

    // Up to 1000 orders
    for (int i = 100; i < 1000; i++) {
        orderBook.removeOrder(i, "BUY", false);
    }
    auto t4 = high_resolution_clock::now(); // time after 1000

    // Up to 10000 orders
    for (int i = 1000; i < 10000; i++) {
        orderBook.removeOrder(i, "BUY", false);
    }
    auto t5 = high_resolution_clock::now(); // time after 10000

    // Up to 100000 orders
    for (int i = 10000; i < 100000; i++) {
        orderBook.removeOrder(i, "BUY", false);
    }
    auto t6 = high_resolution_clock::now(); // time after 100000

    // Return formatted string
    std::string opt = formatTable(t1, t2, t3, t4, t5, t6);
    opt += "Latency and throughput are fairly consistent when removing from the head\n\n";
    std::cout << opt;
    return opt;
}

std::string createRemoveTailTimeTable() {
    OrderBook orderBook = OrderBook();
    std::string optStr;
    int orderCount = 99999;

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

    // Up to 100 orders
    for (int i = 0; i < 90; i++) {
        orderBook.removeOrder(orderCount, "BUY", false);
        orderCount -= 1;
    }
    auto t3 = high_resolution_clock::now(); // time after 100

    // Up to 1000 orders
    for (int i = 0; i < 900; i++) {
        orderBook.removeOrder(orderCount, "BUY", false);
        orderCount -= 1;
    }
    auto t4 = high_resolution_clock::now(); // time after 1000

    // Up to 10000 orders
    for (int i = 0; i < 9000; i++) {
        orderBook.removeOrder(orderCount, "BUY", false);
        orderCount -= 1;
    }
    auto t5 = high_resolution_clock::now(); // time after 10000

    // Up to 100000 orders
    for (int i = 0; i < 90000; i++) {
        orderBook.removeOrder(orderCount, "BUY", false);
        orderCount -= 1;
    }
    auto t6 = high_resolution_clock::now(); // time after 100000

    // Return formatted string
    std::string opt = formatTable(t1, t2, t3, t4, t5, t6);
    opt += "Latency gets continuously lower as the linked-list shortens so the average for 100,000 is much lower\n\n";
    std::cout << opt;
    return opt;
}

std::string createOrderMatchEfficiencyTable() {
    OrderBook orderBook = OrderBook();
    std::string optStr;

    // Create order lists
    // Everything will be put in buy to make benchmarking more straightforward
    for (int i = 0; i < 100000; i++) {
        orderBook.addOrder(100.0, 50, "BUY", false);
        orderBook.addOrder(100.0, 50, "SELL", false);
    }

    auto t1 = high_resolution_clock::now(); // initial time

    // Up to 10 orders
    orderBook.matchOrders(false, 10);
    auto t2 = high_resolution_clock::now(); // time after 10

    // Up to 100 orders
    orderBook.matchOrders(false, 90);
    auto t3 = high_resolution_clock::now(); // time after 100

    // Up to 1000 orders
    orderBook.matchOrders(false, 900);
    auto t4 = high_resolution_clock::now(); // time after 1000

    // Up to 10000 orders
    orderBook.matchOrders(false, 9000);
    auto t5 = high_resolution_clock::now(); // time after 10000

    // Up to 100000 orders
    orderBook.matchOrders(false, 90000);
    auto t6 = high_resolution_clock::now(); // time after 100000

    // Return formatted string
    std::string opt = formatTable(t1, t2, t3, t4, t5, t6);
    opt += "Latency and throughput are fairly consistent despite the number of orders as the top orders are easily accessible for matching";
    std::cout << opt;
    return opt;
}

int main() {
    // Create an ofstream object
    std::ofstream outputFile;

    // Open the file and clear content
    outputFile.open("naiveEfficiencyTable.md", std::ofstream::out | std::ofstream::trunc);

    // Check if the file was opened successfully
    if (outputFile.is_open()) {
        // Write data to the file
        outputFile << "# Naive Linked List Efficiency Data\n\n";
        outputFile << "## Adding Order Efficiency Table\n\n" << createAddOrderTimeTable();
        outputFile << "## Reamoving Head Order Efficiency Table\n\n" << createRemoveHeadTimeTable();
        outputFile << "## Reamoving Tail Order Efficiency Table\n\n" << createRemoveTailTimeTable();
        outputFile << "## Order Match Efficiency Table\n\n" << createOrderMatchEfficiencyTable() << std::endl;

        // Close the file
        outputFile.close();
    } else {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    return 0;
}