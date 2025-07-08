#include <thread>
#include <vector>
#include <random>
#include <gtest/gtest.h>
#include "hazardPointers.h"
#include "hazardRetire.h"
using namespace std;

// Test Status
// Normal: PASSED
// ASAN: PASSED
// TSAN: PASSED

// Simple Node structure for demonstration in the test
struct Node {
    int value;
    Node(int v) : value(v) {}
};

// NUM_THREADS will be set to the maximum supported amount
int NUM_THREADS = thread::hardware_concurrency();

// Number of iterations each thread will perform (creating and retiring nodes)
constexpr int ITERATIONS_PER_THREAD = 1000000;

// Global counters for allocations and deallocations, for leak detection
std::atomic<int> totalAllocated{0};
std::atomic<int> totalFreed{0};

// Used to delete nodes
// Increements totalFreed
void deletionFunc(void* ptr) {
    delete static_cast<Node*>(ptr);
    totalFreed.fetch_add(1, memory_order_relaxed);
}

// Thread function: each thread creates, protects, retires, and unprotects nodes repeatedly
void hazardTestThread(int tid) {
    // Random number generator seeded differently per thread
    mt19937 rng(tid ^ random_device{}());
    uniform_int_distribution<> dist(1, 100);

    for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
        // With 80% probability, allocate/protect/retire/unprotect a node
        if (dist(rng) <= 80) {
            // Create node
            Node* node = new Node(i + tid * 100000);

            // increment totalAllocated
            totalAllocated.fetch_add(1, std::memory_order_relaxed);

            // Protect the node
            setHazardPointer(node);

            // Retire the node
            retireObj(node, deletionFunc);

            // Unprotect the pointer
            removeHazardPointer(node);
        } else {
            // Flush the retire list the other 20% of the time
            updateRetireList(deletionFunc);
        }
    }

    // After finishing delete everything that is remaining
    updateRetireList(deletionFunc);
}

// Test Heavy Concurrency
TEST(LocklessMemoryPoolTest, HandlesHeavyConcurrency) {
    // Reset the global counters
    totalAllocated.store(0);
    totalFreed.store(0);

    // Create vector to hold threads
    vector<thread> threads;

    // Launch all threads
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(hazardTestThread, i);
    }

    // Wait for all threads to finish   
    for (auto& thread : threads) {
        thread.join();
    }

    // The number of allocations and deallocations must match, or else there's a leak
    EXPECT_EQ(totalAllocated.load(), totalFreed.load()) << "Memory leak detected: not all nodes freed!";
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}