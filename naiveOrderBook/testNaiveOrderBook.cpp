#include <gtest/gtest.h>
#include "naiveOrderBook.h"

std::string captureOutput(std::function<void()> func) {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        func();  // Run function that prints output
        std::cout.rdbuf(old);
        return buffer.str();
    }

// Test adding valid buy orders
TEST(OrderBookTest, HandlesValidBuyOrderAdding) {
    OrderBook orderBook = OrderBook();

    // Adding first order
    orderBook.addOrder(100.0, 10, "BUY", false);
    Order* expected = new Order(0, 100.0, 10, "BUY");
    EXPECT_EQ(expected->id, orderBook.buyOrderList->order->id);
    EXPECT_EQ(expected->price, orderBook.buyOrderList->order->price);
    EXPECT_EQ(expected->quantity, orderBook.buyOrderList->order->quantity);
    EXPECT_EQ(expected->type, orderBook.buyOrderList->order->type);
    delete expected;

    // Adding more expensive order
    orderBook.addOrder(110.0, 5, "BUY", false);
    expected = new Order(1, 110.0, 5, "BUY");
    EXPECT_EQ(expected->id, orderBook.buyOrderList->next->order->id);
    EXPECT_EQ(expected->price, orderBook.buyOrderList->next->order->price);
    EXPECT_EQ(expected->quantity, orderBook.buyOrderList->next->order->quantity);
    EXPECT_EQ(expected->type, orderBook.buyOrderList->next->order->type);
    delete expected;

    // Add same price order
    orderBook.addOrder(100.0, 20, "BUY", false);
    expected = new Order(2, 100.0, 20, "BUY");
    EXPECT_EQ(expected->id, orderBook.buyOrderList->next->order->id);
    EXPECT_EQ(expected->price, orderBook.buyOrderList->next->order->price);
    EXPECT_EQ(expected->quantity, orderBook.buyOrderList->next->order->quantity);
    EXPECT_EQ(expected->type, orderBook.buyOrderList->next->order->type);
    delete expected;

    // Add cheaper price order
    orderBook.addOrder(90.0, 15, "BUY", false);
    expected = new Order(3, 90.0, 15, "BUY");
    EXPECT_EQ(expected->id, orderBook.buyOrderList->order->id);
    EXPECT_EQ(expected->price, orderBook.buyOrderList->order->price);
    EXPECT_EQ(expected->quantity, orderBook.buyOrderList->order->quantity);
    EXPECT_EQ(expected->type, orderBook.buyOrderList->order->type);
    delete expected;

    // TRY ADDING ANOTHER SAME PRICE ORDER
}

// Test adding valid sell orders
TEST(OrderBookTest, HandlesValidSellOrderAdding) {
    OrderBook orderBook = OrderBook();
    orderBook.addOrder(100.0, 10, "SELL", false);
    Order* expected = new Order(0, 100.0, 10, "SELL");
    EXPECT_EQ(expected->id, orderBook.sellOrderList->order->id);
    EXPECT_EQ(expected->price, orderBook.sellOrderList->order->price);
    EXPECT_EQ(expected->quantity, orderBook.sellOrderList->order->quantity);
    EXPECT_EQ(expected->type, orderBook.sellOrderList->order->type);
    delete expected;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}