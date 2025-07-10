#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <set>
#include <iostream>
#include "queue.h"

using namespace std;

// Test Concurrent Popping
// Testing with 6 threads
TEST(LocklessQueueTest, HandlesConcurrentPopping) {
    cout << "Starting HandlesConcurrentPopping test..." << endl;
    
    // Reference constant to be used in testing
    const int N = 1000;

    // Create memory pool vector
    // Each memory pool will have a capacity of N
    vector<MemoryPool<sizeof(Node<int>), N>*>pools(6);

    // Construct pools
    for (int i = 0; i < 6; i++) {
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

        // Add all nodes
        for (int t = 0; t < 6; t++) {
            threads.emplace_back([&, t] {
                cout << "Push thread " << t << " starting..." << endl;
                for(int i = 0; i < N; i++) {
                    queue.pushLeft(t * N + i, pools[t]);
                    if (i % 250 == 0) {
                        cout << "Push thread " << t << " progress: " << i << "/" << N << endl;
                    }
                }
                cout << "Push thread " << t << " finished." << endl;
            });
        }

        // Wait for threads to finish
        for (auto& thread : threads) {
            thread.join();
        }

        auto push_end = chrono::high_resolution_clock::now();
        auto push_duration = chrono::duration_cast<chrono::milliseconds>(push_end - push_start).count();
        cout << "Push phase completed in " << push_duration << "ms" << endl;

        // Clear threads vector
        threads.clear();

        // Use atomic counter to track successful pops
        atomic<int> successfulPops{0};
        atomic<bool> queueExhausted{false};

        cout << "Starting concurrent pop phase..." << endl;
        auto pop_start = chrono::high_resolution_clock::now();

        // Three threads popping from left
        for (int t = 0; t < 3; t++) {
            threads.emplace_back([&, t] {
                cout << "PopLeft thread " << t << " starting..." << endl;
                for(int i = 0; i < N; i++) {
                    auto val = queue.popLeft();

                    // Make sure pop was valid
                    if (val.has_value()) {
                        int pops = successfulPops.fetch_add(1) + 1;
                        if (pops % 500 == 0) {
                            cout << "PopLeft thread " << t << " progress: successful pops = " << pops << endl;
                        }
                    } else {
                        // Queue is empty, stop trying
                        cout << "PopLeft thread " << t << " found empty queue at iteration " << i << endl;
                        queueExhausted.store(true);
                        break;
                    }
                }
                cout << "PopLeft thread " << t << " finished." << endl;
            });
        }

        // Three threads popping from right
        for (int t = 0; t < 3; t++) {
            threads.emplace_back([&, t] {
                cout << "PopRight thread " << t << " starting..." << endl;
                for(int i = 0; i < N; i++) {
                    auto val = queue.popRight();

                    // Make sure pop was valid
                    if (val.has_value()) {
                        int pops = successfulPops.fetch_add(1) + 1;
                        if (pops % 500 == 0) {
                            cout << "PopRight thread " << t << " progress: successful pops = " << pops << endl;
                        }
                    } else {
                        // Queue is empty, stop trying
                        cout << "PopRight thread " << t << " found empty queue at iteration " << i << endl;
                        queueExhausted.store(true);
                        break;
                    }
                }
                cout << "PopRight thread " << t << " finished." << endl;
            });
        }

        // Wait for threads to finish
        for (auto& thread : threads) {
            thread.join();
        }

        auto pop_end = chrono::high_resolution_clock::now();
        auto pop_duration = chrono::duration_cast<chrono::milliseconds>(pop_end - pop_start).count();
        cout << "Pop phase completed in " << pop_duration << "ms" << endl;

        // Verify that we popped all inserted elements
        EXPECT_EQ(successfulPops.load(), 6 * N);
        EXPECT_FALSE(queueExhausted.load());

        cout << "Results: " << successfulPops.load() << " successful pops, queue exhausted: " << queueExhausted.load() << endl;

        // Verify that the queue is empty
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
    }

    // Delete Memory Pools
    for (auto pool: pools) {
        delete pool;
    }

    // Clear retire list
    retireList.clear();
    
    cout << "HandlesConcurrentPopping test completed successfully!" << endl;
}

