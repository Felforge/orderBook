#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <optional>
#include <set>
#include "queue.h"
using namespace std;

// WILL NEED TO TAKE ALL MEMORY POOLS AND TRACK WHERE EACH BLOCK BELONGS
// Maybe take all pools and put them into an unordered map
// Pair this with a marker for each block documenting who owns it

// Test Pushing to the Left
TEST(LocklessQueueTest, HandlesPushLeft) {
    // Create queue
    LocklessQueue<int> queue = LocklessQueue<int>();

    // Create memory pool
    MemoryPool<sizeof(int), 3> pool;

    // Push to left on empty queue
    void* block = pool.allocate();
    queue.pushLeft(1, &pool);

    // Verify expected results
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
    EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr(), queue.tail);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Push to left again
    block = pool.allocate();
    queue.pushLeft(2, &pool);

    // Verify expected results
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr()->data, 2);
    EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Push to left again
    block = pool.allocate();
    queue.pushLeft(3, &pool);

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

// // Test Pushing to the Right
// TEST(LocklessQueueTest, HandlesPushRight) {
//     // Create queue
//     LocklessQueue<int> queue = LocklessQueue<int>(3);

//     // Create memory pool
//     MemoryPool<sizeof(int), 3> pool;

//     // Push to left on empty queue
//     void* block = pool.allocate();
//     queue.pushRight(1, block);

//     // Verify expected results
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Push to left again
//     block = pool.allocate();
//     queue.pushRight(2, block);

//     // Verify expected results
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Push to left again
//     block = pool.allocate();
//     queue.pushRight(3, block);

//     // Verify expected results
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->next.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
// }

// // Test Pushing with Left and Right
// TEST(LocklessQueueTest, HandlesPushCombination) {
//     // Create queue
//     LocklessQueue<int> queue = LocklessQueue<int>(3);

//     // Create memory pool
//     MemoryPool<sizeof(int), 3> pool;

//     // Push to left on empty queue
//     void* block = pool.allocate();
//     queue.pushLeft(1, block);

//     // Verify expected results
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Push to left again
//     block = pool.allocate();
//     queue.pushRight(2, block);

//     // Verify expected results
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Push to left again
//     block = pool.allocate();
//     queue.pushLeft(3, block);

//     // Verify expected results
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->next.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->prev.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
// }

// // Test Pop Left
// TEST(LocklessQueueTest, HandlesPopLeft) {
//     // Create queue
//     LocklessQueue<int> queue = LocklessQueue<int>(3);

//     // Pop from empty queue
//     auto val = queue.popLeft();

//     // Verify expected result
//     EXPECT_EQ(val, nullopt);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//     EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Pop one element to empty queue
//     queue.pushRight(1);

//     // Pop element
//     val = queue.popLeft();

//     // Verify expected result
//     EXPECT_EQ(*val, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//     EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Pop three elements one by one to empty queue
//     queue.pushRight(1);
//     queue.pushRight(2);
//     queue.pushRight(3);

//     // Pop first element
//     val = queue.popLeft();

//     // Verify expected result
//     EXPECT_EQ(*val, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Pop second element
//     val = queue.popLeft();

//     // Verify expected result
//     EXPECT_EQ(*val, 2);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Pop third element
//     val = queue.popLeft();

//     // Verify expected result
//     EXPECT_EQ(*val, 3);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//     EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
// }

// // Test Pop Right
// TEST(LocklessQueueTest, HandlesPopRight) {
//     // Create queue
//     LocklessQueue<int> queue = LocklessQueue<int>(3);

//     // Pop from empty queue
//     auto val = queue.popRight();

//     // Verify expected result
//     EXPECT_EQ(val, nullopt);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//     EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Pop one element to empty queue
//     queue.pushRight(1);

//     // Pop element
//     val = queue.popRight();

//     // Verify expected result
//     EXPECT_EQ(*val, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//     EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Pop three elements one by one to empty queue
//     queue.pushRight(1);
//     queue.pushRight(2);
//     queue.pushRight(3);

//     // Pop first element
//     val = queue.popRight();

//     // Verify expected result
//     EXPECT_EQ(*val, 3);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Pop second element
//     val = queue.popRight();

//     // Verify expected result
//     EXPECT_EQ(*val, 2);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Pop third element
//     val = queue.popRight();

//     // Verify expected result
//     EXPECT_EQ(*val, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//     EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
// }

