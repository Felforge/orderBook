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

std::string createAddOrderTimeTable() {
    OrderBook orderBook = OrderBook();
    std::string optStr;

    auto t1 = high_resolution_clock::now(); // initial time

    // Up to 10 orders
    for (int i = 0; i < 10; i++) {
        orderBook.addOrder(getRandomPrice(), getRandomQuantity(), getRandomOrderType(), false);
    }
    auto t2 = high_resolution_clock::now(); // time after 10

    // Up to 1000 orders
    for (int i = 0; i < 990; i++) {
        orderBook.addOrder(getRandomPrice(), getRandomQuantity(), getRandomOrderType(), false);
    }
    auto t3 = high_resolution_clock::now(); // time after 1000

    // Up to 100000 orders
    for (int i = 0; i < 99000; i++) {
        orderBook.addOrder(getRandomPrice(), getRandomQuantity(), getRandomOrderType(), false);
    }
    auto t4 = high_resolution_clock::now(); // time after 100000

    int totalT = (t2 - t1).count() / 1e3; // Total for 10 orders
    int totalK = (t3 - t1).count() / 1e3; // Total for 1000 orders
    int totalHK = (t4 - t1).count() / 1e3; // Total for 100000 orders

    optStr = "| **Num of Orders** | **Total Runtime (µs)** | **Time per Order (µs)** |\n";
    optStr += "| :-----------: |  :-----------: |  :-----------: |\n";
    optStr += "| 10 | " + std::to_string(totalT) + " | " + std::to_string(totalT / 1e1) + " |\n";
    optStr += "| 1000 | " + std::to_string(totalK) + " | " + std::to_string(totalK / 1e3) + " |\n";
    optStr += "| 100000 | " + std::to_string(totalHK) + " | " + std::to_string(totalHK / 1e5) + " |\n\n";
    
    // Return final string
    return optStr;
}

std::string createRemoveOrderTimeTable() {
    OrderBook orderBook = OrderBook();
    std::string optStr;
    int orderCount = 0;
    std::vector<int> buyVec;
    std::string orderType;
    int randomIndex;

    // Create order lists
    // Everything will be put in buy to make benchmarking more straightforward
    for (int i = 0; i < 100000; i++) {
        orderBook.addOrder(getRandomPrice(), getRandomQuantity(), "BUY", false);
        buyVec.push_back(orderCount);
        orderCount += 1;
    }

    auto t1 = high_resolution_clock::now(); // initial time

    // Up to 10 orders
    for (int i = 0; i < 10; i++) {
        randomIndex = std::rand() % buyVec.size();
        orderBook.removeOrder(buyVec[randomIndex], "BUY", false);
        buyVec.erase(buyVec.begin() + randomIndex);
    }
    auto t2 = high_resolution_clock::now(); // time after 10

    // Up to 1000 orders
    for (int i = 0; i < 990; i++) {
        randomIndex = std::rand() % buyVec.size();
        orderBook.removeOrder(buyVec[randomIndex], "BUY", false);
        buyVec.erase(buyVec.begin() + randomIndex);
    }
    auto t3 = high_resolution_clock::now(); // time after 1000

    // Up to 100000 orders
    for (int i = 0; i < 99000; i++) {
        randomIndex = std::rand() % buyVec.size();
        orderBook.removeOrder(buyVec[randomIndex], "BUY", false);
        buyVec.erase(buyVec.begin() + randomIndex);
    }
    auto t4 = high_resolution_clock::now(); // time after 100000

    int totalT = (t2 - t1).count() / 1e3; // Total for 10 orders
    int totalK = (t3 - t1).count() / 1e3; // Total for 1000 orders
    int totalHK = (t4 - t1).count() / 1e3; // Total for 100000 orders

    optStr = "| **Num of Orders** | **Total Runtime (µs)** | **Time per Second (µs)** |\n";
    optStr += "| :-----------: |  :-----------: |  :-----------: |\n";
    optStr += "| 10 | " + std::to_string(totalT) + " | " + std::to_string(totalT / 1e1) + " |\n";
    optStr += "| 1000 | " + std::to_string(totalK) + " | " + std::to_string(totalK / 1e3) + " |\n";
    optStr += "| 100000 | " + std::to_string(totalHK) + " | " + std::to_string(totalHK / 1e5) + " |\n\n";
    
    // Return final string
    return optStr;
}

// ^^ Remove from already made order lists for benchmark

int main() {
    // Create an ofstream object
    std::ofstream outputFile;

    // Open the file and clear content
    outputFile.open("efficiencyTable.md", std::ofstream::out | std::ofstream::trunc);

    // Check if the file was opened successfully
    if (outputFile.is_open()) {
        // Write data to the file
        outputFile << "# Naive Linked List Efficiency Data\n\n";
        outputFile << "## Order Adding Efficiency Table\n\n";
        outputFile << createAddOrderTimeTable();
        // outputFile << "## Order Removing Efficiency Table\n\n";
        // outputFile << createRemoveOrderTimeTable();

        // Close the file
        outputFile.close();
    } else {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    return 0;
}