#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "hazardRetire.h"
using namespace std;

// Deletion function
// Just setting the memory to a distinct junk value
// Does not need to be deleted as it is a local variable
void deletionFunc(void* ptr) {
    memset(ptr, 0x69420, sizeof(int));
}

// Test Immediate Reclamation
TEST(LocklessMemoryPoolTest, HandlesImmediateReclamation) {
    // Declare a value to be tested
    int val = 1;

    // Check expected state
    EXPECT_EQ(val, 1);
    EXPECT_EQ(retireList.size(), 0);

    // Run retireObj on val
    retireObj(&val, deletionFunc);

    // Check expected state
    EXPECT_EQ(val, 1);
    EXPECT_EQ(retireList.size(), 1);

    // Clear retireList
    updateRetireList(deletionFunc);

    // Check expected state
    EXPECT_NE(val, 1);
    EXPECT_EQ(retireList.size(), 0);
}

// Test Delayed Reclamation
TEST(LocklessMemoryPoolTest, HandlesDelayedReclamation) {
    // Declare a value to be tested
    int val = 1;

    // Check expected state
    EXPECT_EQ(val, 1);
    EXPECT_EQ(retireList.size(), 0);
    EXPECT_FALSE(isHazard(&val));

    // Protect val
    setHazardPointer(&val);

    // Check expected state
    EXPECT_EQ(val, 1);
    EXPECT_EQ(retireList.size(), 0);
    EXPECT_TRUE(isHazard(&val));

    // Run retireObj on val
    retireObj(&val, deletionFunc);

    // Check expected state
    EXPECT_EQ(val, 1);
    EXPECT_EQ(retireList.size(), 1);
    EXPECT_TRUE(isHazard(&val));

    // Attempt to clear retireList
    updateRetireList(deletionFunc);

    // Check expected state
    EXPECT_EQ(val, 1);
    EXPECT_EQ(retireList.size(), 1);
    EXPECT_TRUE(isHazard(&val));

    // Clear protection from val
    clearHazardPointer();

    // Check expected state
    EXPECT_EQ(val, 1);
    EXPECT_EQ(retireList.size(), 1);
    EXPECT_FALSE(isHazard(&val));

    // Attempt to clear retireList
    updateRetireList(deletionFunc);

    // Check expected state
    EXPECT_NE(val, 1);
    EXPECT_EQ(retireList.size(), 0);
    EXPECT_FALSE(isHazard(&val));
}

// Test Cross-Thread Retire Lists
// Testing with 1 external thread
TEST(LocklessMemoryPoolTest, HandlesCrossThreadRetire) {
    // Declare value to be tested
    int val1 = 1;

    // Check expected state
    EXPECT_EQ(retireList.size(), 0);

    // Retire val1
    retireObj(&val1, deletionFunc);

    // Check expected state
    EXPECT_EQ(retireList.size(), 1);

    // Create bool to control execution
    atomic<bool> host{false};

    // Launch external thread
    thread remote([&]() {
        // Declare value to be tested
        int val2 = 2;

        // Check expected state
        EXPECT_EQ(retireList.size(), 0);

        // Retire val2
        retireObj(&val2, deletionFunc);

        // Check expected state
        EXPECT_EQ(retireList.size(), 1);

        // Swap to host
        host.store(true);

        // While host is running yield the thread
        while (host.load()) {
            this_thread::yield();
        }

        // Check expected state
        EXPECT_EQ(retireList.size(), 1);

        // Clear thread retire list
        updateRetireList(deletionFunc);

        // Check expected state
        EXPECT_EQ(retireList.size(), 0);

        // Swap to host
        host.store(true);
    });

    // While thread is running yield host
    while (!host.load()) {
        this_thread::yield();
    }

    // Check expected state
    EXPECT_EQ(retireList.size(), 1);

    // Clear host retire list
    updateRetireList(deletionFunc);

    // Check expected state
    EXPECT_EQ(retireList.size(), 0);

    // Swap to thread
    host.store(false);

    // While thread is running yield host
    while (!host.load()) {
        this_thread::yield();
    }

    // join thread
    remote.join();

    // Check expected state
    EXPECT_EQ(retireList.size(), 0);
}

// Test Retire List Overflow
TEST(LocklessMemoryPoolTest, HandlesRetireOverflow) {
    // Verify expected state
    EXPECT_EQ(retireList.size(), 0);

    // Create vector of values for testing
    vector<int> nums(64);
    for (int i = 0; i < 64; i++) {
        nums[i] = i;
    }

    // Retire values
    for (int i = 0; i < 64; i++) {
        // Verify expected state
        EXPECT_EQ(retireList.size(), i);

        // Retire vector index i
        retireObj(&nums[i], deletionFunc);
    }

    // Verify expected state
    // Retire list should have reached capacity (64) and cleared
    EXPECT_EQ(retireList.size(), 0);

    // Verify expected values
    // All should now be junk values
    for (int i = 0; i < 64; i++) {
        EXPECT_NE(nums[i], i);
    }
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}