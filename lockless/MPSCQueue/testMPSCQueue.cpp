#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <set>
#include "MPSCQueue.h"
using namespace std;

// Test Single Item
TEST(LocklessQueueTest, HandlesSingleItem) {
    // Create queue
    MPSCQueue<int, 2> queue;

    // Verify expected state
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());

    // Push an item
    int val = 1;
    queue.push(&val);

    // Verify expected state
    EXPECT_EQ(queue.buffer[0].load(), &val);
    EXPECT_FALSE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());

    // Pop the item
    int *result;
    queue.pop(result);

    // Verify expected state
    EXPECT_EQ(result, &val);
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());
}

// Test Full Capacity
TEST(LocklessQueueTest, HandlesFullCapacity) {
    // Create queue
    MPSCQueue<int, 2> queue;

    // Verify expected state
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());

    // Push two items
    int val1 = 1, val2 = 2;
    queue.push(&val1);
    queue.push(&val2);

    // Verify expected state
    EXPECT_EQ(queue.buffer[0].load(), &val1);
    EXPECT_EQ(queue.buffer[1].load(), &val1);
    EXPECT_FALSE(queue.isEmpty());
    EXPECT_TRUE(queue.isFull());

    // Pop the items
    int *result1, *result2;
    queue.pop(result1);
    queue.pop(result2);

    // Verify expected state
    EXPECT_EQ(result1, &val1);
    EXPECT_EQ(result2, &val2);
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());
}

// Test Concurrent Adding
// Testing with 8 threads
TEST(LocklessQueueTest, HandlesConcurrentAdding) {
    // Create queue
    MPSCQueue<int, 1024> queue;

    // Verify expected state
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());

    // Create vector to hold working threads
    vector<thread> threads;

    // Create vector of values to be pushed
    vector<int> vals;
    for (int i = 1; i <= 1024; i++) {
        vals.push_back(i);
    }

    // Run threads
    for (int t = 0; t < 8; t++) {
        threads.emplace_back([&, t] {
            for(int i = 0; i < 128; ++i) {
                queue.push(&vals[t * 128 + i - 1]);
            }
        });
    }

    // Wait for threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify expected state
    EXPECT_FALSE(queue.isEmpty());
    EXPECT_TRUE(queue.isFull());
}

// Test Concurrent Adding and Removing
// Testing with 5 threads
TEST(LocklessQueueTest, HandlesConcurrentAddRemove) {
    // Create queue
    MPSCQueue<int, 1024> queue;

    // Verify expected state
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());

    // Create vector to hold working threads
    vector<thread> threads;

    // Create vector of values to be pushed
    vector<int> vals;
    for (int i = 1; i <= 1024; i++) {
        vals.push_back(i);
    }

    // Run Add threads
    for (int t = 0; t < 8; t++) {
        threads.emplace_back([&, t] {
            for(int i = 0; i < 128; ++i) {
                queue.push(&vals[t * 128 + i - 1]);
            }
        });
    }

    // Use set to verify unique numbers
    // Sets guarentee all elements are unique
    set<int> seen;
    
    // Run Single Remove thread
    threads.emplace_back([&] {
        for(int i = 0; i < 1024; ++i) {
            int* result;
            queue.pop(result);
            seen.insert(*result);
        }
    });


    // Wait for threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify expected state
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());
    EXPECT_EQ(seen.size(), 1024);
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}