// Test Concurrent Removing
// Testing with 6 threads
TEST(LocklessQueueTest, HandlesConcurrentRemoving) {
    cout << "Starting HandlesConcurrentRemoving test..." << endl;
    
    // Reference constant to be used in testing
    const int N = 1000;

    // Create memory pool vector
    // Each memory pool will have a capacity of N
    vector<MemoryPool<sizeof(Node<int>), N>*>pools(6);

    // Construct pools
    for (int i = 0; i < 6; i++) {
        pools[i] = new MemoryPool<sizeof(Node<int>), N>();
    }

    // Create vector to hold working threads
    vector<thread> threads;

    // Create vectors to hold nodes
    vector<vector<Node<int>*>> nodeVecs(6);

    // Partition to allow the queue to destruct
    // MemoryPool must be deleted after the queue
    {
        // Create queue
        LocklessQueue<int> queue = LocklessQueue<int>();

        cout << "Starting push phase..." << endl;
        auto push_start = chrono::high_resolution_clock::now();

        // Add all nodes
        for (int t = 0; t < 6; t++) {
            // Pre-allocate to avoid reallocations
            nodeVecs[t].reserve(N);

            for(int i = 0; i < N; i++) {
                Node<int>* node = queue.pushLeft(t * N + i, pools[t]);
                nodeVecs[t].push_back(node);
            }
            
            if (t % 2 == 0) {
                cout << "Push phase: completed thread " << t << " with " << N << " nodes" << endl;
            }
        }

        auto push_end = chrono::high_resolution_clock::now();
        auto push_duration = chrono::duration_cast<chrono::milliseconds>(push_end - push_start).count();
        cout << "Push phase completed in " << push_duration << "ms" << endl;

        // Use atomic counter to track successful removals
        atomic<int> successfulRemovals{0};

        cout << "Starting concurrent remove phase..." << endl;
        auto remove_start = chrono::high_resolution_clock::now();

        // Six threads removing their assigned nodes
        for (int t = 0; t < 6; t++) {
            threads.emplace_back([&, t] {
                cout << "Remove thread " << t << " starting..." << endl;
                for(int i = 0; i < N; i++) {
                    Node<int>* nodeToRemove = nodeVecs[t][i];
                    auto val = queue.removeNode(nodeToRemove);

                    // Make sure removal is valid
                    if (val.has_value()) {
                        int removals = successfulRemovals.fetch_add(1) + 1;
                        
                        // Verify the removed value is correct
                        EXPECT_EQ(*val, t * N + i);
                        
                        if (removals % 500 == 0) {
                            cout << "Remove thread " << t << " progress: successful removals = " << removals << endl;
                        }
                    } else {
                        cout << "Remove thread " << t << " failed to remove node " << i << " (value " << (t * N + i) << ")" << endl;
                    }
                }
                cout << "Remove thread " << t << " finished." << endl;
            });
        }

        // Wait for threads to finish
        for (auto& thread : threads) {
            thread.join();
        }

        auto remove_end = chrono::high_resolution_clock::now();
        auto remove_duration = chrono::duration_cast<chrono::milliseconds>(remove_end - remove_start).count();
        cout << "Remove phase completed in " << remove_duration << "ms" << endl;

        // Verify that we removed all inserted elements
        EXPECT_EQ(successfulRemovals.load(), 6 * N);

        cout << "Results: " << successfulRemovals.load() << " successful removals (expected " << (6 * N) << ")" << endl;

        // Verify that the queue is empty
        EXPECT_EQ(queue.head->next.load().getPtr()->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.head->next.load().getPtr(), queue.tail);
        EXPECT_EQ(queue.tail->prev.load().getPtr(), queue.head);
        EXPECT_EQ(queue.tail->prev.load().getPtr()->next.load().getPtr(), queue.tail);
    }

    // Delete Memory Pools
    for (auto pool: pools) {
        delete pool;
    }

    // Clear retire list
    retireList.clear();
    
    cout << "HandlesConcurrentRemoving test completed successfully!" << endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}