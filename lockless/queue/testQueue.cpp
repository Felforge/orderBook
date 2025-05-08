#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "queue.h"
using namespace std;

void threadWorkerInsert(Queue<Node<int>>& queue, Node<int>* node) {
    queue.insert(node);
}

void threadWorkerDelete(Queue<Node<int>>& queue) {
    queue.remove();
}

// Test General Functionality
TEST(OrderListTest, HandlesGeneralUsage) {
    // Create queue
    Queue queue = Queue<Node<int>>();

    // Create some nodes
    Node<int> node1 = Node(1);
    Node<int> node2 = Node(2);
    Node<int> node3 = Node(3);

    // Insert everything
    queue.insert(&node1);
    queue.insert(&node2);
    queue.insert(&node3);

    // Verify expected order
    EXPECT_EQ(queue.head.load()->data, 1);
    EXPECT_EQ(queue.head.load()->next.load()->data, 2);
    EXPECT_EQ(queue.tail.load()->data, 3);

    // Remove one and verify
    queue.remove();
    EXPECT_EQ(queue.head.load()->data, 2);
    EXPECT_EQ(queue.tail.load()->data, 3);

    // Remove another and verify
    queue.remove();
    EXPECT_EQ(queue.head.load()->data, 3);
    EXPECT_EQ(queue.tail.load()->data, 3);

    // Remove last and verify
    queue.remove();
    EXPECT_EQ(queue.head.load(), nullptr);
    EXPECT_EQ(queue.tail.load(), nullptr);
}

// Test Concurrency
TEST(OrderListTest, HandlesConcurrency) {
    // Create queue
    Queue queue = Queue<Node<int>>();

    // To delete later
    vector<Node<int>*> nodes;

    // Create and start threads
    vector<thread> addThreads;
    for (int i = 1; i <= 3; i++) {
        Node<int> node = Node(i);
        nodes.push_back(&node);
        addThreads.emplace_back(threadWorkerInsert, ref(queue), node);
    }

    // Wait for threads to finish
    for (auto& thread : addThreads) {
        thread.join();
    }

    // Verify expected order
    EXPECT_EQ(queue.head.load()->data, 1);
    EXPECT_EQ(queue.head.load()->next.load()->data, 2);
    EXPECT_EQ(queue.tail.load()->data, 3);

    // Delete everything
    vector<thread> removeThreads;
    for (Node<int>* node: nodes) {
        removeThreads.emplace_back(threadWorkerDelete, ref(queue));
    }

    // Wait for threads to finish
    for (auto& thread : removeThreads) {
        thread.join();
    }

    // Verify expected result
    EXPECT_EQ(queue.head.load(), nullptr);
    EXPECT_EQ(queue.tail.load(), nullptr);
}


// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}