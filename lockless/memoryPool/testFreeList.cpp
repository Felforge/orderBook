#include <gtest/gtest.h>
#include "freeList.h"
using namespace std;

// Test Single Item
TEST(LocklessQueueTest, HandlesSingleItem) {
    // Create free list
    FreeList<int> freeList;

    // Verify expected state
    EXPECT_TRUE(freeList.isEmpty());

    // Add in object
    int val = 1;
    freeList.push(&val);

    // Verify expected state
    EXPECT_FALSE(freeList.isEmpty());

    // Retrieve object
    int* result = freeList.pop();

    // Verify expected state
    EXPECT_EQ(result, &val);
    EXPECT_TRUE(freeList.isEmpty());
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}