#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include "naiveOrderBook.h"

// Test Status
// Normal: PASSED
// ASAN: PASSED

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

// Test removing valid orders
TEST(OrderBookTest, HandlesValidOrderRemoving) {
    OrderBook orderBook = OrderBook();
    Order* expected;
    Order* actual;

    // Adding first buy order
    orderBook.addOrder(100.0, 10, "BUY", false); // ID 0
    // Adding first sell order
    orderBook.addOrder(100.0, 10, "SELL", false); // ID 1
    // Adding second buy order
    orderBook.addOrder(110.0, 5, "BUY", false); // ID 2
    // Adding second sell order
    orderBook.addOrder(110.0, 5, "SELL", false); // ID 3
    // Adding third buy order
    orderBook.addOrder(120.0, 1, "BUY", false); // ID 4
    // Adding thrid sell order
    orderBook.addOrder(120.0, 1, "SELL", false); // ID 5
    // Adding fourth buy order
    orderBook.addOrder(130.0, 15, "BUY", false); // ID 6
    // Adding fourth buy order
    orderBook.addOrder(130.0, 15, "SELL", false); // ID 7

    // Remove two orders from each list
    orderBook.removeOrder(0, "BUY", false);
    orderBook.removeOrder(3, "SELL", false);
    orderBook.removeOrder(4, "BUY", false);
    orderBook.removeOrder(7, "SELL", false);

    // Verify expected positions

    expected = new Order(6, 130.0, 15, "BUY");
    actual = orderBook.buyOrderList->order;
    isOrderEqual(expected, actual);
    delete expected;

   expected = new Order(2, 110.0, 5, "BUY");
    actual = orderBook.buyOrderList->next->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(1, 100.0, 10, "SELL");
    actual = orderBook.sellOrderList->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(5, 120.0, 1, "SELL");
    actual = orderBook.sellOrderList->next->order;
    isOrderEqual(expected, actual);
    delete expected;
}

// Test removing invalid orders
TEST(OrderBookTest, HandlesInvalidOrderRemoving) {
    OrderBook orderBook = OrderBook();

    // Test invalid ID
    std::string output = captureOutput([&]() { orderBook.removeOrder(999, "BUY"); });
    EXPECT_EQ(output, "Order Book Error: Invalid Order ID\n");

    // Add orders to bring orderCount up
    orderBook.addOrder(100.0, 10, "BUY", false); // Order ID is 0
    orderBook.addOrder(110.0, 5, "BUY", false); // Order ID is 1
    orderBook.addOrder(100.0, 20, "BUY", false); // Order ID is 2
    orderBook.addOrder(90.0, 15, "BUY", false); // Order ID is 3
    orderBook.addOrder(100.0, 30, "BUY", false); // Order ID is 4

    // Remove an order to test ID not found error
    orderBook.removeOrder(2, "BUY", false);

    // Test ID not found error
    output = captureOutput([&]() { orderBook.removeOrder(2, "BUY"); });
    EXPECT_EQ(output, "Order Book Error: Order ID 2 Not Found\n");

    // Test invalid order type
    output = captureOutput([&]() { orderBook.removeOrder(0, "INVALID"); });
    EXPECT_EQ(output, "Order Book Error: Invalid Order Type\n");
}

// Test no order matches
TEST(OrderBookTest, HandlesNoOrderMatch) {
    OrderBook orderBook = OrderBook();
    Order* expected;
    Order* actual;

    // Add Buy and Sell orders
    orderBook.addOrder(100.0, 10, "BUY", false); // Order ID 0
    orderBook.addOrder(90.0, 5, "BUY", false); // Order ID 1
    orderBook.addOrder(110.0, 15, "SELL", false); // Order ID 2
    orderBook.addOrder(120.0, 30, "SELL", false); // Order ID 3

    // Run order matching
    orderBook.matchOrders(false);

    // Verify that nothing sold

    expected = new Order(0, 100.0, 10, "BUY");
    actual = orderBook.buyOrderList->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(1, 90.0, 5, "BUY");
    actual = orderBook.buyOrderList->next->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(2, 110.0, 15, "SELL");
    actual = orderBook.sellOrderList->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(3, 120.0, 30, "SELL");
    actual = orderBook.sellOrderList->next->order;
    isOrderEqual(expected, actual);
    delete expected;
}

// Test single order match
TEST(OrderBookTest, HandlesSingleOrderMatch) {
    OrderBook orderBook = OrderBook();
    Order* expected;
    Order* actual;

    // Add Buy and Sell orders
    orderBook.addOrder(100.0, 10, "BUY", false); // Order ID 0
    orderBook.addOrder(90.0, 5, "BUY", false); // Order ID 1
    orderBook.addOrder(100.0, 15, "SELL", false); // Order ID 2
    orderBook.addOrder(110.0, 30, "SELL", false); // Order ID 3

    // Run order matching
    orderBook.matchOrders(false);

    // Verify expected result

    expected = new Order(1, 90.0, 5, "BUY");
    actual = orderBook.buyOrderList->order;
    isOrderEqual(expected, actual);
    delete expected;

    // 5 should be left over after 10 are sold
    expected = new Order(2, 100.0, 5, "SELL");
    actual = orderBook.sellOrderList->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(3, 110.0, 30, "SELL");
    actual = orderBook.sellOrderList->next->order;
    isOrderEqual(expected, actual);
    delete expected;
}

// Test multiple order match
TEST(OrderBookTest, HandlesMultipleOrderMatch) {
    OrderBook orderBook = OrderBook();
    Order* expected;
    Order* actual;

    // Add Buy and Sell orders
    orderBook.addOrder(100.0, 20, "BUY", false); // Order ID 0
    orderBook.addOrder(90.0, 5, "BUY", false); // Order ID 1
    orderBook.addOrder(80.0, 10, "SELL", false); // Order ID 2
    orderBook.addOrder(90.0, 5, "SELL", false); // Order ID 3
    orderBook.addOrder(100.0, 10, "SELL", false); // Order ID 4
    orderBook.addOrder(110.0, 20, "SELL", false); // Order ID 5

    // Run order matching
    orderBook.matchOrders(false);

    // Verify expected result

    expected = new Order(1, 90.0, 5, "BUY");
    actual = orderBook.buyOrderList->order;
    isOrderEqual(expected, actual);
    delete expected;
    
    // 5 are left over after 5 are bought
    expected = new Order(4, 100.0, 5, "SELL");
    actual = orderBook.sellOrderList->order;
    isOrderEqual(expected, actual);
    delete expected;

    expected = new Order(5, 110.0, 20, "SELL");
    actual = orderBook.sellOrderList->next->order;
    isOrderEqual(expected, actual);
    delete expected;
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}