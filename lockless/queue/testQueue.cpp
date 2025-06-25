#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <optional>
#include "queue.h"
using namespace std;

// Test Pushing to the Left
TEST(LocklessQueueTest, HandlesPushLeft) {
    // Create queue
    LocklessQueue<int> queue = LocklessQueue<int>(3);

    // Push to left on empty queue
    queue.pushLeft(1);

    // Verify expected results
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
    EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr(), queue.tail);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Push to left again
    queue.pushLeft(2);

    // Verify expected results
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr()->data, 2);
    EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 2);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Push to left again
    queue.pushLeft(3);

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

// Test Pushing to the Right
TEST(LocklessQueueTest, HandlesPushRight) {
    // Create queue
    LocklessQueue<int> queue = LocklessQueue<int>(3);

    // Push to left on empty queue
    queue.pushRight(1);

    // Verify expected results
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Push to left again
    queue.pushRight(2);

    // Verify expected results
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
    EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Push to left again
    queue.pushRight(3);

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

// Test Pushing with Left and Right
TEST(LocklessQueueTest, HandlesPushCombination) {
    // Create queue
    LocklessQueue<int> queue = LocklessQueue<int>(3);

    // Push to left on empty queue
    queue.pushLeft(1);

    // Verify expected results
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Push to left again
    queue.pushRight(2);

    // Verify expected results
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr()->data, 1);
    EXPECT_EQ(queue.head->next.load().getPtr()->next.load().getPtr()->data, 2);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->prev.load().getPtr()->data, 1);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->data, 2);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Push to left again
    queue.pushLeft(3);

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

// Test Pop Left
TEST(LocklessQueueTest, HandlesPopLeft) {
    // Create queue
    LocklessQueue<int> queue = LocklessQueue<int>(3);

    // Pop from empty queue
    auto val = queue.popLeft();

    // Verify expected result
    EXPECT_EQ(val, nullopt);
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
    EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Pop one element to empty queue
    queue.pushRight(1);

    // Pop element
    val = queue.popLeft();

    // Verify expected result
    EXPECT_EQ(*val, 1);
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
    EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Pop three elements one by one to empty queue
    queue.pushRight(1);
    queue.pushRight(2);
    queue.pushRight(3);

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

// Test Pop Right
TEST(LocklessQueueTest, HandlesPopRight) {
    // Create queue
    LocklessQueue<int> queue = LocklessQueue<int>(3);

    // Pop from empty queue
    auto val = queue.popRight();

    // Verify expected result
    EXPECT_EQ(val, nullopt);
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
    EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Pop one element to empty queue
    queue.pushRight(1);

    // Pop element
    val = queue.popRight();

    // Verify expected result
    EXPECT_EQ(*val, 1);
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
    EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Pop three elements one by one to empty queue
    queue.pushRight(1);
    queue.pushRight(2);
    queue.pushRight(3);

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

// Test Remove Node
TEST(LocklessQueueTest, HandlesRemoveNode) {
    // Create queue
    LocklessQueue<int> queue = LocklessQueue<int>(3);

    // Construct queue
    Node<int>* node1 = queue.pushRight(1);
    Node<int>* node2 = queue.pushRight(2);
    Node<int>* node3 = queue.pushRight(3);

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
    Node<int>* node4 = queue.pushRight(4);

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

// Test Pop Left, Pop Right, and Remove Node
TEST(LocklessQueueTest, HandlesRemoveCombination) {
    // Create queue
    LocklessQueue<int> queue = LocklessQueue<int>(4);

    // Construct queue
    Node<int>* node1 = queue.pushRight(1);
    Node<int>* node2 = queue.pushRight(2);
    Node<int>* node3 = queue.pushRight(3);
    Node<int>* node4 = queue.pushRight(4);

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
    Node<int>* node5 = queue.pushRight(5);

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
    Node<int>* node6 = queue.pushLeft(6);

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

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}