// // Test Remove Node
// TEST(LocklessQueueTest, HandlesRemoveNode) {
//     // Create queue
//     LocklessQueue<int> queue = LocklessQueue<int>(3);

//     // Construct queue
//     Node<int>* node1 = queue.pushRight(1);
//     Node<int>* node2 = queue.pushRight(2);
//     Node<int>* node3 = queue.pushRight(3);

//     // Try removing head
//     auto val = queue.removeNode(queue.head);

//     // Verify expected result
//     EXPECT_EQ(val, nullopt);

//     // Try removing tail
//     val = queue.removeNode(queue.tail);

//     // Verify expected result
//     EXPECT_EQ(val, nullopt);

//     // Try removing nullptr
//     val = queue.removeNode(nullptr);

//     // Verify expected result
//     EXPECT_EQ(val, nullopt);

//     // Try contention case
//     Node<int>* node = new Node<int>(nullptr);
//     val = queue.removeNode(node);

//     // Verify expected result
//     EXPECT_EQ(val, nullopt);

//     // Clean up memory
//     delete node;

//     // Try removing middle node
//     val = queue.removeNode(node2);

//     // Verify expected result
//     EXPECT_EQ(*val, 2);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Add another node to the back
//     Node<int>* node4 = queue.pushRight(4);

//     // Remove last node
//     val = queue.removeNode(node4);
    
//     // Verify expected result
//     EXPECT_EQ(*val, 4);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Remove first node
//     val = queue.removeNode(node1);

//     // Verify expected result
//     EXPECT_EQ(*val, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Remove final node
//     val = queue.removeNode(node3);

//     // Verify expected result
//     EXPECT_EQ(*val, 3);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//     EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
// }

// // Test Pop Left, Pop Right, and Remove Node
// TEST(LocklessQueueTest, HandlesRemoveCombination) {
//     // Create queue
//     LocklessQueue<int> queue = LocklessQueue<int>(4);

//     // Construct queue
//     Node<int>* node1 = queue.pushRight(1);
//     Node<int>* node2 = queue.pushRight(2);
//     Node<int>* node3 = queue.pushRight(3);
//     Node<int>* node4 = queue.pushRight(4);

//     // Pop Right
//     auto val = queue.popRight();

//     // Verify expected result
//     EXPECT_EQ(*val, 4);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->next.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Remove node2
//     val = queue.removeNode(node2);

//     // Verify expected result
//     EXPECT_EQ(*val, 2);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Push 5 to the right
//     Node<int>* node5 = queue.pushRight(5);

//     // Pop Left
//     val = queue.popLeft();

//     // Verify expected result
//     EXPECT_EQ(*val, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 5);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 3);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 5);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Push 6 to the left
//     Node<int>* node6 = queue.pushLeft(6);

//     // Removde node3
//     val = queue.removeNode(node3);

//     // Verify expected result
//     EXPECT_EQ(*val, 3);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 6);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 5);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 6);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 5);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Pop Right
//     val = queue.popRight();

//     // Verify expected result
//     EXPECT_EQ(*val, 5);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr()->data, 6);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 6);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

//     // Pop Left to empty the queue
//     val = queue.popLeft();

//     // Verify expected result
//     EXPECT_EQ(*val, 6);
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//     EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
// }

// // Test For Correct Ref Counts
// TEST(LocklessQueueTest, HandlesReferenceCounting) {
//     // Create queue
//     LocklessQueue<int> queue = LocklessQueue<int>(3);

//     // Verify expected state
//     EXPECT_EQ(queue.head->refCount, 1);
//     EXPECT_EQ(queue.tail->refCount, 1);

//     // Push To Left on Empty
//     queue.pushLeft(1);

//     // Verify expected state
//     EXPECT_EQ(queue.head->refCount, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->refCount, 1);

//     // Empty the queue
//     queue.popLeft();

//     // Verify expected state
//     EXPECT_EQ(queue.head->refCount, 1);
//     EXPECT_EQ(queue.tail->refCount, 1);

//     // Push three nodes to the right
//     queue.pushRight(1);
//     Node<int>* node2 = queue.pushRight(2);
//     Node<int>* node3 = queue.pushRight(3);

//     // Verify expected state
//     EXPECT_EQ(queue.head->refCount, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->refCount, 1);

//     // Remove middle node
//     queue.removeNode(node2);

//     // Verify expected state
//     EXPECT_EQ(queue.head->refCount, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->refCount, 1);

