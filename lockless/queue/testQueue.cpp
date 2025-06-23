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
    EXPECT_EQ(val, std::nullopt);
    EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
    EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
    EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);

    // Pop one element to empty queue
    queue.pushLeft(1);

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

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}