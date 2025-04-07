#include <iostream>
#include <chrono>
#include <cstdlib>
#include <string>
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

void createOrderTimeTable() {
    OrderBook orderBook = OrderBook();
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

    std::cout << "Order Adding Efficiency Table" << std::endl;
    std::cout << "10 Orders Added: " << (t2 - t1).count() << " nanoseconds" << std::endl;
    std::cout << "1000 Orders Added: " << (t3 - t1).count() << " nanoseconds" << std::endl;
    std::cout << "100000 Orders Added: " << (t4 - t1).count() << " nanoseconds" << std::endl;
    std::cout << std::endl;
}

int main() {
    createOrderTimeTable();
    return 0;
}