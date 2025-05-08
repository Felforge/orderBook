#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "orderList.h"
using namespace std;

string captureOutput(function<void()> func) {
    stringstream buffer;
    streambuf* old = cout.rdbuf(buffer.rdbuf());
    func();  // Run function that prints output
    cout.rdbuf(old);
    return buffer.str();
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

void threadWorkerInsert(OrderList& orderList, OrderNode* node) {
    orderList.insert(node);
}

void threadWorkerDelete(OrderList& orderList, OrderNode* node) {
    orderList.remove(node);
}

// Test General Functionality
TEST(OrderListTest, HandlesGeneralUsage) {
    // Create memory pools
    MemoryPool orderPool(sizeof(Order), 4);
    MemoryPool nodePool(sizeof(OrderNode), 4);
    MemoryPool listPool(sizeof(OrderList), 1);

    // Create the OrderList
    void* block = listPool.allocate();
    OrderList orderList(block, orderPool, nodePool);

    // Create some memory blocks
    void* orderBlock1 = orderPool.allocate();
    void* orderBlock2 = orderPool.allocate();
    void* orderBlock3 = orderPool.allocate();
    void* nodeBlock1 = nodePool.allocate();
    void* nodeBlock2 = nodePool.allocate();
    void* nodeBlock3 = nodePool.allocate();

    // Create some orders and nodes
    Order* order1 = new (orderBlock1) Order(orderBlock1, 0, 1, "BUY", "AAPL", 10, 100.0);
    Order* order2 = new (orderBlock2) Order(orderBlock2, 1, 1, "BUY", "AAPL", 10, 100.0);
    Order* order3 = new (orderBlock3) Order(orderBlock3, 2, 1, "BUY", "AAPL", 10, 100.0);
    OrderNode* node1 = new (nodeBlock1) OrderNode(nodeBlock1, order1);
    OrderNode* node2 = new (nodeBlock2) OrderNode(nodeBlock2, order2);
    OrderNode* node3 = new (nodeBlock3) OrderNode(nodeBlock3, order3);

    // Insert everything
    orderList.insert(node1);
    orderList.insert(node2);
    orderList.insert(node3);

    // Verify expected order
    Order* actualHead = orderList.head.load()->order;
    Order* actualNext = orderList.head.load()->next.load()->order;
    isOrderEqual(order1, actualHead);
    isOrderEqual(order2, actualNext);
    EXPECT_EQ(orderList.head.load()->prev.load(), nullptr);

    Order* actualPrev = orderList.head.load()->next.load()->prev.load()->order;
    actualNext = orderList.head.load()->next.load()->next.load()->order;
    isOrderEqual(order1, actualPrev);
    isOrderEqual(order3, actualNext);

    Order* actualTail = orderList.tail.load()->order;
    actualPrev = orderList.tail.load()->prev.load()->order;
    isOrderEqual(order3, actualTail);
    isOrderEqual(order2, actualPrev);
    EXPECT_EQ(orderList.tail.load()->next.load(), nullptr);

    // Remove middle and verify expected
    orderList.remove(node2);
    actualHead = orderList.head.load()->order;
    actualTail = orderList.tail.load()->order;
    actualNext = orderList.head.load()->next.load()->order;
    actualPrev = orderList.tail.load()->prev.load()->order;
    isOrderEqual(order1, actualHead);
    isOrderEqual(order3, actualTail);
    isOrderEqual(order3, actualNext);
    isOrderEqual(order1, actualPrev);

    // Remove Head
    orderList.remove(node1);
    actualHead = orderList.head.load()->order;
    actualTail = orderList.tail.load()->order;
    isOrderEqual(order3, actualHead);
    isOrderEqual(order3, actualTail);
    EXPECT_EQ(orderList.head.load()->prev.load(), nullptr);
    EXPECT_EQ(orderList.head.load()->next.load(), nullptr);

    // Add and remove tail
    void* orderBlock4 = orderPool.allocate();
    void* nodeBlock4 = nodePool.allocate();
    Order* order4 = new (orderBlock4) Order(orderBlock4, 2, 1, "BUY", "AAPL", 10, 100.0);
    OrderNode* node4 = new (nodeBlock4) OrderNode(nodeBlock4, order4);
    orderList.insert(node4);
    orderList.remove(node4);
    actualHead = orderList.head.load()->order;
    actualTail = orderList.tail.load()->order;
    isOrderEqual(order3, actualHead);
    isOrderEqual(order3, actualTail);
    EXPECT_EQ(orderList.head.load()->prev.load(), nullptr);
    EXPECT_EQ(orderList.head.load()->next.load(), nullptr);

    // Remove Last
    orderList.remove(node3);
    EXPECT_EQ(orderList.head.load(), nullptr);
    EXPECT_EQ(orderList.tail.load(), nullptr);
}

// Test Concurrency
TEST(OrderListTest, HandlesConcurrency) {
    // Create memory pools
    MemoryPool orderPool(sizeof(Order), 3);
    MemoryPool nodePool(sizeof(OrderNode), 3);
    MemoryPool listPool(sizeof(OrderList), 1);

    // Create the OrderList
    void* block = listPool.allocate();
    OrderList orderList(block, orderPool, nodePool);

    // To delete later
    vector<OrderNode*> nodes;

    // Create and start threads
    vector<thread> addThreads;
    for (int i = 0; i < 3; i++) {
        void* nodeBlock = nodePool.allocate();
        void* orderBlock = orderPool.allocate();
        Order* order = new (orderBlock) Order(orderBlock, i, 1, "BUY", "AAPL", 50, 100.0);
        OrderNode* node = new (nodeBlock) OrderNode(nodeBlock, order);
        nodes.push_back(node);
        addThreads.emplace_back(threadWorkerInsert, ref(orderList), node);
    }

    // Wait for threads to finish
    for (auto& thread : addThreads) {
        thread.join();
    }

    // Declare expected orders
    Order* order1 = new Order(nullptr, 0, 1, "BUY", "AAPL", 50, 100.0);
    Order* order2 = new Order(nullptr, 1, 1, "BUY", "AAPL", 50, 100.0);
    Order* order3 = new Order(nullptr, 2, 1, "BUY", "AAPL", 50, 100.0);

    // Verify expected order
    Order* actualHead = orderList.head.load()->order;
    Order* actualNext = orderList.head.load()->next.load()->order;
    isOrderEqual(order1, actualHead);
    isOrderEqual(order2, actualNext);
    EXPECT_EQ(orderList.head.load()->prev.load(), nullptr);

    Order* actualPrev = orderList.head.load()->next.load()->prev.load()->order;
    actualNext = orderList.head.load()->next.load()->next.load()->order;
    isOrderEqual(order1, actualPrev);
    isOrderEqual(order3, actualNext);

    Order* actualTail = orderList.tail.load()->order;
    actualPrev = orderList.tail.load()->prev.load()->order;
    isOrderEqual(order3, actualTail);
    isOrderEqual(order2, actualPrev);
    EXPECT_EQ(orderList.tail.load()->next.load(), nullptr);

    // Clear pointers
    delete order1;
    delete order2;
    delete order3;

    // Delete everything
    vector<thread> removeThreads;
    for (OrderNode* node: nodes) {
        removeThreads.emplace_back(threadWorkerDelete, ref(orderList), node);
    }

    // Wait for threads to finish
    for (auto& thread : removeThreads) {
        thread.join();
    }

    // Verify expected result
    EXPECT_EQ(orderList.head.load(), nullptr);
    EXPECT_EQ(orderList.tail.load(), nullptr);
}

// Test Illegal Remove
TEST(OrderListTest, HandlesIllegalRemove) {
    // Create memory pools
    MemoryPool orderPool(sizeof(Order), 1);
    MemoryPool nodePool(sizeof(OrderNode), 1);
    MemoryPool listPool(sizeof(OrderList), 1);

    // Create the OrderList
    void* block = listPool.allocate();
    OrderList orderList(block, orderPool, nodePool);

    // Create memory blocks
    void* orderBlock = orderPool.allocate();
    void* nodeBlock = nodePool.allocate();

    // Create order
    Order* order = new (orderBlock) Order(orderBlock, 0, 1, "BUY", "AAPL", 10, 100.0);
    OrderNode* node = new (nodeBlock) OrderNode(nodeBlock, order);

    // Insert order
    orderList.insert(node);

    // Remove order
    orderList.remove(node);

    // Verify expected output
    string output = captureOutput([&]() { orderList.remove(node); });
    EXPECT_EQ(output, "");
}


// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}