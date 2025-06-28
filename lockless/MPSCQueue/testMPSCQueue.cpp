#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <optional>
#include <set>
#include "MPSCQueue.h"
using namespace std;

// Test Single Item
TEST(LocklessQueueTest, HandlesSingleItem) {
    // Create queue
    MPSCQueue<int, 2> queue;

    // Push an item
    int val = 1;
    queue.push(&val);

    // Verify expected result
    EXPECT_EQ(queue.buffer[0].load(), &val);

    // Pop the item
    int *result;
    queue.pop(result);

    // Verify expected result
    EXPECT_EQ(result, &val);
    EXPECT_TRUE(queue.isEmpty());
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}