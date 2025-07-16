#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <optional>
#include <set>
#include <iostream>
#include <random>
#include "queue.h"
using namespace std;

int isHazardSize() {
    int count = 0;
    for (size_t i = 0; i < HAZARD_POINTERS_PER_THREAD; i++) {
        if (globalHazardPointers[hazardSlot].ptrs[i].load() != nullptr) {
            count++;
        }
    }
    return count;
}

// Test Pushing to the Left
TEST(LocklessQueueTest, HandlesPushLeft) {
    // Create memory pool
    auto* pool = new MemoryPool<sizeof(Node<int>), 3>();

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // Push to left on empty queue
        Node<int>* node1 = queue.pushLeft(1, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
        EXPECT_FALSE(isHazard(node1));

        // Push to left again
        Node<int>* node2 = queue.pushLeft(2, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
        EXPECT_FALSE(isHazard(node2));

        // Push to left again
        Node<int>* node3 = queue.pushLeft(3, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
        EXPECT_FALSE(isHazard(node3));
    }

    // Delete memory pool
    delete pool;
}

// Test Pushing to the Right
TEST(LocklessQueueTest, HandlesPushRight) {
    // Create memory pool
    auto* pool = new MemoryPool<sizeof(Node<int>), 3>();

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // Push to left on empty queue
        queue.pushRight(1, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Push to left again
        queue.pushRight(2, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Push to left again
        queue.pushRight(3, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
    }

    // Delete memory pool
    delete pool;
}

// Test Pushing with Left and Right
TEST(LocklessQueueTest, HandlesPushCombination) {
    // Create memory pool
    auto* pool = new MemoryPool<sizeof(Node<int>), 3>();

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // Push to left on empty queue
        Node<int>* node1 = queue.pushLeft(1, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
        EXPECT_FALSE(isHazard(node1));

        // Push to left again
        Node<int>* node2 = queue.pushRight(2, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
        EXPECT_FALSE(isHazard(node2));

        // Push to left again
        Node<int>* node3 = queue.pushLeft(3, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
        EXPECT_FALSE(isHazard(node3));
    }

    // Delete memory pool
    delete pool;
}

// Test Pop Left
TEST(LocklessQueueTest, HandlesPopLeft) {
    // Create memory pool
    auto* pool = new MemoryPool<sizeof(Node<int>), 3>();

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // Pop from empty queue
        auto val = queue.popLeft();

        // Verify expected result
        EXPECT_EQ(val, nullopt);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Pop one element to empty queue
        queue.pushRight(1, pool);

        // Pop element
        val = queue.popLeft();

        // Verify expected result
        EXPECT_EQ(*val, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Pop three elements one by one to empty queue
        queue.pushRight(1, pool);
        queue.pushRight(2, pool);
        queue.pushRight(3, pool);

        // Pop first element
        val = queue.popLeft();

        // Verify expected result
        EXPECT_EQ(*val, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Pop second element
        val = queue.popLeft();

        // Verify expected result
        EXPECT_EQ(*val, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Pop third element
        val = queue.popLeft();

        // Verify expected result
        EXPECT_EQ(*val, 3);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
    }

    // Delete memory pool
    delete pool;
}

// Test Pop Right
TEST(LocklessQueueTest, HandlesPopRight) {
    // Create memory pool
    auto* pool = new MemoryPool<sizeof(Node<int>), 3>();

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // Pop from empty queue
        auto val = queue.popRight();

        // Verify expected result
        EXPECT_EQ(val, nullopt);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Pop one element to empty queue
        queue.pushRight(1, pool);

        // Pop element
        val = queue.popRight();

        // Verify expected result
        EXPECT_EQ(*val, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Pop three elements one by one to empty queue
        queue.pushRight(1, pool);
        queue.pushRight(2, pool);
        queue.pushRight(3, pool);

        // Pop first element
        val = queue.popRight();

        // Verify expected result
        EXPECT_EQ(*val, 3);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Pop second element
        val = queue.popRight();

        // Verify expected result
        EXPECT_EQ(*val, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Pop third element
        val = queue.popRight();

        // Verify expected result
        EXPECT_EQ(*val, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
    }

    // Delete memory pool
    delete pool;
}

// // Test Remove Node
// TEST(LocklessQueueTest, HandlesRemoveNode) {
//     // Create memory pool
//     auto* pool = new MemoryPool<sizeof(Node<int>), 3>();

//     // Partition to allow the queue to destruct
//     // MemoryPool must be deleted after the queue
//     {
//         // Create queue
//         LocklessQueue<int> queue = LocklessQueue<int>();

//         // Construct queue
//         Node<int>* node1 = queue.pushRight(1, pool);
//         Node<int>* node2 = queue.pushRight(2, pool);
//         Node<int>* node3 = queue.pushRight(3, pool);

//         // Try removing head
//         auto val = queue.removeNode(queue.head);

//         // Verify expected result
//         EXPECT_EQ(val, nullopt);

//         // Try removing tail
//         val = queue.removeNode(queue.tail);

//         // Verify expected result
//         EXPECT_EQ(val, nullopt);

//         // Try removing nullptr
//         val = queue.removeNode(nullptr);

//         // Verify expected result
//         EXPECT_EQ(val, nullopt);

//         // Try contention case
//         Node<int>* node = new Node<int>(nullptr, nullptr);
//         val = queue.removeNode(node);

//         // Verify expected result
//         EXPECT_EQ(val, nullopt);

//         // Clean up memory
//         delete node;

//         // Try removing middle node
//         val = queue.removeNode(node2);

//         // Verify expected result
//         EXPECT_EQ(*val, 2);
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//         EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//         // Add another node to the back
//         Node<int>* node4 = queue.pushRight(4, pool);

//         // Remove last node
//         val = queue.removeNode(node4);
        
//         // Verify expected result
//         EXPECT_EQ(*val, 4);
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//         EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//         // Remove first node
//         val = queue.removeNode(node1);

//         // Verify expected result
//         EXPECT_EQ(*val, 1);
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//         // Remove final node
//         val = queue.removeNode(node3);

//         // Verify expected result
//         EXPECT_EQ(*val, 3);
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//         EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
//     }

//     // Delete memory pool
//     delete pool;

//     // Clear retire list
//     retireList.clear();
// }

// // Test Pop Left, Pop Right, and Remove Node
// TEST(LocklessQueueTest, HandlesRemoveCombination) {
//     // Create memory pool
//     auto* pool = new MemoryPool<sizeof(Node<int>), 4>();

//     // Partition to allow the queue to destruct
//     // MemoryPool must be deleted after the queue
//     {
//         // Create queue
//         LocklessQueue<int> queue = LocklessQueue<int>();

//         // Construct queue
//         Node<int>* node1 = queue.pushRight(1, pool);
//         Node<int>* node2 = queue.pushRight(2, pool);
//         Node<int>* node3 = queue.pushRight(3, pool);
//         Node<int>* node4 = queue.pushRight(4, pool);

//         // Pop Right
//         auto val = queue.popRight();

//         // Verify expected result
//         EXPECT_EQ(*val, 4);
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//         EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
//         EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->next.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//         // Remove node2
//         val = queue.removeNode(node2);

//         // Verify expected result
//         EXPECT_EQ(*val, 2);
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//         EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//         // Push 5 to the right
//         Node<int>* node5 = queue.pushRight(5, pool);

//         // Pop Left
//         val = queue.popLeft();

//         // Verify expected result
//         EXPECT_EQ(*val, 1);
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 5);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 3);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 5);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//         // Push 6 to the left
//         Node<int>* node6 = queue.pushLeft(6, pool);

//         // Remove node3
//         val = queue.removeNode(node3);

//         // Verify expected result
//         EXPECT_EQ(*val, 3);
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr()->data, 6);
//         EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 5);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 6);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 5);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//         // Pop Right
//         val = queue.popRight();

//         // Verify expected result
//         EXPECT_EQ(*val, 5);
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr()->data, 6);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 6);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//         // Pop Left to empty the queue
//         val = queue.popLeft();

//         // Verify expected result
//         EXPECT_EQ(*val, 6);
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//         EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
//     }

//     // Delete memory pool
//     delete pool;

//     // Clear retire list
//     retireList.clear();
// }

// Test Concurrent Pushing
// Testing with 8 threads
TEST(LocklessQueueTest, HandlesConcurrentPushing) {
    // Reference constant to be used in testing
    const int N = 1000;

    // Create memory pool dict
    // Each memory pool will have a capacity of N
    unordered_map<int, MemoryPool<sizeof(Node<int>), N>*>pools;

    // Construct pools
    for (int i = 0; i < 8; i++) {
        pools[i] = new MemoryPool<sizeof(Node<int>), N>();
    }

    // Create vector to hold working threads
    vector<thread> threads;

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // pushLeft threads
        for (int t = 0; t < 4; t++) {
            threads.emplace_back([&, t] {
                for(int i = 0; i < N; ++i) {
                    queue.pushLeft(t * N + i, pools[t]);
                }
            });
        }

        // pushRight threads
        for (int t = 4; t < 8; t++) {
            threads.emplace_back([&, t] {
                for(int i = 0; i < N; ++i) {
                    queue.pushRight(t * N + i, pools[t]);
                }
            });
        }

        // Wait for threads to finish
        for (auto& thread : threads) {
            thread.join();
        }

        // Create set to remember all seen elements
        // sets guarentee all elements are unique
        set<int> seen;

        // Pop all elements
        for (int i = 0; i < 8*N; ++i) {
            // Pop and retrieve value
            auto val = queue.popLeft();

            // Make sure val is valid
            EXPECT_TRUE(val.has_value());

            // Insert val into set
            seen.insert(*val);
        }

        // Verify expected size
        EXPECT_EQ(seen.size(), 8*N);
    }

    // Delete Memory Pools
    for (auto& [k, v]: pools) {
        delete v;
    }

    // Clear retire list
    retireList.clear();
}

// Test Concurrent Popping
// Testing with 8 threads
TEST(LocklessQueueTest, HandlesConcurrentPopping) {
    // Reference constant to be used in testing
    const int N = 1000;

    // Create memory pool vector
    // Each memory pool will have a capacity of N
    vector<MemoryPool<sizeof(Node<int>), N>*>pools(8);

    // Construct pools
    for (int i = 0; i < 8; i++) {
        pools[i] = new MemoryPool<sizeof(Node<int>), N>();
    }

    // Create vector to hold working threads
    vector<thread> threads;

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // Add all nodes
        for (int t = 0; t < 8; t++) {
            threads.emplace_back([&, t] {
                for(int i = 0; i < N; i++) {
                    queue.pushLeft(t * N + i, pools[t]);
                }
            });
        }

        // Wait for threads to finish
        for (auto& thread : threads) {
            thread.join();
        }

        // Clear threads vector
        threads.clear();

        // Use atomic counter to track successful pops
        atomic<int> successfulPops{0};
        atomic<bool> queueExhausted{false};

        // Four threads popping from left
        for (int t = 0; t < 4; t++) {
            threads.emplace_back([&] {
                for(int i = 0; i < N; i++) {
                    auto val = queue.popLeft();

                    // Make sure pop was valid
                    if (val.has_value()) {
                        successfulPops.fetch_add(1);
                    } else {
                        // Queue is empty, stop trying
                        queueExhausted.store(true);
                        break;
                    }
                }
            });
        }

        // Four threads popping from right
        for (int t = 0; t < 4; t++) {
            threads.emplace_back([&] {
                for(int i = 0; i < N; i++) {
                    auto val = queue.popRight();

                    // Make sure pop was valid
                    if (val.has_value()) {
                        successfulPops.fetch_add(1);
                    } else {
                        // Queue is empty, stop trying
                        queueExhausted.store(true);
                        break;
                    }
                }
            });
        }

        // Wait for threads to finish
        for (auto& thread : threads) {
            thread.join();
        }

        // Now check that all hazard pointers are cleaned up
        EXPECT_EQ(isHazardSize(), 0);

        // Verify that we popped all inserted elements
        EXPECT_EQ(successfulPops.load(), 8 * N);
        EXPECT_FALSE(queueExhausted.load());

        // Verify that the queue is empty
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
    }

    // Delete Memory Pools
    for (auto pool: pools) {
        delete pool;
    }

    // Clear retire list
    retireList.clear();
}

// // Test Concurrent Removing
// // Testing with 6 threads
// TEST(LocklessQueueTest, HandlesConcurrentRemoving) {
//     // Reference constant to be used in testing
//     const int N = 1000;

//     // Create memory pool vector
//     // Each memory pool will have a capacity of N
//     vector<MemoryPool<sizeof(Node<int>), N>*>pools(6);

//     // Construct pools
//     for (int i = 0; i < 6; i++) {
//         pools[i] = new MemoryPool<sizeof(Node<int>), N>();
//     }

//     // Create vector to hold working threads
//     vector<thread> threads;

//     // Create vectors to hold nodes
//     vector<vector<Node<int>*>> nodeVecs(6);

//     // Partition to allow the queue to destruct
//     // MemoryPool must be deleted after the queue
//     {
//         // Create queue
//         LocklessQueue<int> queue = LocklessQueue<int>();

//         // Add all nodes
//         for (int t = 0; t < 6; t++) {
//             // Pre-allocate to avoid reallocations
//             nodeVecs[t].reserve(N);

//             for(int i = 0; i < N; i++) {
//                 Node<int>* node = queue.pushLeft(t * N + i, pools[t]);
//                 nodeVecs[t].push_back(node);
//             }
//         }

//         // Use atomic counter to track successful removals
//         atomic<int> successfulRemovals{0};

//         // Six threads removing their assigned nodes
//         for (int t = 0; t < 6; t++) {
//             threads.emplace_back([&, t] {
//                 for(int i = 0; i < N; i++) {
//                     Node<int>* nodeToRemove = nodeVecs[t][i];
//                     auto val = queue.removeNode(nodeToRemove);

//                     // Make sure removal is valid
//                     if (val.has_value()) {
//                         successfulRemovals.fetch_add(1);

//                         // Verify the removed value is correct
//                         EXPECT_EQ(*val, t * N + i);
//                     }
//                 }
//             });
//         }

//         // Wait for threads to finish
//         for (auto& thread : threads) {
//             thread.join();
//         }

//         // Verify that we removed all inserted elements
//         EXPECT_EQ(successfulRemovals.load(), 6 * N);

//         // Verify that the queue is empty
//         EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//         EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//         EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
//     }

//     // Delete Memory Pools
//     for (auto pool: pools) {
//         delete pool;
//     }

//     // Clear retire list
//     retireList.clear();
// }

// Test Concurrent Combination of Push and Pop
// Basically a mini version of the soak I'll do later
// Testing with 4 threads
TEST(LocklessQueueTest, HandlesConcurrentCombination) {
    // Estimate memory pool size
    const size_t poolSize = 1000000;

    // Create memory pool vector
    // Memory pool size is an instance
    vector<MemoryPool<sizeof(Node<int>), poolSize>*>pools(5);

    // Construct pools
    for (int i = 0; i < 5; i++) {
        pools[i] = new MemoryPool<sizeof(Node<int>), poolSize>();
    }

    // Create vector to hold working threads
    vector<thread> threads;

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // Populate queue
        for (int i = 0; i < 1000000; i++) {
            queue.pushLeft(i, pools[4]);
        }

        // Launch threads
        atomic<bool> stop = false;

        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&, i]() {
                mt19937 rng(random_device{}());
                
                while (!stop) {
                    int op = rng() % 4;

                    switch (op) {
                        case 0: 
                            queue.pushLeft(rng(), pools[i]); 
                            break;

                        case 1: 
                            queue.pushRight(rng(), pools[i]); 
                            break;

                        case 2: 
                            queue.popLeft(); 
                            break;

                        case 3: 
                            queue.popRight(); 
                            break;
                    }
                }
            });
        }

        // Wait for 1 second
        this_thread::sleep_for(chrono::seconds(1));

        // Stop threads
        stop.store(true);

        // Wait for threads to finish
        for (auto& thread : threads) {
            thread.join();
        }
    }

    // Delete Memory Pools
    for (auto pool: pools) {
        delete pool;
    }
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}