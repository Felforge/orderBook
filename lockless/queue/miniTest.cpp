#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "queue.h"

using namespace std;

int main() {
    // Scale up to match failing test: 6 threads, more elements
    const int N = 1000; 
    const int NUM_THREADS = 6;
    const int TOTAL_ELEMENTS = NUM_THREADS * N;
    
    // Create memory pools like the failing test
    vector<MemoryPool<sizeof(Node<int>), N>*> pools(NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; i++) {
        pools[i] = new MemoryPool<sizeof(Node<int>), N>();
    }
    
    {
        LocklessQueue<int> queue;
        
        // Fill the queue with concurrent pushes (like the failing test)
        cout << "Filling queue with " << TOTAL_ELEMENTS << " elements using " << NUM_THREADS << " push threads..." << endl;
        vector<thread> pushThreads;
        
        for (int t = 0; t < NUM_THREADS; t++) {
            pushThreads.emplace_back([&, t] {
                for(int i = 0; i < N; i++) {
                    queue.pushLeft(t * N + i, pools[t]);
                }
                cout << "Push thread " << t << " finished." << endl;
            });
        }
        
        for (auto& thread : pushThreads) {
            thread.join();
        }
        cout << "All push threads completed." << endl;
        
        // Use atomic counters to track progress
        atomic<int> leftPops{0};
        atomic<int> rightPops{0};
        atomic<bool> shouldStop{false};
        
        cout << "Starting " << NUM_THREADS << " concurrent pop threads (3 left, 3 right)..." << endl;
        vector<thread> popThreads;
        
        // Three threads popping from left  
        for (int t = 0; t < 3; t++) {
            popThreads.emplace_back([&, t] {
                cout << "PopLeft thread " << t << " starting..." << endl;
                int myPops = 0;
                while (!shouldStop.load() && myPops < N) {
                    auto val = queue.popLeft();
                    if (val.has_value()) {
                        myPops++;
                        int totalPops = leftPops.fetch_add(1) + 1;
                        if (totalPops % 500 == 0) {
                            cout << "Left pops: " << totalPops << endl;
                        }
                    } else {
                        cout << "PopLeft thread " << t << ": queue empty after " << myPops << " pops" << endl;
                        break;
                    }
                }
                cout << "PopLeft thread " << t << " finished with " << myPops << " pops." << endl;
            });
        }
        
        // Three threads popping from right
        for (int t = 0; t < 3; t++) {
            popThreads.emplace_back([&, t] {
                cout << "PopRight thread " << t << " starting..." << endl;
                int myPops = 0;
                while (!shouldStop.load() && myPops < N) {
                    auto val = queue.popRight();
                    if (val.has_value()) {
                        myPops++;
                        int totalPops = rightPops.fetch_add(1) + 1;
                        if (totalPops % 500 == 0) {
                            cout << "Right pops: " << totalPops << endl;
                        }
                    } else {
                        cout << "PopRight thread " << t << ": queue empty after " << myPops << " pops" << endl;
                        break;
                    }
                }
                cout << "PopRight thread " << t << " finished with " << myPops << " pops." << endl;
            });
        }
        
        // Wait for completion with timeout detection
        for (int i = 0; i < 60; i++) { // 60 second timeout
            this_thread::sleep_for(chrono::seconds(1));
            int total = leftPops.load() + rightPops.load();
            cout << "Progress after " << (i+1) << "s: Left=" << leftPops.load() 
                 << ", Right=" << rightPops.load() << ", Total=" << total << "/" << TOTAL_ELEMENTS << endl;
            
            if (total >= TOTAL_ELEMENTS) {
                cout << "All elements popped successfully!" << endl;
                shouldStop.store(true);
                break;
            }
            
            // Check if we're stuck (no progress for 5 seconds)
            static int lastTotal = -1;
            static int stuckCount = 0;
            if (total == lastTotal) {
                stuckCount++;
                if (stuckCount >= 5) {
                    cout << "DETECTED DEADLOCK: No progress for 5 seconds. Stopping..." << endl;
                    shouldStop.store(true);
                    break;
                }
            } else {
                stuckCount = 0;
                lastTotal = total;
            }
        }
        
        shouldStop.store(true);
        
        for (auto& thread : popThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        int finalTotal = leftPops.load() + rightPops.load();
        cout << "Final results: Left=" << leftPops.load() << ", Right=" << rightPops.load() 
             << ", Total=" << finalTotal << "/" << TOTAL_ELEMENTS << endl;
             
        if (finalTotal == TOTAL_ELEMENTS) {
            cout << "SUCCESS: All elements were popped!" << endl;
        } else {
            cout << "FAILURE: Only " << finalTotal << " out of " << TOTAL_ELEMENTS << " elements were popped." << endl;
        }
    }
    
    // Clean up
    for (auto pool : pools) {
        delete pool;
    }
    retireList.clear();
    
    return 0;
}