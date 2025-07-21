#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <optional>
#include <set>
#include <iostream>
#include <random>
#include <chrono>
#include <atomic>
#include "queue.h"
using namespace std;

// Test Status
// Normal: PASSED
// ASAN: PASSED
// TSAN: PASSED

// I was capped at five minutes due to memory constraints
// WIll likely do a longer duration test later

TEST(LocklessQueueSoakTest, HandlesFiveMinuteSoakTest) {
    cout << "Five Minute Soak Test Starting" << endl;

    const size_t TEST_DURATION = 5;
    const size_t NUM_THREADS = 8;
    const size_t POOL_SIZE = 7500000;
    
    vector<MemoryPool<sizeof(Node<int>), POOL_SIZE>*> pools(NUM_THREADS + 1);
    for (size_t i = 0; i <= NUM_THREADS; i++) {
        pools[i] = new MemoryPool<sizeof(Node<int>), POOL_SIZE>();
    }
    
    atomic<uint64_t> totalOperations{0};
    atomic<bool> stopFlag{false};
    
    {
        LocklessQueue<int> queue = LocklessQueue<int>();
        
        // Pre-populate to avoid empty queue edge cases
        for (int i = 0; i < POOL_SIZE; i++) {
            queue.pushLeft(i, pools[NUM_THREADS]);
        }
        
        // Store nodes per thread for removal operations
        vector<vector<Node<int>*>> threadNodes(NUM_THREADS);
        for (size_t i = 0; i < NUM_THREADS; i++) {
            threadNodes[i].reserve(10000);
        }
        
        vector<thread> threads;
        
        for (size_t threadId = 0; threadId < NUM_THREADS; threadId++) {
            threads.emplace_back([&, threadId]() {
                mt19937 rng(random_device{}() + threadId);
                uniform_int_distribution<int> opDist(0, 99);
                uniform_int_distribution<int> valueDist(0, 1000000);
                
                while (!stopFlag.load()) {
                    int op = opDist(rng);
                    int value = valueDist(rng);
                    
                    if (op < 35) {
                        // Push left (35%)
                        Node<int>* node = queue.pushLeft(value, pools[threadId]);
                        if (node && threadNodes[threadId].size() < 5000) {
                            threadNodes[threadId].push_back(node);
                        }
                    } else if (op < 70) {
                        // Push right (35%)
                        Node<int>* node = queue.pushRight(value, pools[threadId]);
                        if (node && threadNodes[threadId].size() < 5000) {
                            threadNodes[threadId].push_back(node);
                        }
                    } else if (op < 85) {
                        // Pop left (15%)
                        queue.popLeft();
                    } else if (op < 95) {
                        // Pop right (10%)
                        queue.popRight();
                    } else {
                        // Remove node (5%)
                        if (!threadNodes[threadId].empty()) {
                            uniform_int_distribution<size_t> indexDist(0, threadNodes[threadId].size() - 1);
                            size_t index = indexDist(rng);
                            Node<int>* nodeToRemove = threadNodes[threadId][index];
                            threadNodes[threadId][index] = threadNodes[threadId].back();
                            threadNodes[threadId].pop_back();
                            queue.removeNode(nodeToRemove);
                        }
                    }
                    
                    totalOperations.fetch_add(1);
                    
                    if (totalOperations.load() % 100000 == 0) {
                        this_thread::yield();
                    }
                }
            });
        }
        
        for (size_t i = 1; i <= TEST_DURATION; i++) {
            this_thread::sleep_for(chrono::minutes(1));
            cout << "Five Minute Soak Test Time: " << i << " min" << endl;
        }
        
        stopFlag.store(true);
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Verify queue is still functional
        Node<int>* testNode = queue.pushLeft(999, pools[0]);
        EXPECT_NE(testNode, nullptr);
        
        auto popResult = queue.popLeft();
        EXPECT_TRUE(popResult.has_value());
        if (popResult.has_value()) {
            EXPECT_EQ(*popResult, 999);
        }
        
        EXPECT_GT(totalOperations.load(), 1000000);
    }
    
    for (auto pool : pools) {
        delete pool;
    }
    
    retireList.clear();

    cout << "Five Minute Soak Test Completed" << endl;
}

TEST(LocklessQueueSoakTest, HandlesHighContentionTest) {
    cout << "High Contention Test Starting" << endl;

    const size_t TEST_DURATION = 5;
    const size_t NUM_THREADS = 8;
    const size_t POOL_SIZE = 7500000;
    
    vector<MemoryPool<sizeof(Node<int>), POOL_SIZE>*> pools(NUM_THREADS);
    for (size_t i = 0; i < NUM_THREADS; i++) {
        pools[i] = new MemoryPool<sizeof(Node<int>), POOL_SIZE>();
    }
    
    atomic<uint64_t> operations{0};
    atomic<bool> stopFlag{false};
    
    {
        LocklessQueue<int> queue = LocklessQueue<int>();
        
        vector<thread> threads;
        
        for (size_t i = 0; i < NUM_THREADS; i++) {
            threads.emplace_back([&, i]() {
                mt19937 rng(random_device{}() + i);
                
                while (!stopFlag.load()) {
                    for (int j = 0; j < 100 && !stopFlag.load(); j++) {
                        int op = rng() % 4;
                        switch (op) {
                            case 0:
                                queue.pushLeft(rng(), pools[i]);
                                break;
                            case 1:
                                queue.pushRight(rng(), pools[i]);
                                break;
                            case 2:
                                queue.popLeft();
                                break;
                            case 3:
                                queue.popRight();
                                break;
                        }
                        operations.fetch_add(1);
                    }
                    
                    this_thread::yield();
                }
            });
        }
        
        for (size_t i = 1; i <= TEST_DURATION; i++) {
            this_thread::sleep_for(chrono::minutes(1));
            cout << "High Contention Test Time: " << i << " min" << endl;
        }

        stopFlag.store(true);
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto result = queue.pushLeft(42, pools[0]);
        EXPECT_NE(result, nullptr);
        
        auto popResult = queue.popLeft();
        EXPECT_TRUE(popResult.has_value());
        if (popResult.has_value()) {
            EXPECT_EQ(*popResult, 42);
        }
        
        EXPECT_GT(operations.load(), 100000);
    }
    
    for (auto pool : pools) {
        delete pool;
    }

    retireList.clear();

    cout << "High Contention Test Completed" << endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}