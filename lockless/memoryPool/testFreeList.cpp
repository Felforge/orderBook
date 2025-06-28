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

// Test Multiple Items
TEST(LocklessQueueTest, HandlesMultipleItems) {
    // Create free list
    FreeList<testStruct> freeList;

    // Verify expected state
    EXPECT_TRUE(freeList.isEmpty());

    // Add in four objects
    testStruct val1 = testStruct(1);
    freeList.push(&val1);

    testStruct val2 = testStruct(2);
    freeList.push(&val2);

    testStruct val3 = testStruct(3);
    freeList.push(&val3);

    testStruct val4 = testStruct(4);
    freeList.push(&val4);

    // Verify expected state
    EXPECT_FALSE(freeList.isEmpty());

    // Retrieve all objects and verify values
    testStruct* result = freeList.pop();
    EXPECT_EQ(result, &val4);

    result = freeList.pop();
    EXPECT_EQ(result, &val3);

    result = freeList.pop();
    EXPECT_EQ(result, &val2);

    result = freeList.pop();
    EXPECT_EQ(result, &val1);

    // Verify expected state
    EXPECT_TRUE(freeList.isEmpty());
}

// Test Push-Pop Combination
TEST(LocklessQueueTest, HandlesPushPopCombo) {
    // Create free list
    FreeList<testStruct> freeList;

    // Verify expected state
    EXPECT_TRUE(freeList.isEmpty());

    // Add in two objects
    testStruct val1 = testStruct(1);
    freeList.push(&val1);

    testStruct val2 = testStruct(2);
    freeList.push(&val2);

    // Verify expected state
    EXPECT_FALSE(freeList.isEmpty());

    // Retrieve top object and verify value
    testStruct* result = freeList.pop();
    EXPECT_EQ(result, &val2);

    // Verify expected state
    EXPECT_FALSE(freeList.isEmpty());

    // Add back object
    freeList.push(&val2);

    // Verify expected state
    EXPECT_FALSE(freeList.isEmpty());

    // Retrieve both objects and verify values
    result = freeList.pop();
    EXPECT_EQ(result, &val2);

    result = freeList.pop();
    EXPECT_EQ(result, &val1);

    // Verify expected state
    EXPECT_TRUE(freeList.isEmpty());
}

// Test Push Nullptr
TEST(LocklessQueueTest, HandlesPushNullptr) {
    // Create free list
    FreeList<testStruct> freeList;

    // Verify expected state
    EXPECT_TRUE(freeList.isEmpty());

    // Attempt to push nullptr
    EXPECT_THROW(freeList.push(nullptr), invalid_argument);
}

// Test Pop From Empty List
TEST(LocklessQueueTest, HandlesPopEmpty) {
    // Create free list
    FreeList<testStruct> freeList;

    // Verify expected state
    EXPECT_TRUE(freeList.isEmpty());

    // Attempt to remove from empty queue
    testStruct* result = freeList.pop();
    EXPECT_EQ(result, nullptr);
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}