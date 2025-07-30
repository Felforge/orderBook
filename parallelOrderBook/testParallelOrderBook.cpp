#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <unistd.h>
#include "parallelOrderBook.h"
using namespace std;

string captureOutput(function<void()> func) {
    stringstream buffer;
    streambuf* old = cout.rdbuf(buffer.rdbuf());
    func();  // Run function that prints output
    cout.rdbuf(old);
    return buffer.str();
}

int getListIdx(double price, double minPrice) {
    return int((price - minPrice) * 100);
}

// Check if two order pointers are equal in values
void isOrderEqual(Order* expected, Order* actual) {
    EXPECT_EQ(expected->orderID, actual->orderID);
    EXPECT_EQ(expected->userID, actual->userID);
    EXPECT_EQ(expected->price, actual->price);
    EXPECT_EQ(expected->quantity, actual->quantity);
    EXPECT_EQ(expected->side, actual->side);
    EXPECT_EQ(expected->symbolID, actual->symbolID);
}

// Test Adding Valid Buy Order
TEST(OrderBookTest, HandlesValidBuyOrderAdding) {
    OrderBook<1, 1, 2, 1> orderBook = OrderBook<1, 1, 2, 1>();
    SymbolID appleID = orderBook.registerSymbol("AAPL");
    orderBook.addOrder(1, appleID, Side::BUY, 10, 100.0);
    
    Order* expected = new Order(nullptr, 0, 1, Side::BUY, appleID, 10, 100.0);

    // // Find order
    // Order* actual = nullptr;
    // for (unordered_map<int, Node<Order*>*>* map: orderBook.allOrderMaps) {
    //     // If key exists set it to actual
    //     if (map->find(0) != map->end()) {
    //         actual = (*map)[0]->data;
    //     }
    // }

    // // Make sure item was found
    // EXPECT_NE(actual, nullptr);

    // // Make sure orders are equal
    // isOrderEqual(expected, actual);

    delete expected;
}

// // Test Adding Valid Sell Order
// TEST(OrderBookTest, HandlesValidSellOrderAdding) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
//     Order* expected = new Order(nullptr, 0, 1, "SELL", "AAPL", 10, 100.0);
//     usleep(1000000); // Delay to make sure it completes
    
//     // Check Order Map
//     Order* actual = orderBook.orders[0]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->sellOrderList[getListIdx(100.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);

//     delete expected;
// }

// // Test Adding Valid Buy and Sell Order
// TEST(OrderBookTest, HandlesValidBuyAndSellOrderAdding) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
//     usleep(1000000); // Delay to make sure it completes
    
//     // Check Buy 
//     Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 100.0);

//     // Check Order Map
//     Order* actual = orderBook.orders[0]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Check Sell
//     expected = new Order(nullptr, 1, 1, "SELL", "AAPL", 10, 100.0);

//     // Check Order Map
//     actual = orderBook.orders[1]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->sellOrderList[getListIdx(100.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Adding Multiple Same Price Price Orders
// TEST(OrderBookTest, HandlesMultipleSamePriceOrderAdding) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     usleep(1000000); // Delay to make sure it completes
    
//     // First Order
//     Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 100.0);

//     // Check Order Map
//     Order* actual = orderBook.orders[0]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Last Order
//     expected = new Order(nullptr, 2, 1, "BUY", "AAPL", 10, 100.0);

//     // Check Order Map
//     actual = orderBook.orders[2]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]->tail.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Middle Order
//     expected = new Order(nullptr, 1, 1, "BUY", "AAPL", 10, 100.0);

//     // Check Order Map
//     actual = orderBook.orders[1]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]->head.load()->next.load()->order; // From head
//     isOrderEqual(expected, actual);
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]->tail.load()->prev.load()->order; // From tail
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Adding Multiple Different Price Price Orders
// TEST(OrderBookTest, HandlesMultipleDifferentPriceOrderAdding) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 90.0, false);
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 110.0, false);
//     usleep(1000000); // Delay to make sure it completes
    
//     // First Order
//     Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 90.0);

//     // Check Order Map
//     Order* actual = orderBook.orders[0]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(90.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Second Order
//     expected = new Order(nullptr, 1, 1, "BUY", "AAPL", 10, 100.0);

