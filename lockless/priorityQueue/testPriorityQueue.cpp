#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "priorityQueue.h"
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
    PriorityQueue queue = PriorityQueue();

    // Create some nodes
    Node<int> node1 = Node(1);
    Node<int> node2 = Node(2);
    Node<int> node3 = Node(3);

    // Insert everything
    queue.insert(&node1);
    queue.insert(&node2);
    queue.insert(&node3);

    // Verify expected order
    EXPECT_EQ(queue.head.load()->data, 3);
    EXPECT_EQ(queue.head.load()->next.load()->data, 2);
    EXPECT_EQ(queue.tail.load()->data, 1);

    // Remove one and verify
    queue.remove();
    EXPECT_EQ(queue.head.load()->data, 2);
    EXPECT_EQ(queue.tail.load()->data, 1);

    // Remove another and verify
    queue.remove();
    EXPECT_EQ(queue.head.load()->data, 1);
    EXPECT_EQ(queue.tail.load()->data, 1);

    // Remove last and verify
    queue.remove();
    EXPECT_EQ(queue.head.load(), nullptr);
    EXPECT_EQ(queue.tail.load(), nullptr);
}

// Test Concurrency
TEST(OrderListTest, HandlesConcurrency) {
    // Create queue
    PriorityQueue queue = PriorityQueue();

    // Create nodes
    Node<int>* node1 = new Node(1);
    Node<int>* node2 = new Node(2);
    Node<int>* node3 = new Node(3);

    // Create and start threads
    vector<thread> addThreads;
    addThreads.emplace_back(threadWorkerInsert, ref(queue), node1);
    addThreads.emplace_back(threadWorkerInsert, ref(queue), node2);
    addThreads.emplace_back(threadWorkerInsert, ref(queue), node3);

    // Wait for threads to finish
    for (auto& thread : addThreads) {
        thread.join();
    }

    // Verify expected order
    EXPECT_EQ(queue.head.load()->data, 3);
    EXPECT_EQ(queue.head.load()->next.load()->data, 2);
    EXPECT_EQ(queue.tail.load()->data, 1);

    // Delete everything
    vector<thread> removeThreads;
    for (int i = 0; i < 3; i++) {
        removeThreads.emplace_back(threadWorkerDelete, ref(queue));
    }

    // Wait for threads to finish
    for (auto& thread : removeThreads) {
        thread.join();
    }

    // Verify expected result
    EXPECT_EQ(queue.head.load(), nullptr);
    EXPECT_EQ(queue.tail.load(), nullptr);

    // Clear memory
    delete node1;
    delete node2;
    delete node3;
}

// Test General Functionality
TEST(OrderListTest, HandlesReverseGeneralUsage) {
    // Create queue
    PriorityQueue queue = PriorityQueue(true);

    // Create some nodes
    Node<int> node1 = Node(1);
    Node<int> node2 = Node(2);
    Node<int> node3 = Node(3);

    // Insert everything
    queue.insert(&node2);
    queue.insert(&node1);
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
TEST(OrderListTest, HandlesReverseConcurrency) {
    // Create queue
    PriorityQueue queue = PriorityQueue(true);

    // Create nodes
    Node<int>* node1 = new Node(1);
    Node<int>* node2 = new Node(2);
    Node<int>* node3 = new Node(3);

    // Create and start threads
    vector<thread> addThreads;
    addThreads.emplace_back(threadWorkerInsert, ref(queue), node2);
    addThreads.emplace_back(threadWorkerInsert, ref(queue), node1);
    addThreads.emplace_back(threadWorkerInsert, ref(queue), node3);

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
    for (int i = 0; i < 3; i++) {
        removeThreads.emplace_back(threadWorkerDelete, ref(queue));
    }

    // Wait for threads to finish
    for (auto& thread : removeThreads) {
        thread.join();
    }

    // Verify expected result
    EXPECT_EQ(queue.head.load(), nullptr);
    EXPECT_EQ(queue.tail.load(), nullptr);

    // Clear memory
    delete node1;
    delete node2;
    delete node3;
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}