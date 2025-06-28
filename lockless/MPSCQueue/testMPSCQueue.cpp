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
    bool result = queue.push(&val);

    // Verify expected state
    EXPECT_TRUE(result);
    EXPECT_EQ(queue.buffer[0].load(), &val);
    EXPECT_FALSE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());

    // Pop the item
    int *num;
    result = queue.pop(num);

    // Verify expected state
    EXPECT_TRUE(result);
    EXPECT_EQ(num, &val);
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
    bool result1 = queue.push(&val1);
    bool result2 = queue.push(&val2);

    // Verify expected state
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_EQ(queue.buffer[0].load(), &val1);
    EXPECT_EQ(queue.buffer[1].load(), &val2);
    EXPECT_FALSE(queue.isEmpty());
    EXPECT_TRUE(queue.isFull());

    // Pop the items
    int *num1, *num2;
    result1 = queue.pop(num1);
    result2 = queue.pop(num2);

    // Verify expected state
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_EQ(num1, &val1);
    EXPECT_EQ(num2, &val2);
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
                queue.push(&vals[t * 128 + i]);
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

// Test Push To Full
TEST(LocklessQueueTest, HandlesPushToFull) {
    // Create queue
    MPSCQueue<int, 2> queue;

    // Verify expected state
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());

    // Fill Queue
    int val1 = 1, val2 = 2;
    queue.push(&val1);
    queue.push(&val2);

    // Verify expected state
    EXPECT_FALSE(queue.isEmpty());
    EXPECT_TRUE(queue.isFull());

    // Attempt to push to full queue
    int val3 = 3;
    bool result = queue.push(&val3);

    // Verify expected state
    EXPECT_FALSE(result);
    EXPECT_FALSE(queue.isEmpty());
    EXPECT_TRUE(queue.isFull());
}

// Test Pop From Empty
TEST(LocklessQueueTest, HandlesPopFromEmpty) {
    // Create queue
    MPSCQueue<int, 2> queue;

    // Verify expected state
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());

    // Attempt to pop from empty queue
    int* num = nullptr;
    bool result = queue.pop(num);

    // Verify expected state
    EXPECT_EQ(num, nullptr);
    EXPECT_FALSE(result);
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}