//     // Check Order Map
//     actual = orderBook.orders[1]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Last Order
//     expected = new Order(nullptr, 2, 1, "BUY", "AAPL", 10, 110.0);

//     // Check Order Map
//     actual = orderBook.orders[2]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(110.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Adding Best Buy Order
// TEST(OrderBookTest, HandlesBestBuyOrderAdding) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     Ticker* tickerPtr = orderBook.tickerMap["AAPL"];
//     usleep(1000000); // Delay to make sure it completes
    
//     // Check Initial
//     Order* actual = tickerPtr->buyOrderList[tickerPtr->bestBuyIdx]->head.load()->order;
//     Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 100.00);
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Add New Best Order
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 110.0, false);

//     // Check Final
//     actual = tickerPtr->buyOrderList[tickerPtr->bestBuyIdx]->head.load()->order;
//     expected = new Order(nullptr, 1, 1, "BUY", "AAPL", 10, 110.00);
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Adding Best Sell Order
// TEST(OrderBookTest, HandlesBestSellOrderAdding) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
//     Ticker* tickerPtr = orderBook.tickerMap["AAPL"];
//     usleep(1000000); // Delay to make sure it completes
    
//     // Check Initial
//     Order* actual = tickerPtr->sellOrderList[tickerPtr->bestSellIdx]->head.load()->order;
//     Order* expected = new Order(nullptr, 0, 1, "SELL", "AAPL", 10, 100.00);
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Add New Best Order
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 90.0, false);

//     // Check Final
//     actual = tickerPtr->sellOrderList[tickerPtr->bestSellIdx]->head.load()->order;
//     expected = new Order(nullptr, 1, 1, "SELL", "AAPL", 10, 90.00);
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Adding Orders For Different Tickers
// TEST(OrderBookTest, HandlesDifferentTickerOrderAdding) {
//     OrderBook orderBook = OrderBook(2, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addTicker("AMZN");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AMZN", "BUY", 10, 100.0, false);
//     usleep(1000000); // Delay to make sure it completes

//     // Apple Order
//     Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 100.0);

//     // Check Order Map
//     Order* actual = orderBook.orders[0]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Amazon Order
//     expected = new Order(nullptr, 1, 1, "BUY", "AMZN", 10, 100.0);

//     // Check Order Map
//     actual = orderBook.orders[1]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AMZN"]->buyOrderList[getListIdx(100.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Removing Valid Buy Order
// TEST(OrderBookTest, HandlesValidBuyOrderRemoving) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.removeOrder(0, false);
//     usleep(1000000); // Delay to make sure it completes

//     // Make sure order map is empty
//     EXPECT_TRUE(isEmpty(orderBook.orders));

//     // Make sure price level is empty
//     // Should be nullptr which is false
//     EXPECT_TRUE(!orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]);
// }

// // Test Removing Valid Sell Order
// TEST(OrderBookTest, HandlesValidSellOrderRemoving) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
//     orderBook.removeOrder(0, false);

//     // Make sure order map is empty
//     EXPECT_TRUE(isEmpty(orderBook.orders));

//     // Make sure price level is empty
//     // Should be nullptr which is false
//     EXPECT_TRUE(!orderBook.tickerMap["AAPL"]->sellOrderList[getListIdx(100.0, 50.0)]);
// }

// // Test Removing Duplicate Orders
// TEST(OrderBookTest, HandlesDuplicateOrderRemoving) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     usleep(1000000); // Delay to make sure it completes

//     // Remove first two orders
//     orderBook.removeOrder(0, false);
//     orderBook.removeOrder(1, false);

//     // Create expected order
//     Order* expected = new Order(nullptr, 2, 1, "BUY", "AAPL", 10, 100.0);

//     // Check Order Map
//     Order* actual = orderBook.orders[2]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);

//     delete expected;
// }

// // Test Updating Best Buy Order Upon Remove
// TEST(OrderBookTest, HandlesBestBuyOrderRemoving) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 110.0, false);
//     Ticker* tickerPtr = orderBook.tickerMap["AAPL"];
//     usleep(1000000); // Delay to make sure it completes

//     // Check Initial
//     Order* actual = tickerPtr->buyOrderList[tickerPtr->bestBuyIdx]->head.load()->order;
//     Order* expected = new Order(nullptr, 1, 1, "BUY", "AAPL", 10, 110.00);
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Remove Best Order
//     orderBook.removeOrder(1, false);

