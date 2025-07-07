#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "hazardPointers.h"
using namespace std;

// Test Setting and Clearing a Hazard Pointer on a single thread
TEST(LocklessMemoryPoolTest, HandlesSetAndClear) {
    // Declare a value to be tested
    int val = 1;

    // Set as hazard pointer
    EXPECT_NO_THROW(setHazardPointer(&val));

    // Verify expected result
    EXPECT_TRUE(isHazard(&val));

    // Remove protection from val
    EXPECT_NO_THROW(clearHazardPointer());

    // Verify expected result
    EXPECT_FALSE(isHazard(&val));
}

// Test Swapping Hazard Pointer
TEST(LocklessMemoryPoolTest, HandlesSwappingHazard) {
    // Declare values to be tested
    int val1 = 1;
    int val2 = 2;

    // Set as hazard pointer
    setHazardPointer(&val1);

    // Verify expected result
    EXPECT_TRUE(isHazard(&val1));
    EXPECT_FALSE(isHazard(&val2));

    // Swap hazard pointer to other value
    EXPECT_NO_THROW(setHazardPointer(&val2));

    // Verify expected result
    EXPECT_FALSE(isHazard(&val1));
    EXPECT_TRUE(isHazard(&val2));
}

// Test hazard pointer sharing between threads
// Testing with 1 external thread
TEST(LocklessMemoryPoolTest, HandlesSharedHazard) {
    // Declare a value to be tested
    int val = 1;

    // Set as hazard pointer on host
    setHazardPointer(&val);

    // Verify expected result
    EXPECT_TRUE(isHazard(&val));

    // Run external thread
    thread remote([&]() {
        // Verify expected result
        EXPECT_TRUE(isHazard(&val));

        // Set as hazard pointer
        setHazardPointer(&val);

        // Verify expected result
        EXPECT_TRUE(isHazard(&val));

        // Clear hazard pointer from this thread
        clearHazardPointer();

        // Verify expected result
        EXPECT_TRUE(isHazard(&val));
    });
    remote.join();

    // Clear hazard pointer from host
    clearHazardPointer();

    // Verify expected result
    EXPECT_FALSE(isHazard(&val));
}

// Test seperate thread hazards
// Testing with 1 external thread
TEST(LocklessMemoryPoolTest, HandlesSeperateHazard) {
    // Declare values to be tested
    int val1 = 1;
    int val2 = 2;

    // Set val1 as hazard pointer on host
    setHazardPointer(&val1);

    // Verify expected result
    EXPECT_TRUE(isHazard(&val1));
    EXPECT_FALSE(isHazard(&val2));

    // Create bool to control execution
    atomic<bool> host{false};

    // Launch external thread
    thread remote([&]() {
        // Verify expected result
        EXPECT_TRUE(isHazard(&val1));
        EXPECT_FALSE(isHazard(&val2));

        // Set val2 as hazard pointer on external thread
        setHazardPointer(&val2);

        // Verify expected result
        EXPECT_TRUE(isHazard(&val1));
        EXPECT_TRUE(isHazard(&val2));

        // Swap to host
        host.store(true);

        // While host is running yield the thread
        while (host.load()) {
            this_thread::yield();
        }

        // Verify expected result
        EXPECT_FALSE(isHazard(&val1));
        EXPECT_TRUE(isHazard(&val2));

        // Swap thread to val1
        setHazardPointer(&val1);

        // Verify expected result
        EXPECT_TRUE(isHazard(&val1));
        EXPECT_TRUE(isHazard(&val2));

        // Swap to host
        host.store(true);

        // While host is running yield the thread
        while (host.load()) {
            this_thread::yield();
        }

        // Verify expected result
        EXPECT_TRUE(isHazard(&val1));
        EXPECT_FALSE(isHazard(&val2));

        // Clear external thread protection
        clearHazardPointer();

        // Verify expected result
        EXPECT_FALSE(isHazard(&val1));
        EXPECT_FALSE(isHazard(&val2));

        // Swap to host
        host.store(true);
    });

    // While thread is running yield host
    while (!host.load()) {
        this_thread::yield();
    }

    // Verify expected result
    EXPECT_TRUE(isHazard(&val1));
    EXPECT_TRUE(isHazard(&val2));

    // Swap host protection
    setHazardPointer(&val2);

    // Verify expected result
    EXPECT_FALSE(isHazard(&val1));
    EXPECT_TRUE(isHazard(&val2));

    // Swap to thread
    host.store(false);

    // While thread is running yield host
    while (!host.load()) {
        this_thread::yield();
    }

    // Clear host protection
    clearHazardPointer();

    // Verify expected result
    EXPECT_TRUE(isHazard(&val1));
    EXPECT_FALSE(isHazard(&val2));

    // Swap to thread
    host.store(false);

    // While thread is running yield host
    while (!host.load()) {
        this_thread::yield();
    }

    // join thread
    remote.join();

    // Verify expected result
    EXPECT_FALSE(isHazard(&val1));
    EXPECT_FALSE(isHazard(&val2));
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}