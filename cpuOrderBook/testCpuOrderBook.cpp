#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include "cpuOrderBook.h"

std::string captureOutput(std::function<void()> func) {
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    func();  // Run function that prints output
    std::cout.rdbuf(old);
    return buffer.str();
}

// Get location in order list
int getListIndex(double price) {
    return int(price * 100.0) - 1;
}

// Check if two order pointers are equal in values
void isOrderEqual(Order* expected, Order* actual) {
    EXPECT_EQ(expected->orderID, actual->orderID);
    EXPECT_EQ(expected->userID, actual->userID);
    EXPECT_EQ(expected->price, actual->price);
    EXPECT_EQ(expected->quantity, actual->quantity);
    EXPECT_EQ(expected->side, actual->side);
    EXPECT_EQ(expected->ticker, actual->ticker);
}

// Test Adding Valid Buy Order
TEST(OrderBookTest, HandlesValidBuyOrderAdding) {
    OrderBook orderBook = OrderBook(1);
    orderBook.addTicker("AAPL");
    orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
    Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 100.0);

    // Check Order Map
    Order* actual = orderBook.orderMap[0]->order;
    isOrderEqual(expected, actual);

    // Check Order List
    actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIndex(100.0)]->head->order;
    isOrderEqual(expected, actual);

    delete expected;
}

// Test Adding Valid Sell Order
TEST(OrderBookTest, HandlesValidSellOrderAdding) {
    OrderBook orderBook = OrderBook(1);
    orderBook.addTicker("AAPL");
    orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
    Order* expected = new Order(nullptr, 0, 1, "SELL", "AAPL", 10, 100.0);
    
    // Check Order Map
    Order* actual = orderBook.orderMap[0]->order;
    isOrderEqual(expected, actual);

    // Check Order List
    actual = orderBook.tickerMap["AAPL"]->sellOrderList[getListIndex(100.0)]->head->order;
    isOrderEqual(expected, actual);

    delete expected;
}

// Test Adding Valid Buy and Sell Order
TEST(OrderBookTest, HandlesValidBuyAndSellOrderAdding) {
    OrderBook orderBook = OrderBook(1);
    orderBook.addTicker("AAPL");
    orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
    orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
    
    // Check Buy 
    Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 100.0);

    // Check Order Map
    Order* actual = orderBook.orderMap[0]->order;
    isOrderEqual(expected, actual);

    // Check Order List
    actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIndex(100.0)]->head->order;
    isOrderEqual(expected, actual);
    delete expected;

    // Check Sell
    expected = new Order(nullptr, 1, 1, "SELL", "AAPL", 10, 100.0);

    // Check Order Map
    actual = orderBook.orderMap[1]->order;
    isOrderEqual(expected, actual);

    // Check Order List
    actual = orderBook.tickerMap["AAPL"]->sellOrderList[getListIndex(100.0)]->head->order;
    isOrderEqual(expected, actual);
    delete expected;
}

// Test Adding Multiple Same Price Price Orders
TEST(OrderBookTest, HandlesMultipleSamePriceOrderAdding) {
    OrderBook orderBook = OrderBook(1);
    orderBook.addTicker("AAPL");
    orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
    orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
    orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
    
    // First Order
    Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 100.0);

    // Check Order Map
    Order* actual = orderBook.orderMap[0]->order;
    isOrderEqual(expected, actual);

    // Check Order List
    actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIndex(100.0)]->head->order;
    isOrderEqual(expected, actual);
    delete expected;

    // Last Order
    expected = new Order(nullptr, 2, 1, "BUY", "AAPL", 10, 100.0);

    // Check Order Map
    actual = orderBook.orderMap[2]->order;
    isOrderEqual(expected, actual);

    // Check Order List
    actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIndex(100.0)]->tail->order;
    isOrderEqual(expected, actual);
    delete expected;

    // Middle Order
    expected = new Order(nullptr, 1, 1, "BUY", "AAPL", 10, 100.0);

    // Check Order Map
    actual = orderBook.orderMap[1]->order;
    isOrderEqual(expected, actual);

    // Check Order List
    actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIndex(100.0)]->head->next->order; // From head
    isOrderEqual(expected, actual);
    actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIndex(100.0)]->tail->prev->order; // From tail
    isOrderEqual(expected, actual);
    delete expected;
}

// Test Adding Minimum Valid Price Order
TEST(OrderBookTest, HandlesMinimumPriceOrderAdding) {
    OrderBook orderBook = OrderBook(1);
    orderBook.addTicker("AAPL");
    orderBook.addOrder(1, "AAPL", "BUY", 10, 0.01, false);
    Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 0.01);
    
    // Check Order Map
    Order* actual = orderBook.orderMap[0]->order;
    isOrderEqual(expected, actual);

    // Check Order List
    actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIndex(0.01)]->head->order;
    isOrderEqual(expected, actual);

    delete expected;
}

// Test Adding Maximum Valid Price Order
TEST(OrderBookTest, HandlesMaximumPriceOrderAdding) {
    OrderBook orderBook = OrderBook(1);
    orderBook.addTicker("AAPL");
    orderBook.addOrder(1, "AAPL", "BUY", 10, 1000.0, false);
    Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 1000.00);
    
    // Check Order Map
    Order* actual = orderBook.orderMap[0]->order;
    isOrderEqual(expected, actual);

    // Check Order List
    actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIndex(1000.0)]->head->order;
    isOrderEqual(expected, actual);

    delete expected;
}

// Test Adding Best Buy Order
TEST(OrderBookTest, HandlesBestBuyOrderAdding) {
    OrderBook orderBook = OrderBook(1);
    orderBook.addTicker("AAPL");
    orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
    
    // Check Initial
    Order* actual = orderBook.tickerMap["AAPL"]->bestBuyOrder->head->order;
    Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 100.00);
    isOrderEqual(expected, actual);
    delete expected;

    // Add New Best Order
    orderBook.addOrder(1, "AAPL", "BUY", 10, 110.0, false);

    // Check Final
    actual = orderBook.tickerMap["AAPL"]->bestBuyOrder->head->order;
    expected = new Order(nullptr, 1, 1, "BUY", "AAPL", 10, 110.00);
    isOrderEqual(expected, actual);
    delete expected;
}

// Test Adding Best Sell Order
TEST(OrderBookTest, HandlesBestSellOrderAdding) {
    OrderBook orderBook = OrderBook(1);
    orderBook.addTicker("AAPL");
    orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
    
    // Check Initial
    Order* actual = orderBook.tickerMap["AAPL"]->bestSellOrder->head->order;
    Order* expected = new Order(nullptr, 0, 1, "SELL", "AAPL", 10, 100.00);
    isOrderEqual(expected, actual);
    delete expected;

    // Add New Best Order
    orderBook.addOrder(1, "AAPL", "SELL", 10, 90.0, false);

    // Check Final
    actual = orderBook.tickerMap["AAPL"]->bestSellOrder->head->order;
    expected = new Order(nullptr, 1, 1, "SELL", "AAPL", 10, 90.00);
    isOrderEqual(expected, actual);
    delete expected;
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}