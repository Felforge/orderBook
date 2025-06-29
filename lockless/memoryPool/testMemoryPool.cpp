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
    MemoryPool<sizeof(int), 2, 2> pool;

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


// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}