#include <gtest/gtest.h>
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
void isOrderEqual(Order* order1, Order* order2) {
    EXPECT_EQ(order1->id, order2->id);
    EXPECT_EQ(order1->price, order2->price);
    EXPECT_EQ(order1->quantity, order2->quantity);
    EXPECT_EQ(order1->type, order2->type);
}

// Test adding valid orders
TEST(OrderBookTest, HandlesValidOrderAdding) {
    OrderBook orderBook = OrderBook();
    OrderList* selectedList;
    Order* expected;
    Order* actual;
    std::string orderType;

    for (int i = 0; i < 2; i++) {
        // Select buy or sell
        if (i) {
            selectedList = orderBook.buyOrderList;
            orderType = "BUY";
        } else {
            selectedList = orderBook.sellOrderList;
            orderType = "SELL";
        }

        // Adding first order
        orderBook.addOrder(100.0, 10, orderType, false);
        expected = new Order(0, 100.0, 10, orderType);
        actual = selectedList->order;
        isOrderEqual(expected, actual);
        delete expected;

        // Adding more expensive order
        orderBook.addOrder(110.0, 5, orderType, false);
        expected = new Order(1, 110.0, 5, orderType);
        actual = selectedList->next->order;
        isOrderEqual(expected, actual);
        delete expected;

        // Add same price order
        orderBook.addOrder(100.0, 20, orderType, false);
        expected = new Order(2, 100.0, 20, orderType);
        actual = selectedList->next->order;
        isOrderEqual(expected, actual);
        delete expected;

        // Add cheaper price order
        orderBook.addOrder(90.0, 15, orderType, false);
        expected = new Order(3, 90.0, 15, orderType);
        actual = selectedList->order;
        isOrderEqual(expected, actual);
        delete expected;

        // TRY ADDING ANOTHER SAME PRICE ORDER
        orderBook.addOrder(100.0, 30, orderType, false);
        expected = new Order(4, 100.0, 30, orderType);
        actual = selectedList->next->next->next->order;
        isOrderEqual(expected, actual);
        delete expected;
    }
}

// Test adding valid sell orders
// TEST(OrderBookTest, HandlesValidSellOrderAdding) {
//     OrderBook orderBook = OrderBook();
//     orderBook.addOrder(100.0, 10, "SELL", false);
//     Order* expected = new Order(0, 100.0, 10, "SELL");
//     EXPECT_EQ(expected->id, orderBook.sellOrderList->order->id);
//     EXPECT_EQ(expected->price, orderBook.sellOrderList->order->price);
//     EXPECT_EQ(expected->quantity, orderBook.sellOrderList->order->quantity);
//     EXPECT_EQ(expected->type, orderBook.sellOrderList->order->type);
//     delete expected;
// }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}