//     // Check Final
//     actual = tickerPtr->buyOrderList[tickerPtr->bestBuyIdx]->head.load()->order;
//     expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 100.00);
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Updating Best Sell Order Upon Remove
// TEST(OrderBookTest, HandlesBestSellOrderRemoving) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 90.0, false);
//     Ticker* tickerPtr = orderBook.tickerMap["AAPL"];
//     usleep(1000000); // Delay to make sure it completes

//     // Check Initial
//     Order* actual = tickerPtr->sellOrderList[tickerPtr->bestSellIdx]->head.load()->order;
//     Order* expected = new Order(nullptr, 1, 1, "SELL", "AAPL", 10, 90.00);
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Remove Best Order
//     orderBook.removeOrder(1, false);

//     // Check Final
//     actual = tickerPtr->sellOrderList[tickerPtr->bestSellIdx]->head.load()->order;
//     expected = new Order(nullptr, 0, 1, "SELL", "AAPL", 10, 100.00);
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Matching Equal Orders
// TEST(OrderBookTest, HandlesMatchingEqualOrders) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
//     orderBook.matchOrders("AAPL", false);

//     // Verify lists are empty
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->buyOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->buyOrderMap.end());
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->sellOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->sellOrderMap.end());
// }

// // Test Matching Greater Buy
// TEST(OrderBookTest, HandlesMatchingGreaterBuy) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 5, 100.0, false);
//     orderBook.matchOrders("AAPL", false);

//     // Verify Expected Results
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->sellOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->sellOrderMap.end());
//     Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 5, 100.00);
//     Order* actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Matching Greater Sell
// TEST(OrderBookTest, HandlesMatchingGreaterSell) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 5, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
//     orderBook.matchOrders("AAPL", false);

//     // Verify Expected Results
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->buyOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->buyOrderMap.end());
//     Order* expected = new Order(nullptr, 1, 1, "SELL", "AAPL", 5, 100.00);
//     Order* actual = orderBook.tickerMap["AAPL"]->sellOrderMap[100.0]->head->order;
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Matching Multiple Orders
// TEST(OrderBookTest, HandlesMatchingDouble) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
//     orderBook.matchOrders("AAPL", false);

//     // Verify Lists Are Empty
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->buyOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->buyOrderMap.end());
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->sellOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->sellOrderMap.end());
// }

// // Test Matching Two To One
// TEST(OrderBookTest, HandlesMatchingTwoToOne) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 5, 100.0, false); 
//     orderBook.addOrder(1, "AAPL", "SELL", 5, 100.0, false); 
//     orderBook.matchOrders("AAPL", false);

//     // Verify Lists Are Empty
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->buyOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->buyOrderMap.end());
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->sellOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->sellOrderMap.end());
// }

// // Test Matching Unequal Orders
// TEST(OrderBookTest, HandlesMatchingUnequalOrders) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 90.0, false); 

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.matchOrders("AAPL"); });
//     EXPECT_EQ(output, "Order successfully completed:\n  Ticker: AAPL\n  Price: 90\n  Quantity: 10\n");
// }


// // Test Matching Multiple Unequal Orders
// TEST(OrderBookTest, HandlesMatchingUnequalDouble) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 110.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 95.0, false);
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 90.0, false);
//     orderBook.matchOrders("AAPL", false);

//     // Verify Lists Are Empty
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->buyOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->buyOrderMap.end());
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->sellOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->sellOrderMap.end());
// }

// // Test Matching Across Different Tickers
// TEST(OrderBookTest, HandlesMatchingDifferentTickers) {
//     OrderBook orderBook = OrderBook(2, 100000);
//     orderBook.addTicker("AAPL");
//     orderBook.addTicker("AMZN");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);
//     orderBook.addOrder(1, "AMZN", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AMZN", "SELL", 10, 100.0, false);
//     orderBook.matchOrders("AAPL", false);
//     orderBook.matchOrders("AMZN", false);

//     // Verify Lists Are Empty
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->buyOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->buyOrderMap.end());
//     EXPECT_TRUE(orderBook.tickerMap["AAPL"]->sellOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->sellOrderMap.end());
//     EXPECT_TRUE(orderBook.tickerMap["AMZN"]->buyOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->buyOrderMap.end());
//     EXPECT_TRUE(orderBook.tickerMap["AMZN"]->sellOrderMap.find(100.0) == orderBook.tickerMap["AAPL"]->sellOrderMap.end());
// }