//     // Add back a node to the right
//     queue.pushRight(4);

//     // Verify expected state
//     EXPECT_EQ(queue.head->refCount, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->refCount, 1);

//     // Pop node from the right
//     queue.popRight();

//     // Verify expected state
//     EXPECT_EQ(queue.head->refCount, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->refCount, 1);

//     // Pop node from the left
//     queue.popLeft();

//     // Verify expected state
//     EXPECT_EQ(queue.head->refCount, 1);
//     EXPECT_EQ(queue.head->next.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->refCount, 2);
//     EXPECT_EQ(queue.tail->refCount, 1);

//     // Remove last node
//     queue.removeNode(node3);

//     // Verify expected state
//     EXPECT_EQ(queue.head->refCount, 1);
//     EXPECT_EQ(queue.tail->refCount, 1);
// }

// // Test Concurrent Pushing
// // Testing with 8 threads
// TEST(LocklessQueueTest, HandlesConcurrentPushing) {
//     // Reference constant to be used in testing
//     const int N = 1000;

//     // Queue will have a capacity of 10 times this reference constant
//     LocklessQueue<int> queue(8*N);

//     // Create vector to hold working threads
//     vector<thread> threads;

//     // pushLeft threads
//     for (int t = 0; t < 4; t++) {
//         threads.emplace_back([&, t] {
//             for(int i = 0; i < N; ++i) {
//                 queue.pushLeft(t * N + i);
//             }
//         });
//     }

//     // pushRight threads
//     for (int t = 5; t <= 8; t++) {
//         threads.emplace_back([&, t] {
//             for(int i = 0; i < N; ++i) {
//                 queue.pushRight(t * N + i);
//             }
//         });
//     }

//     // Wait for threads to finish
//     for (auto& thread : threads) {
//         thread.join();
//     }

//     // Create set to remember all seen elements
//     // sets guarentee all elements are unique
//     set<int> seen;

//     // Pop all elements
//     for (int i = 0; i < 8*N; ++i) {
//         // Pop and retrieve value
//         auto val = queue.popLeft();

//         // Make sure val is valid
//         EXPECT_TRUE(val.has_value());

//         // Insert val into set
//         seen.insert(*val);
//     }

//     // Verify expected size
//     EXPECT_EQ(seen.size(), 8*N);
// }

// To Remember:
// Use thread sanitizer
// Memory pool is the issue, fix it
// Maybe do thread local pools?

// // Test Concurrent Removing
// // Testing with 6 threads
// TEST(LocklessQueueTest, HandlesConcurrentRemoving) {
//     // Reference constant to be used in testing
//     const int N = 1000;

//     // Queue will have a capacity of 10 times this reference constant
//     LocklessQueue<int> queue(6*N);

//     // Create vector to hold working threads
//     vector<thread> threads;

//     // Create vector to hold nodes to be removed
//     vector<Node<int>*> nodes;

//     // Add all nodes
//     for (int i = 0; i <= 2; i++) {
//         for (int j = 1; j <= 2*N; j++) {
//             Node<int>* node = queue.pushRight(2*i*N + j);

//             // Store middle nodes for deletion
//             if (i == 1) {
//                 nodes.push_back(node);
//             }
//         }
//     }

//     // Two threads popping from left
//     for (int t = 0; t < 1; t++) {
//         threads.emplace_back([&] {
//             for(int i = 0; i < N; ++i) {
//                 auto val = queue.popLeft();

//                 // Make sure return value is valid
//                 EXPECT_NE(val, nullopt);
//             }
//         });
//     }

//     // // Two threads running removeNode on nodes
//     // for (int t = 0; t < 2; t++) {
//     //     threads.emplace_back([&] {
//     //         for (Node<int>* node: nodes) {
//     //             auto val = queue.removeNode(node);

//     //             // Make sure return value is valid
//     //             EXPECT_NE(val, nullopt);
//     //         }
//     //     });
//     // }

//     // Two threads popping from right
//     for (int t = 0; t < 1; t++) {
//         threads.emplace_back([&] {
//             for(int i = 0; i < N; ++i) {
//                 auto val = queue.popRight();

//                 // Make sure return value is valid
//                 EXPECT_NE(val, nullopt);
//             }
//         });
//     }

//     // Wait for threads to finish
//     for (auto& thread : threads) {
//         thread.join();
//     }

//     // Verify that the queue is empty
//     EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
//     EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
//     EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
// }

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}