#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "memoryPool.h"
using namespace std;

// Test Status
// Normal: PASSED
// ASAN: PASSED

string captureOutput(function<void()> func) {
    stringstream buffer;
    streambuf* old = cout.rdbuf(buffer.rdbuf());
    func();  // Run function that prints output
    cout.rdbuf(old);
    return buffer.str();
}

// Test Memory Pool Allocation and Deallocation
TEST(LocklessMemoryPoolTest, HandlesBasicUsage) {
    MemoryPool pool(64, 4);
    void* block = pool.allocate();

    // Check that pool
    EXPECT_FALSE(block == nullptr);

    // Fill block with 0xCD
    EXPECT_NO_THROW(memset(block, 0xCD, 64));

    EXPECT_NO_THROW(pool.deallocate(block));

    void* newBlock = pool.allocate();

    // Verify that memory location is reused
    EXPECT_EQ(block, newBlock);

    EXPECT_NO_THROW(pool.deallocate(newBlock));
}

// Test Memory Pool Exhaustion
TEST(LocklessMemoryPoolTest, HandlesPoolExhaustion) {
    MemoryPool pool(32, 2);
    void* block1 = pool.allocate();
    void* block2 = pool.allocate();

    // Make sure that exception is thrown
    EXPECT_THROW(pool.allocate(true), bad_alloc);

    pool.deallocate(block1);
    pool.deallocate(block2);
}

// Test Memory Pool Allignment
TEST(LocklessMemoryPoolTest, HandlesPoolAllignment) {
    MemoryPool pool(24, 3);
    void* block1 = pool.allocate();
    void* block2 = pool.allocate();

    // Check for memory allignment with 64 due to alignas(64)
    // uintpitr_t can safely hold a pointer's value as an integer
    EXPECT_EQ(reinterpret_cast<uintptr_t>(block1) % alignof(max_align_t), 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(block2) % alignof(max_align_t), 0);

    pool.deallocate(block1);
    pool.deallocate(block2);
}

// Test Memory Pool Full Reuse
TEST(LocklessMemoryPoolTest, HandlesFullReuse) {
    MemoryPool pool(64, 10);

    // Allocate blocks
    vector<void*> blocks;
    for (int i = 0; i < 10; ++i) {
        blocks.push_back(pool.allocate());
    }

    // Deallocate blocks
    for (void* block : blocks) {
        pool.deallocate(block);
    }

    // Reallocate blocks
    for (int i = 0; i < 10; ++i) {
        void* block = pool.allocate();
        EXPECT_FALSE(block == nullptr);
        blocks[i] = block;
    }

    // Clear memory
    for (void* block : blocks) {
        pool.deallocate(block);
    }
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}