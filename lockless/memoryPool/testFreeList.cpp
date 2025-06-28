#include <gtest/gtest.h>
#include "freeList.h"
using namespace std;

struct testStruct {
    int val;
    testStruct* next;
    testStruct(int val) : val(val), next(nullptr) {}
};

// Test Single Item
TEST(LocklessQueueTest, HandlesSingleItem) {
    // Create free list
    FreeList<testStruct> freeList;

    // Verify expected state
    EXPECT_TRUE(freeList.isEmpty());

    // Add in object
    testStruct val = testStruct(1);
    freeList.push(&val);

    // Verify expected state
    EXPECT_FALSE(freeList.isEmpty());

    // Retrieve object
    testStruct* result = freeList.pop();

    // Verify expected state
    EXPECT_EQ(result, &val);
    EXPECT_TRUE(freeList.isEmpty());
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}