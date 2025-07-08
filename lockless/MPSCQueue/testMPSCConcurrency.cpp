#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <random>
#include <unordered_set>
#include <iostream>
#include "MPSCQueue.h"
using namespace std;

// Test Status
// Normal:
// ASAN:
// TSAN:

// NUM_THREADS will be set to the maximum supported amount minus 1 consumer
int NUM_THREADS = thread::hardware_concurrency() - 1;

// Number of iterations each thread will perform
constexpr int ITERATIONS_PER_THREAD = 10000;

// Max capacity of the queue
// A consumer thread will be running at the same time as the producers
constexpr int QUEUE_CAPACITY = 1024;

// Each producer will push ITERATIONS_PER_THREAD unique pointers
void producer(MPSCQueue<int, QUEUE_CAPACITY>& queue, atomic<bool>& startFlag, vector<int*>& produced, int baseValue) {
    // Wait till startFlag is thrown
    while (!startFlag.load(memory_order_acquire)) {}

    // Push items
    for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
        // Create pointer
        int* ptr = new int(baseValue + i);

        // Push item
        queue.push(ptr);

        // Log what was produced
        produced.push_back(ptr);
    }
}

// Single consumer pops until all expected items are received
void consumer(MPSCQueue<int, QUEUE_CAPACITY>& queue, atomic<bool>& startFlag, atomic<int>& totalToConsume, vector<int*>& consumed) {
    // Wait till startFlag is thrown
    while (!startFlag.load(memory_order_acquire)) {}

    // Pop items
    int count = 0;
    while (count < totalToConsume.load()) {
        int* ptr = nullptr;
        if (queue.pop(ptr)) {
            consumed.push_back(ptr);
            count++;
        } else {
            // If queue is empty yield
            this_thread::yield();
        }
    }
}

TEST(MemoryPool, HandlesHeavyConcurrency) {
    // Create queue object
    MPSCQueue<int, QUEUE_CAPACITY> queue;

    // Create start flag to sync up threads
    atomic<bool> startFlag{false};

    // Count total items that much be consumed
    atomic<int> totalToConsume{NUM_THREADS * ITERATIONS_PER_THREAD};

    cout << totalToConsume.load() << endl;

    // To make sure produced and consumed items matched
    vector<vector<int*>> producerItems(NUM_THREADS);
    vector<int*> consumedItems;

    // Start consumer thread
    thread cons(consumer, ref(queue), ref(startFlag), ref(totalToConsume), ref(consumedItems));

    // Start producer threads
    vector<thread> producers;
    for (int i = 0; i < NUM_THREADS; i++) {
        producers.emplace_back(producer, ref(queue), ref(startFlag), ref(producerItems[i]), i * ITERATIONS_PER_THREAD);
    }

    cout << "launched" << endl;

    // Launch all threads at the same time
    startFlag.store(true, memory_order_release);

    // Wait for producers to finish
    for (auto& t : producers) {
        t.join();
    }

    cout << "produced" << endl;

    // Wait for consumer to finish
    cons.join();

    cout << "consumed" << endl;

    // Verification: All produced pointers should be consumed exactly once
    unordered_set<int*> produced_set;
    for (const auto& vec : producerItems)
        produced_set.insert(vec.begin(), vec.end());

    // To be used to make sure producedItems matched consumedItems
    unordered_set<int*> producedSet;
    for (const auto& vec : producerItems) {
        producedSet.insert(vec.begin(), vec.end());
    }
    unordered_set<int*> consumedSet(consumedItems.begin(), consumedItems.end());

    // Make sure the expected equalities hold
    EXPECT_EQ(producedSet.size(), NUM_THREADS * ITERATIONS_PER_THREAD);
    EXPECT_EQ(consumedSet.size(), NUM_THREADS * ITERATIONS_PER_THREAD);
    EXPECT_EQ(producedSet, consumedSet);

    // Clean up all pointers
    for (int* ptr : consumedItems) {
        delete ptr;
    }
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}