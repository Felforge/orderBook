#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include "naiveOrderBook.h"

std::string captureOutput(std::function<void()> func) {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        func();  // Run function that prints output
        std::cout.rdbuf(old);
        return buffer.str();
    }

// Check if two order pointers are equal in values
void isOrderEqual(Order* expected, Order* actual) {
    EXPECT_EQ(expected->id, actual->id);
    EXPECT_EQ(expected->price, actual->price);
    EXPECT_EQ(expected->quantity, actual->quantity);
    EXPECT_EQ(expected->type, actual->type);
}

// Test adding valid buy orders
TEST(OrderBookTest, HandlesValidBuyOrderAdding) {
    OrderBook orderBook = OrderBook();
    Order* expected;
    Order* actual;

    // Adding first order
    orderBook.addOrder(100.0, 10, "BUY", false);
    // Adding more expensive order
    orderBook.addOrder(110.0, 5, "BUY", false);
    // Add same price order
    orderBook.addOrder(100.0, 20, "BUY", false);
    // Add cheaper price order
    orderBook.addOrder(90.0, 15, "BUY", false);
    // Add another same price order
    orderBook.addOrder(100.0, 30, "BUY", false);

    expected = new Order(0, 100.0, 10, "BUY");
    actual = orderBook.buyOrderList->next->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(1, 110.0, 5, "BUY");
    actual = orderBook.buyOrderList->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(2, 100.0, 20, "BUY");
    actual = orderBook.buyOrderList->next->next->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(3, 90.0, 15, "BUY");
    actual = orderBook.buyOrderList->next->next->next->next->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(4, 100.0, 30, "BUY");
    actual = orderBook.buyOrderList->next->next->next->order;
    isOrderEqual(expected, actual);
    delete expected;
}

// Test adding valid sell orders
TEST(OrderBookTest, HandlesValidSellOrderAdding) {
    OrderBook orderBook = OrderBook();
    Order* expected;
    Order* actual;

    // Adding first order
    orderBook.addOrder(100.0, 10, "SELL", false);
    // Adding more expensive order
    orderBook.addOrder(110.0, 5, "SELL", false);
    // Add same price order
    orderBook.addOrder(100.0, 20, "SELL", false);
    // Add cheaper price order
    orderBook.addOrder(90.0, 15, "SELL", false);
    // Add another same price order
    orderBook.addOrder(100.0, 30, "SELL", false);

    expected = new Order(0, 100.0, 10, "SELL");
    actual = orderBook.sellOrderList->next->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(1, 110.0, 5, "SELL");
    actual = orderBook.sellOrderList->next->next->next->next->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(2, 100.0, 20, "SELL");
    actual = orderBook.sellOrderList->next->next->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(3, 90.0, 15, "SELL");
    actual = orderBook.sellOrderList->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(4, 100.0, 30, "SELL");
    actual = orderBook.sellOrderList->next->next->next->order;
    isOrderEqual(expected, actual);
    delete expected;
}

// Test adding invalid orders
TEST(OrderBookTest, HandlesInvalidOrderAdding) {
    OrderBook orderBook = OrderBook();

    // Invalid order type
    std::string output = captureOutput([&]() { orderBook.addOrder(100.0, 10, "INVALID"); });
    EXPECT_EQ(output, "Order Book Error: Invalid Order Type\n");

    // Invalid order quantity
    output = captureOutput([&]() { orderBook.addOrder(100.0, -1, "BUY"); });
    EXPECT_EQ(output, "Order Book Error: Quantity Must Be An Integer Greater Than 0\n");

    // Invalid order price
    output = captureOutput([&]() { orderBook.addOrder(0.0, 10, "BUY"); });
    EXPECT_EQ(output, "Order Book Error: Price Must Be A Number Greater Than 0\n");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}