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

    // Make sure remote remote free is empty
    EXPECT_TRUE(pool.isRemoteFreeEmpty());
    EXPECT_FALSE(pool.isRemoteFreeFull());

    // Attempt full reuse
    block1 = pool.allocate();
    block2 = pool.allocate();

    // Verify expected result
    EXPECT_NE(block1, nullptr);
    EXPECT_NE(block2, nullptr);
    EXPECT_NE(block1, block2);
}

// Test Memory Pool Owner Thread Deallocation
// Tetsing with 1 External Thread
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

        // Attempt first block deallocation
        pool.deallocate(block1);

        // Verify expected state
        EXPECT_FALSE(pool.isRemoteFreeEmpty());
        EXPECT_FALSE(pool.isRemoteFreeFull());

        // Attempt second block deallocation
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

// Test Memory Pool Concurrent Remote Deallocation
// Tetsing with 4 External Threads
TEST(LocklessMemoryPoolTest, HandlesConcurrentRemoteDealloc) {
    // Create Memory Pool
    MemoryPool<sizeof(int), 400> pool;

    // Allocate blocks
    vector<void*> blocks(400);
    for (int i = 0; i < 400; i++) {
        blocks[i] = pool.allocate();
    }

    // Make sure pool is full
    EXPECT_TRUE(pool.isDrained());

    // Create vector to hold working threads
    vector<thread> threads;

    // Attempt concurrent deallocation with 4 remote threads
    // Remote Free is being drained periodically
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 4; j++) {
            threads.emplace_back([&, i, j]() {
                for (int k = 0; k < 10; k++) {
                    pool.deallocate(blocks[40*i + 10*j + k]);
                }
            });
        }

        // Wait for threads to finish
        for (auto& thread : threads) {
            thread.join();
        }

        // Empty threads vector
        threads.clear();

        // Verify expected state
        EXPECT_FALSE(pool.isRemoteFreeEmpty());
        EXPECT_FALSE(pool.isRemoteFreeFull());

        // Drain remote free
        pool.drainRemoteFree();

        // Verify expected state
        EXPECT_TRUE(pool.isRemoteFreeEmpty());
        EXPECT_FALSE(pool.isRemoteFreeFull());
    }

    // Make sure pool is not full
    EXPECT_FALSE(pool.isDrained());

    // Make sure remote remote free is empty
    EXPECT_TRUE(pool.isRemoteFreeEmpty());
    EXPECT_FALSE(pool.isRemoteFreeFull());
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}