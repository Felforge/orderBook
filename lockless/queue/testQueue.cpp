#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <optional>
#include <set>
#include "queue.h"
using namespace std;

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
        queue.pushLeft(1, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Push to left again
        queue.pushLeft(2, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Push to left again
        queue.pushLeft(3, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
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
        queue.pushLeft(1, pool);

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
        queue.pushLeft(3, pool);

        // Verify expected results
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
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

// Test Remove Node
TEST(LocklessQueueTest, HandlesRemoveNode) {
    // Create memory pool
    auto* pool = new MemoryPool<sizeof(Node<int>), 3>();

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // Construct queue
        Node<int>* node1 = queue.pushRight(1, pool);
        Node<int>* node2 = queue.pushRight(2, pool);
        Node<int>* node3 = queue.pushRight(3, pool);

        // Try removing head
        auto val = queue.removeNode(queue.head);

        // Verify expected result
        EXPECT_EQ(val, nullopt);

        // Try removing tail
        val = queue.removeNode(queue.tail);

        // Verify expected result
        EXPECT_EQ(val, nullopt);

        // Try removing nullptr
        val = queue.removeNode(nullptr);

        // Verify expected result
        EXPECT_EQ(val, nullopt);

        // Try contention case
        Node<int>* node = new Node<int>(nullptr, nullptr);
        val = queue.removeNode(node);

        // Verify expected result
        EXPECT_EQ(val, nullopt);

        // Clean up memory
        delete node;

        // Try removing middle node
        val = queue.removeNode(node2);

        // Verify expected result
        EXPECT_EQ(*val, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Add another node to the back
        Node<int>* node4 = queue.pushRight(4, pool);

        // Remove last node
        val = queue.removeNode(node4);
        
        // Verify expected result
        EXPECT_EQ(*val, 4);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Remove first node
        val = queue.removeNode(node1);

        // Verify expected result
        EXPECT_EQ(*val, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Remove final node
        val = queue.removeNode(node3);

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

// Test Pop Left, Pop Right, and Remove Node
TEST(LocklessQueueTest, HandlesRemoveCombination) {
    // Create memory pool
    auto* pool = new MemoryPool<sizeof(Node<int>), 4>();

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // Construct queue
        Node<int>* node1 = queue.pushRight(1, pool);
        Node<int>* node2 = queue.pushRight(2, pool);
        Node<int>* node3 = queue.pushRight(3, pool);
        Node<int>* node4 = queue.pushRight(4, pool);

        // Pop Right
        auto val = queue.popRight();

        // Verify expected result
        EXPECT_EQ(*val, 4);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Remove node2
        val = queue.removeNode(node2);

        // Verify expected result
        EXPECT_EQ(*val, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Push 5 to the right
        Node<int>* node5 = queue.pushRight(5, pool);

        // Pop Left
        val = queue.popLeft();

        // Verify expected result
        EXPECT_EQ(*val, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 5);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 3);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 5);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Push 6 to the left
        Node<int>* node6 = queue.pushLeft(6, pool);

        // Removde node3
        val = queue.removeNode(node3);

        // Verify expected result
        EXPECT_EQ(*val, 3);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 6);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 5);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 6);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 5);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Pop Right
        val = queue.popRight();

        // Verify expected result
        EXPECT_EQ(*val, 5);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr()->data, 6);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 6);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

        // Pop Left to empty the queue
        val = queue.popLeft();

        // Verify expected result
        EXPECT_EQ(*val, 6);
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
    }

    // Delete memory pool
    delete pool;
}

