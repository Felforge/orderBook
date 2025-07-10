#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <set>
#include <unordered_map>
#include <iostream>
#include "queue.h"

using namespace std;

// Test Concurrent Pushing - Exact copy from testQueue.cpp
TEST(LocklessQueueTest, HandlesConcurrentPushing) {
    cout << "Starting HandlesConcurrentPushing test..." << endl;
    
    // Reference constant to be used in testing
    const int N = 1000;

    // Create memory pool dict
    // Each memory pool will have a capacity of N
    unordered_map<int, MemoryPool<sizeof(Node<int>), N>*>pools;

    // Construct pools
    for (int i = 0; i < 8; i++) {
        pools[i] = new MemoryPool<sizeof(Node<int>), N>();
    }

    // Create vector to hold working threads
    vector<thread> threads;

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        cout << "Starting push phase..." << endl;
        auto push_start = chrono::high_resolution_clock::now();

        // pushLeft threads
        for (int t = 0; t < 4; t++) {
            threads.emplace_back([&, t] {
                cout << "PushLeft thread " << t << " starting..." << endl;
                for(int i = 0; i < N; ++i) {
                    queue.pushLeft(t * N + i, pools[t]);
                    if (i % 250 == 0) {
                        cout << "PushLeft thread " << t << " progress: " << i << "/" << N << endl;
                    }
                }
                cout << "PushLeft thread " << t << " finished." << endl;
            });
        }

        // pushRight threads
        for (int t = 4; t < 8; t++) {
            threads.emplace_back([&, t] {
                cout << "PushRight thread " << t << " starting..." << endl;
                for(int i = 0; i < N; ++i) {
                    queue.pushRight(t * N + i, pools[t]);
                    if (i % 250 == 0) {
                        cout << "PushRight thread " << t << " progress: " << i << "/" << N << endl;
                    }
                }
                cout << "PushRight thread " << t << " finished." << endl;
            });
        }

        // Wait for threads to finish
        cout << "Waiting for push threads to complete..." << endl;
        for (auto& thread : threads) {
            thread.join();
        }

        auto push_end = chrono::high_resolution_clock::now();
        auto push_duration = chrono::duration_cast<chrono::milliseconds>(push_end - push_start).count();
        cout << "Push phase completed in " << push_duration << "ms" << endl;

        // Create set to remember all seen elements
        // sets guarentee all elements are unique
        set<int> seen;

        cout << "Starting pop phase..." << endl;
        auto pop_start = chrono::high_resolution_clock::now();

        // Pop all elements
        for (int i = 0; i < 8*N; ++i) {
            // Pop and retrieve value
            auto val = queue.popLeft();

            // Make sure val is valid
            if (!val.has_value()) {
                cout << "ERROR: Pop " << i << " returned nullopt when it shouldn't!" << endl;
                FAIL() << "Pop returned nullopt unexpectedly at iteration " << i;
            }

            // Insert val into set
            seen.insert(*val);
            
            if ((i + 1) % 1000 == 0) {
                cout << "Popped " << (i + 1) << " items, unique: " << seen.size() << endl;
            }
        }

        auto pop_end = chrono::high_resolution_clock::now();
        auto pop_duration = chrono::duration_cast<chrono::milliseconds>(pop_end - pop_start).count();
        cout << "Pop phase completed in " << pop_duration << "ms" << endl;

        // Verify expected size
        EXPECT_EQ(seen.size(), 8*N);
        cout << "Final verification: seen.size() = " << seen.size() << ", expected = " << (8*N) << endl;
    }

    // Delete Memory Pools
    for (auto& [k, v]: pools) {
        delete v;
    }

    // Clear retire list
    retireList.clear();
    
    cout << "HandlesConcurrentPushing test completed successfully!" << endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}