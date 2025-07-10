#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <set>
#include <unordered_map>
#include "queue.h"

using namespace std;

int main() {
    // Match the exact conditions of HandlesConcurrentPushing
    const int N = 1000;
    const int NUM_THREADS = 8;
    
    cout << "Starting test matching HandlesConcurrentPushing conditions..." << endl;
    cout << "Threads: " << NUM_THREADS << ", Items per thread: " << N << ", Total: " << (NUM_THREADS * N) << endl;
    
    // Create memory pool dict like in the original test
    unordered_map<int, MemoryPool<sizeof(Node<int>), N>*> pools;
    for (int i = 0; i < NUM_THREADS; i++) {
        pools[i] = new MemoryPool<sizeof(Node<int>), N>();
    }
    
    // Test scope
    {
        LocklessQueue<int> queue;
        vector<thread> threads;
        atomic<int> itemsAdded{0};
        
        auto start = chrono::high_resolution_clock::now();
        
        cout << "Starting concurrent push phase..." << endl;
        
        // pushLeft threads (first 4 threads)
        for (int t = 0; t < 4; t++) {
            threads.emplace_back([&, t] {
                cout << "PushLeft thread " << t << " starting..." << endl;
                for(int i = 0; i < N; ++i) {
                    try {
                        queue.pushLeft(t * N + i, pools[t]);
                        int added = itemsAdded.fetch_add(1) + 1;
                        
                        if (i % 250 == 0) {
                            cout << "PushLeft thread " << t << " progress: " << i << "/" << N << " (total added: " << added << ")" << endl;
                        }
                    } catch (const exception& e) {
                        cout << "PushLeft thread " << t << " failed at item " << i << ": " << e.what() << endl;
                        return;
                    }
                }
                cout << "PushLeft thread " << t << " finished." << endl;
            });
        }
        
        // pushRight threads (last 4 threads)
        for (int t = 4; t < 8; t++) {
            threads.emplace_back([&, t] {
                cout << "PushRight thread " << t << " starting..." << endl;
                for(int i = 0; i < N; ++i) {
                    try {
                        queue.pushRight(t * N + i, pools[t]);
                        int added = itemsAdded.fetch_add(1) + 1;
                        
                        if (i % 250 == 0) {
                            cout << "PushRight thread " << t << " progress: " << i << "/" << N << " (total added: " << added << ")" << endl;
                        }
                    } catch (const exception& e) {
                        cout << "PushRight thread " << t << " failed at item " << i << ": " << e.what() << endl;
                        return;
                    }
                }
                cout << "PushRight thread " << t << " finished." << endl;
            });
        }
        
        cout << "Waiting for all push threads to complete..." << endl;
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto mid = chrono::high_resolution_clock::now();
        auto push_duration = chrono::duration_cast<chrono::milliseconds>(mid - start).count();
        
        cout << "Push phase completed in " << push_duration << "ms. Items added: " << itemsAdded.load() << endl;
        
        // Pop phase - exactly like the original test
        cout << "Starting sequential pop phase..." << endl;
        set<int> seen;
        auto pop_start = chrono::high_resolution_clock::now();
        
        for (int i = 0; i < NUM_THREADS * N; ++i) {
            auto val = queue.popLeft();
            
            if (val.has_value()) {
                seen.insert(*val);
                
                if ((i + 1) % 1000 == 0) {
                    cout << "Popped " << (i + 1) << " items, unique values: " << seen.size() << endl;
                }
            } else {
                cout << "ERROR: Failed to pop item " << i << " - queue empty unexpectedly!" << endl;
                break;
            }
        }
        
        auto end = chrono::high_resolution_clock::now();
        auto pop_duration = chrono::duration_cast<chrono::milliseconds>(end - pop_start).count();
        
        cout << "Pop phase completed in " << pop_duration << "ms." << endl;
        cout << "Final result: Unique values seen: " << seen.size() << " (expected: " << (NUM_THREADS * N) << ")" << endl;
        
        if (seen.size() == NUM_THREADS * N) {
            cout << "SUCCESS: All elements were correctly pushed and popped!" << endl;
        } else {
            cout << "ERROR: Mismatch in element count!" << endl;
        }
    }
    
    // Clean up
    for (auto& [k, v]: pools) {
        delete v;
    }
    
    retireList.clear();
    
    cout << "Test completed." << endl;
    return 0;
}