#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "memoryPool.h"
using namespace std;

struct testStruct {
    int val;
    testStruct* next;
    testStruct(int val) : val(val), next(nullptr) {}
};

// Test Memory Pool Allocation and Deallocation
TEST(LocklessMemoryPoolTest, HandlesBasicUsage) {
    // Create Memory Pool
    MemoryPool<sizeof(int), 2> pool;

    // Allocate both blocks
    void* block1 = pool.allocate();
    void* block2 = pool.allocate();

    // Verify expected result
    EXPECT_NE(block1, nullptr);
    EXPECT_NE(block2, nullptr);
    EXPECT_NE(block1, block2);

    // Check for bad alloc
    EXPECT_THROW(pool.allocate(), bad_alloc);

    // Deallocate both blocks
    pool.deallocate(block1);
    pool.deallocate(block2);

    // Attempt full reuse
    block1 = pool.allocate();
    block2 = pool.allocate();

    // Verify expected result
    EXPECT_NE(block1, nullptr);
    EXPECT_NE(block2, nullptr);
    EXPECT_NE(block1, block2);
}

// Test Memory Pool Owner Thread Deallocation
// Tetsing with 2 threads
TEST(LocklessMemoryPoolTest, HandlesOwnerThreadDealloc) {
    // Create Memory Pool
    MemoryPool<sizeof(int), 2> pool;

    // Allocate both blocks
    void* block1 = pool.allocate();
    void* block2 = pool.allocate();

    // Make sure pool is full
    EXPECT_TRUE(pool.isDrained());

    // Attempt deallocation with remote thread
    thread remote([&]() {
        // Verify expected result
        EXPECT_FALSE(pool.isOwnerThread());

        // Attempt block deallocation
        pool.deallocate(block1);
        pool.deallocate(block2);
    });
    remote.join();

    // Make sure pool is still full
    EXPECT_TRUE(pool.isDrained());

    // Make sure remote remote free is full
    EXPECT_FALSE(pool.isRemoteFreeEmpty());
    EXPECT_TRUE(pool.isRemoteFreeFull());

    // Drain remote free
    pool.drainRemoteFree();

    // Make sure remote remote free is empty
    EXPECT_TRUE(pool.isRemoteFreeEmpty());
    EXPECT_FALSE(pool.isRemoteFreeFull());
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}