// Test For Correct Ref Counts
TEST(LocklessQueueTest, HandlesReferenceCounting) {
    // Create memory pool
    auto* pool = new MemoryPool<sizeof(Node<int>), 3>();

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        // Verify expected state
        EXPECT_EQ(queue.head->refCount, 1);
        EXPECT_EQ(queue.tail->refCount, 1);

        // Push To Left on Empty
        queue.pushLeft(1, pool);

        // Verify expected state
        EXPECT_EQ(queue.head->refCount, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->refCount, 1);

        // Empty the queue
        queue.popLeft();

        // Verify expected state
        EXPECT_EQ(queue.head->refCount, 1);
        EXPECT_EQ(queue.tail->refCount, 1);

        // Push three nodes to the right
        queue.pushRight(1, pool);
        Node<int>* node2 = queue.pushRight(2, pool);
        Node<int>* node3 = queue.pushRight(3, pool);

        // Verify expected state
        EXPECT_EQ(queue.head->refCount, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->refCount, 1);

        // Remove middle node
        queue.removeNode(node2);

        // Verify expected state
        EXPECT_EQ(queue.head->refCount, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->refCount, 1);

        // Add back a node to the right
        queue.pushRight(4, pool);

        // Verify expected state
        EXPECT_EQ(queue.head->refCount, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->refCount, 1);

        // Pop node from the right
        queue.popRight();

        // Verify expected state
        EXPECT_EQ(queue.head->refCount, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->refCount, 1);

        // Pop node from the left
        queue.popLeft();

        // Verify expected state
        EXPECT_EQ(queue.head->refCount, 1);
        EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
        EXPECT_EQ(queue.tail->refCount, 1);

        // Remove last node
        queue.removeNode(node3);

        // Verify expected state
        EXPECT_EQ(queue.head->refCount, 1);
        EXPECT_EQ(queue.tail->refCount, 1);
    }

    // Delete memory pool
    delete pool;
}

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
}

// Refcounts may be the problem

// Test Concurrent Popping
// Testing with 6 threads
TEST(LocklessQueueTest, HandlesConcurrentPopping) {
    // Reference constant to be used in testing
    const int N = 500;

    // Create memory pool vector
    // Each memory pool will have a capacity of N
    vector<MemoryPool<sizeof(Node<int>), N>*>pools(6);

    // Construct pools
    for (int i = 0; i < 6; i++) {
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
        for (int t = 0; t < 6; t++) {
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

        // Three threads popping from left
        for (int t = 0; t < 3; t++) {
            threads.emplace_back([&] {
                for(int i = 0; i < N; i++) {
                    auto val = queue.popLeft();

                    // Make sure return value is valid
                    EXPECT_NE(val, nullopt);
                }
            });
        }

        // Three threads popping from right
        for (int t = 0; t < 3; t++) {
            threads.emplace_back([&] {
                for(int i = 0; i < N; i++) {
                    auto val = queue.popRight();

                    // Make sure return value is valid
                    EXPECT_NE(val, nullopt);
                }
            });
        }

        // Wait for threads to finish
        for (auto& thread : threads) {
            thread.join();
        }

        // Verify that the queue is empty
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
    }

    // Delete Memory Pools
    for (auto pool: pools) {
        pool->~MemoryPool();
        delete pool;
    }
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
//     vector<vector<Node<int>*>> nodeVecs;

//     // Partition to allow the queue to destruct
//     // MemoryPool must be deleted after the queue
//     {
//         // Create queue
//         LocklessQueue<int> queue = LocklessQueue<int>();

//         // Add all nodes
//         for (int t = 0; t < 6; t++) {
//             // threads.emplace_back([&, t] {
//             //     for(int i = 0; i < N; i++) {
//             //         Node<int>* node = queue.pushLeft(t * N + i, pools[t]);

//             //         // Store node pointer
//             //         nodeVecs[t].push_back(node);
//             //     }
//             // });
//             vector<Node<int>*> nodes;
//             for(int i = 0; i < N; i++) {
//                 Node<int>* node = queue.pushLeft(t * N + i, pools[t]);

//                 cout << t * N + i << endl;

//                 // // Store node pointer
//                 // nodes.push_back(node);
//             }
//             nodeVecs.push_back(nodes);
//         }

//         cout << "done" << endl;

//         // // Wait for threads to finish
//         // for (auto& thread : threads) {
//         //     thread.join();
//         // }

//         // // Clear threads vector
//         // threads.clear();

//         // // Six threads removing their assigned nodes
//         // for (int t = 0; t < 6; t++) {
//         //     threads.emplace_back([&, t] {
//         //         for(int i = 0; i < N; i++) {
//         //             auto val = queue.removeNode(nodeVecs[t][i]);

//         //             // Make sure return value is valid
//         //             EXPECT_NE(val, nullopt);
//         //         }
//         //     });
//         // }

//         // // Wait for threads to finish
//         // for (auto& thread : threads) {
//         //     thread.join();
//         // }

//         // // Verify that the queue is empty
//         // EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//         // EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//         // EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//         // EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
//     }

//     // Delete Memory Pools
//     for (auto pool: pools) {
//         delete pool;
//     }
// }

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}