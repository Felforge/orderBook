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
    EXPECT_NO_THROW(removeHazardPointer(&val));

    // Verify expected result
    EXPECT_FALSE(isHazard(&val));

    // Clear hazard pointer maps
    for (auto &hazard: globalHazardPointers) {
        hazard.ptrs.clear();
    }
}

// Test Multiple Hazard Pointers
TEST(LocklessMemoryPoolTest, HandlesMultipleHazards) {
    // Declare values to be tested
    int val1 = 1;
    int val2 = 2;

    // Set as hazard pointer
    setHazardPointer(&val1);

    // Verify expected result
    EXPECT_TRUE(isHazard(&val1));
    EXPECT_FALSE(isHazard(&val2));

    // Swap hazard pointer on other value
    EXPECT_NO_THROW(setHazardPointer(&val2));

    // Verify expected result
    EXPECT_TRUE(isHazard(&val1));
    EXPECT_TRUE(isHazard(&val2));

    // Clear hazard pointer maps
    for (auto &hazard: globalHazardPointers) {
        hazard.ptrs.clear();
    }
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
        removeHazardPointer(&val);

        // Verify expected result
        EXPECT_TRUE(isHazard(&val));
    });
    remote.join();

    // Clear hazard pointer from host
    removeHazardPointer(&val);

    // Verify expected result
    EXPECT_FALSE(isHazard(&val));

    // Clear hazard pointer maps
    for (auto &hazard: globalHazardPointers) {
        hazard.ptrs.clear();
    }
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
        EXPECT_TRUE(isHazard(&val1));
        EXPECT_TRUE(isHazard(&val2));

        // Remove protection from val2
        removeHazardPointer(&val2);

        // Verify expected result
        EXPECT_TRUE(isHazard(&val1));
        EXPECT_TRUE(isHazard(&val2));

        // Protect val1 on thread
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
        removeHazardPointer(&val1);

        // Verify expected result
        EXPECT_TRUE(isHazard(&val1));
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

    // Host also protects val2
    setHazardPointer(&val2);

    // Verify expected result
    EXPECT_TRUE(isHazard(&val1));
    EXPECT_TRUE(isHazard(&val2));

    // Swap to thread
    host.store(false);

    // While thread is running yield host
    while (!host.load()) {
        this_thread::yield();
    }

    // Clear host protection
    removeHazardPointer(&val2);

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
    EXPECT_TRUE(isHazard(&val1));
    EXPECT_FALSE(isHazard(&val2));

    // Remove hazard pointer from val1
    removeHazardPointer(&val1);

    // Verify expected result
    EXPECT_FALSE(isHazard(&val1));
    EXPECT_FALSE(isHazard(&val2));

    // Clear hazard pointer maps
    for (auto &hazard: globalHazardPointers) {
        hazard.ptrs.clear();
    }
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}