#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <random>
#include "memoryPool.h"
using namespace std;

// Test Status
// Normal: PASSED
// ASAN: PASSED
// TSAN: PASSED

// NUM_THREADS will be set to the maximum supported amount
int NUM_THREADS = thread::hardware_concurrency();

// Number of iterations each thread will perform
constexpr int ITERATIONS_PER_THREAD = 1000000;

// Self explanatory
constexpr size_t BLOCK_SIZE = 128;
constexpr size_t BLOCK_COUNT = 256;

// Each thread will allocate and deallocate objects from the pool
void threadFunc(MemoryPool<BLOCK_SIZE, BLOCK_COUNT>& pool, std::atomic<bool>& startFlag, std::atomic<size_t>& allocCount) {
    // Wait for all threads to be ready
    while (!startFlag.load(std::memory_order_acquire)) {}

    // To save local allocations
    vector<void*> localAllocs;

    // Random tool with random seed
    mt19937 rng(random_device{}());
    uniform_int_distribution<int> dist(0, 1);

    for (size_t i = 0; i < ITERATIONS_PER_THREAD; ++i) {
        // 50%: alloc, 50%: free (if any to free)
        if (localAllocs.empty() || dist(rng) == 0) {
            // Try to allocate
            try {
                void* ptr = pool.allocate();
                localAllocs.push_back(ptr);
                allocCount.fetch_add(1, memory_order_relaxed);
            } catch (const bad_alloc&) {
                // Pool exhausted, will try to free on next turn
            }
        } else {
            // Deallocate one (random, possibly to simulate remote free)
            size_t idx = dist(rng) % localAllocs.size();
            void* ptr = localAllocs[idx];
            pool.deallocate(ptr);
            localAllocs.erase(localAllocs.begin() + idx);
        }
    }

    // Free any remaining allocations
    for (void* p : localAllocs) {
        pool.deallocate(p);
    }
}

TEST(MemoryPool, HandlesHeavyConcurrency) {
    // Create memory pool dict
    // Each memory pool will have a capacity of N
    unordered_map<size_t, MemoryPool<BLOCK_SIZE, BLOCK_COUNT>*>pools;

    // Construct pools
    for (size_t i = 0; i < NUM_THREADS; i++) {
        pools[i] = new MemoryPool<BLOCK_SIZE, BLOCK_COUNT>();
    }

    // Used to start the threads at the same time
    atomic<bool> startFlag{false};

    // Keep track of the total number of allocations
    atomic<size_t> totalAllocs{0};

    // Hold all used threads
    vector<thread> threads;

    for (size_t i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(threadFunc, std::ref(*pools[i]), std::ref(startFlag), std::ref(totalAllocs));
    }

    // Start all threads simultaneously
    startFlag.store(true, memory_order_release);
    
    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }

    // Drain all remote frees
    for (size_t i = 0; i < NUM_THREADS; i++) {
        pools[i]->drainRemoteFree();
    }

    for (size_t i = 0; i < NUM_THREADS; i++) {
        // After all operations, all pools should be completely drained
        EXPECT_TRUE(pools[i]->isRemoteFreeEmpty());
        EXPECT_FALSE(pools[i]->isRemoteFreeFull());

        // Should have all blocks back in freeList
        EXPECT_FALSE(pools[i]->isDrained());
    }
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}