// // Test No Match
// TEST(OrderBookTest, HandlesNoMatch) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 110.0, false);
//     orderBook.matchOrders("AAPL", false);

//     // Verify No Change

//     // Buy Order
//     Order* expected = new Order(nullptr, 0, 1, "BUY", "AAPL", 10, 100.0);

//     // Check Order Map
//     Order* actual = orderBook.orders[0]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->buyOrderList[getListIdx(100.0, 50.0)]->head.load()->order;
//     isOrderEqual(expected, actual);
//     delete expected;

//     // Sell Order
//     expected = new Order(nullptr, 1, 1, "SELL", "AAPL", 10, 110.0);

//     // Check Order Map
//     actual = orderBook.orderMap[1]->order;
//     isOrderEqual(expected, actual);

//     // Check Order List
//     actual = orderBook.tickerMap["AAPL"]->sellOrderMap[110.0]->head->order;
//     isOrderEqual(expected, actual);
//     delete expected;
// }

// // Test Adding Too Many Tickers
// TEST(OrderBookTest, HandlesTooManyTickers) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.addTicker("AMZN"); });
//     EXPECT_EQ(output, "Order Book Error: Too Many Tickers\n");
// }

// // Test Adding Invalid Order Type
// TEST(OrderBookTest, HandlesInvalidOrderType) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.addOrder(1, "AAPL", "HOLD", 10, 100.0, false); });
//     EXPECT_EQ(output, "Order Book Error: Invalid Order Side\n");
// }

// // Test Adding Invalid Order Price
// TEST(OrderBookTest, HandlesInvalidOrderPrice) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.addOrder(1, "AAPL", "BUY", 10, 0.0, false); });
//     EXPECT_EQ(output, "Order Book Error: Price Must Be A Number Greater Than 0\n");
// }

// // Test Adding Invalid Order Quantity
// TEST(OrderBookTest, HandlesInvalidOrderQuantity) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.addOrder(1, "AAPL", "BUY", -5, 100.0, false); });
//     EXPECT_EQ(output, "Order Book Error: Quantity Must Be An Integer Greater Than 0\n");
// }

// // Test Invalid Ticker
// TEST(OrderBookTest, HandlesInvaildTicker) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.addOrder(1, "AMZN", "BUY", 10, 100.0, false); });
//     EXPECT_EQ(output, "Order Book Error: Ticker is Invalid\n");
// }

// // Test Removing Invalid Order ID
// TEST(OrderBookTest, HandlesInvalidOrderID) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.removeOrder(1); });
//     EXPECT_EQ(output, "Order Book Error: Invalid Order ID\n");
// }

// // Test Matching Empty Order Book
// TEST(OrderBookTest, HandlesEmptyMatch) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.matchOrders("AAPL"); });
//     EXPECT_EQ(output, "Order Book Error: No Orders To Be Matched\n");
// }

// // Test Matching Only Buy
// TEST(OrderBookTest, HandlesMatchingOnlyBuy) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false);

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.matchOrders("AAPL"); });
//     EXPECT_EQ(output, "Order Book Error: No Orders To Be Matched\n");
// }

// // Test Matching Only Sell
// TEST(OrderBookTest, HandlesMatchingOnlySell) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "SELL", 10, 100.0, false);

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.matchOrders("AAPL"); });
//     EXPECT_EQ(output, "Order Book Error: No Orders To Be Matched\n");
// }

// // Test Matching Invalid Ticker
// TEST(OrderBookTest, HandlesMatchingInvalidTicker) {
//     OrderBook orderBook = OrderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.matchOrders("AMZN"); });
//     EXPECT_EQ(output, "Order Book Error: Ticker is Invalid\n");
// }

// // Test Exceeding Order Limit
// TEST(OrderBookTest, HandlesExceedingOrderLimit) {
//     OrderBook orderBook = OrderBook(1, 0, 50.0, 150.0);
//     orderBook.addTicker("AAPL");

//     // Verify output
//     std::string output = captureOutput([&]() { orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0, false); });
//     EXPECT_EQ(output, "Order Book Error: Max Order Limit Reached\n");
// }

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}