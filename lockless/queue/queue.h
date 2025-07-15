#ifndef QUEUE_H
#define QUEUE_H

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <thread>
#include <optional>
#include <iostream>
#include "../memoryPool/memoryPool.h"
#include "../hazardPointers/hazardRetire.h"
#include "../hazardPointers/hazardGuard.h"

// Tomorrow:
// Implement hazard pointers fully into lockless queue
// Add soak (long-duration high-concurrency test) to the lockless queue once it passes everything

// Based off of an algorithm developed by Sundell and Tsigas

// Predeclare for access below
template <typename T>
struct Node;

// We use pointer tagging for marking deletion, so all pointers must be at least 2-byte aligned
template <typename T>
class MarkedPtr {
    private:
        static constexpr uintptr_t MARK_MASK = 1;
        uintptr_t bits;

    public:
        // Used if no parameters are specified
        MarkedPtr() : bits(0) {}

        // Used if parameters are specified
        MarkedPtr(Node<T>* ptr, bool mark) : bits((uintptr_t)ptr | (mark ? MARK_MASK : 0)) {}

        // Marked as const as to not modify any member variables of the parent class
        Node<T>* getPtr() const {
            // ~MARK_MASK inverts the bits of MARK_MASK
            // The return statement clears the first bit and casts it to Node<T>*
            return (Node<T>*)(bits & ~MARK_MASK);
        }
        
        // Marked as const as to not modify any member variables of the parent class
        bool getMark() const {
            // Isolates the lowest bit
            // If the result is nonzero it means the mark is set and returns true
            return bits & MARK_MASK;
        }

        // Comparison operators for MarkedPtr objects
        // Marked as const as to not modify any member variables of the parent class
        bool operator==(const MarkedPtr& other) const {
            return bits == other.bits;
        }
        bool operator!=(const MarkedPtr& other) const {
            return !(*this == other);
        }
};

// Node of doubly linked list
// memoryBlock is for memory pool allocation
// ownerPool is the owner of memoryBlock
// the type of ownerPool must match T
// PoolBlocks is the number of blocks in the owner memory pool
template <typename T>
struct Node {
    std::atomic<MarkedPtr<T>> prev;
    std::atomic<MarkedPtr<T>> next;
    T data;

    // Track if this is a dummy node or not
    bool isDummy;

    // Track if a node is retired
    std::atomic<bool> isRetired;

    // For memory pool allocation
    void* memoryBlock;

    // Owner memory pool
    // Generic reference to MmemoryPool
    GenericMemoryPool* ownerPool;

    // Used if no parameters are provided
    Node(GenericMemoryPool* ownerPool, void* memoryBlock) : prev(MarkedPtr<T>(nullptr, false)), next(MarkedPtr<T>(nullptr, false)), 
        data(), isDummy(true), isRetired(false), memoryBlock(memoryBlock), ownerPool(ownerPool) {}
    
    // Used if parameters are provided
    Node(GenericMemoryPool* ownerPool, void* memoryBlock, const T& val) : prev(MarkedPtr<T>(nullptr, false)), next(MarkedPtr<T>(nullptr, false)), 
        data(val), isDummy(false), isRetired(false), memoryBlock(memoryBlock), ownerPool(ownerPool) {}
};

// Predeclare LocklessQueue
template<typename T>
class LocklessQueue;

// Free nodes from retireList if no longer protected
// Queue-specific version of updateRetireList from hazardRetire.h
template<typename T>
void updateRetireListQueue(LocklessQueue<T>& queue) {
    // surivors will hold still hazardous nodes
    std::vector<void*> survivors;

    // iterate through all nodes
    for (void* ptr : retireList) {
        // Only reclaim the node if no thread is currently protecting it with a hazard pointer
        if (isHazard(ptr)) {
            // Still in use, keep it for later checking
            survivors.push_back(ptr);
        } else {
            // Safe to free node
            queue.terminateNode(static_cast<Node<T>*>(ptr));
        }
    }

    // Set retireList to survivors
    retireList = std::move(survivors);
}

// Retire a given node for later deletion
// Queue-specific version of retireObj from hazardRetire.h
template<typename T>
void retireQueueNode(Node<T>* node, LocklessQueue<T>& queue) {
    // Return if node is already retired or is a dummy node
    // If node->isRetired is false exchange it for true
    if (node->isDummy || node->isRetired.exchange(true)) {
        return;
    }

    // Add the node to this thread's retire list
    retireList.push_back(static_cast<void*>(node));

    // If the retire list has grown large enough, attempt to reclaim nodes
    // This threshold can be tuned for performance/memory tradeoff
    // Potentially find a way to eliminate this boundary later
    if (retireList.size() >= 64) {
        // Clear retire list if applicable
        updateRetireListQueue<T>(queue);
    }
}

template<typename T>
class LocklessQueue {
    private:
        // Dummy node memory pool
        // Size must be assigned in constructor
        MemoryPool<sizeof(Node<T>), 2> pool; 

        // Copy sets a hazard pointer and returns a node pointer
        // This should only be called on nodes that are already protected or are dummy nodes
        HazardGuard<Node<T>> copy(Node<T>* node) {
            // If node is null, return nullptr for safety
            if (!node) {
                return HazardGuard<Node<T>>(nullptr);
            }

            // Return node
            return HazardGuard<Node<T>>(node, node->isDummy);
        }

        // Self explanatory
        void releaseNode(Node<T>* node) {
            // Return if node is invalid or is a dummy node
            // Dummy nodes should never be deleted during runtime and should thus be ignored
            if (!node || node->isDummy) {
                return;
            }

            // Additional safety check: don't release already retired nodes
            if (node->isRetired.load()) {
                return;
            }

            // If node is no longer protected retire it
            if (!isHazard(node)) {
                retireQueueNode<T>(node, *this);
            } else {
                // Release protection of node
                removeHazardPointer(node);
            }
        }

    //     // Adds node into a hazard pointer for the given thread
    //     // Returns nullptr if mark is set and returns node pointer otherwise
    //     HazardGuard<Node<T>> deref(std::atomic<MarkedPtr<T>>* ptr) {
    //         // Safety check: return nullptr if ptr is null
    //         if (!ptr) {
    //             return HazardGuard<Node<T>>(nullptr);
    //         }

    //         while (true) {
    //             // Load MarkedPtr object from atomic
    //             MarkedPtr<T> mPtr = ptr->load(std::memory_order_acquire);

    //             // Return nullptr if mark is set
    //             if (mPtr.getMark()) {
    //                 return HazardGuard<Node<T>>(nullptr);
    //             }

    //             // Load node from MarkedPtr
    //             Node<T>* node = mPtr.getPtr();

    //             // If node is null, return nullptr
    //             if (!node) {
    //                 return HazardGuard<Node<T>>(nullptr);
    //             }

    //             // Set hazard pointer manually (don't use RAII yet)
    //             if (!node->isDummy) {
    //                 setHazardPointer(node);
    //             }

    //             // Re-check that the pointer hasn't changed (ABA protection)
    //             MarkedPtr<T> mPtr2 = ptr->load(std::memory_order_acquire);
    //             if (mPtr.getPtr() != mPtr2.getPtr() || mPtr2.getMark()) {
    //                 // Clean up hazard pointer before continuing
    //                 if (!node->isDummy) {
    //                     removeHazardPointer(node);
    //                 }
                    
    //                 if (mPtr2.getMark()) {
    //                     return HazardGuard<Node<T>>(nullptr);
    //                 }
    //                 std::this_thread::yield();
    //                 continue;
    //             }

    //             // Create guard with node that's already protected
    //             // Use a special constructor that doesn't set the hazard pointer again
    //             HazardGuard<Node<T>> guard(node, node->isDummy);
                
    //             // If it's not a dummy node, we need to remove the manual hazard pointer
    //             // since the HazardGuard will manage it now
    //             if (!node->isDummy) {
    //                 removeHazardPointer(node);
    //             }

    //             return guard;
    //         }
    //     }

    //     // Like deref but a pointer is always returned
    //     HazardGuard<Node<T>> derefD(std::atomic<MarkedPtr<T>>* ptr) {
    //         // Safety check: return nullptr if ptr is null
    //         if (!ptr) {
    //             return HazardGuard<Node<T>>(nullptr);
    //         }

    //         while (true) {
    //             // Load MarkedPtr object from atomic
    //             MarkedPtr<T> mPtr = ptr->load(std::memory_order_acquire);

    //             // Load node from MarkedPtr
    //             Node<T>* node = mPtr.getPtr();

    //             // If node is null, return nullptr
    //             if (!node) {
    //                 return HazardGuard<Node<T>>(nullptr);
    //             }

    //             // Set hazard pointer manually (don't use RAII yet)
    //             if (!node->isDummy) {
    //                 setHazardPointer(node);
    //             }

    //             // Re-check that the pointer hasn't changed (ABA protection)
    //             MarkedPtr<T> mPtr2 = ptr->load(std::memory_order_acquire);
    //             if (mPtr.getPtr() != mPtr2.getPtr()) {
    //                 // Clean up hazard pointer before continuing
    //                 if (!node->isDummy) {
    //                     removeHazardPointer(node);
    //                 }
    //                 std::this_thread::yield();
    //                 continue;
    //             }

    //             // Now safely check if retired
    //             if (node->isRetired.load()) {
    //                 if (!node->isDummy) {
    //                     removeHazardPointer(node);
    //                 }
    //                 return HazardGuard<Node<T>>(nullptr);
    //             }

    //             // Create guard with node that's already protected
    //             HazardGuard<Node<T>> guard(node, node->isDummy);
                
    //             // If it's not a dummy node, we need to remove the manual hazard pointer
    //             // since the HazardGuard will manage it now
    //             if (!node->isDummy) {
    //                 removeHazardPointer(node);
    //             }

    //             return guard;
    //         }
    //     }

    //     // Atomically sets deletion marker on prev pointer
    //     // Tells other threads not to use this connection
    //     // Returns true if this thread marked it, false if already marked
    //     bool markPrev(Node<T>* node) {
    //         // Safety check: return if node is invalid
    //         if (!node) {
    //             std::cout << "markPrev: node is null, returning false" << std::endl;
    //             return false;
    //         }
            
    //         int attempts = 0;
    //         while (true) {
    //             attempts++;
    //             // Retrieve marked pointer from node
    //             MarkedPtr<T> link = node->prev.load();

    //             // If already marked, return false (someone else marked it)
    //             if (link.getMark()) {
    //                 if (attempts <= 3) {
    //                     std::cout << "markPrev: node=" << node << " prev already marked (ptr=" << link.getPtr() << "), returning false, thread " << std::this_thread::get_id() << std::endl;
    //                 }
    //                 return false;
    //             }

    //             // Try to mark it - if successful, return true
    //             if (node->prev.compare_exchange_weak(link, MarkedPtr<T>(link.getPtr(), true))) {
    //                 if (attempts <= 3) {
    //                     std::cout << "markPrev: node=" << node << " successfully marked prev (ptr=" << link.getPtr() << "), thread " << std::this_thread::get_id() << std::endl;
    //                 }
    //                 return true;
    //             }
    //             if (attempts <= 5) {
    //                 std::cout << "markPrev: node=" << node << " CAS failed, retrying (attempt " << attempts << "), thread " << std::this_thread::get_id() << std::endl;
    //             }
    //             if (attempts > 100) {
    //                 std::cout << "markPrev: Too many failed attempts (" << attempts << "), breaking" << std::endl;
    //                 return false;
    //             }
    //         }
    //     }

    //     // Common logic for push operations
    //     void pushCommon(Node<T>* node, Node<T>* next) {
    //         while (true) {
    //             // Retrieve next->prev
    //             MarkedPtr<T> link = next->prev;

    //             // If link is marked or node->next does not equal unmarked next
    //             if (link.getMark() || node->next.load() != MarkedPtr<T>(next, false)) {
    //                 // Break out of loop
    //                 break;
    //             }

    //             // Attempt to swap next->prev from link to unmarked pointer to node
    //             if (next->prev.compare_exchange_weak(link, MarkedPtr<T>(node, false))) {
    //                 // Release next->prev
    //                 releaseNode(link.getPtr());

    //                 // Helps complete incomplete right side of insertion
    //                 if (node->prev.load().getMark()) {
    //                     HazardGuard<Node<T>> Hprev2 = copy(node);
    //                     Node<T>* prev2 = Hprev2.ptr;
    //                     prev2 = helpInsert(prev2, next);
    //                     releaseNode(prev2);
    //                 }
                    
    //                 // Break out of the loop
    //                 break;
    //             }

    //             // Yield the thread if unsuccessful
    //             std::this_thread::yield();
    //         }
    //     }

    //    // Helps complete insert operation that was interrupted
    //     Node<T>* helpInsert(Node<T>* prev, Node<T>* node) {
    //         // Safety checks: return appropriate values if inputs are invalid
    //         if (!prev) {
    //             return node;
    //         }
    //         if (!node) {
    //             return prev;
    //         }

    //         // HI1: lastlink.d:=true; - Initialize lastlink flag
    //         bool lastlink_d = true;
            
    //         static int helpInsert_count = 0;
    //         helpInsert_count++;
    //         if (helpInsert_count % 10000 == 0) {
    //             std::cout << "helpInsert: Called " << helpInsert_count << " times, thread " << std::this_thread::get_id() << std::endl;
    //         }

    //         int helpInsert_loop_count = 0;
    //         // HI2: while true do - Main loop
    //         while (true) {
    //             helpInsert_loop_count++;
    //             if (helpInsert_loop_count % 100 == 0) {
    //                 std::cout << "helpInsert: Loop " << helpInsert_loop_count << ", prev=" << prev << ", node=" << node << std::endl;
    //             }
                
    //             // HI3: prev2:=READ_NODE(&prev.next); - Get prev.next
    //             HazardGuard<Node<T>> Hprev2 = deref(&prev->next);
    //             Node<T>* prev2 = Hprev2.ptr;
                
    //             // HI4: if prev2 = NULL then - If prev2 is null (marked)
    //             if (!prev2) {
    //                 // HI5: if lastlink.d = false then - If we haven't called DeleteNext on prev
    //                 if (!lastlink_d) {
    //                     // HI6: DeleteNext(prev); - Call DeleteNext on prev
    //                     helpDelete(prev);
    //                     // HI7: lastlink.d:=true; - Set flag
    //                     lastlink_d = true;
    //                 }
                    
    //                 // HI8: prev2:=READ_DEL_NODE(&prev.prev); - Get prev.prev
    //                 HazardGuard<Node<T>> Hprev2_alt = derefD(&prev->prev);
    //                 Node<T>* prev2_alt = Hprev2_alt.ptr;
                    
    //                 // HI9: RELEASE_NODE(prev); - Release prev
    //                 releaseNode(prev);
                    
    //                 // HI10: prev:=prev2; - Set prev to prev2
    //                 prev = prev2_alt;
                    
    //                 // HI11: continue; - Continue
    //                 continue;
    //             }

    //             // HI12: link1:=node.prev; - Load node's prev pointer
    //             MarkedPtr<T> link1 = node->prev.load();
                
    //             // HI13: if link1.d = true then - If link is marked
    //             if (link1.getMark()) {
    //                 // HI14: RELEASE_NODE(prev2); - Release prev2
    //                 releaseNode(prev2);
    //                 // HI15: break; - Break out of loop
    //                 break;
    //             }

    //             // HI16: if prev2 ≠ node then - If prev2 != node
    //             if (prev2 != node) {
    //                 // HI17: lastlink.d:=false; - Set flag for forward traversal
    //                 lastlink_d = false;
                    
    //                 // HI18: RELEASE_NODE(prev); - Release prev
    //                 releaseNode(prev);
                    
    //                 // HI19: prev:=prev2; - Set prev to prev2
    //                 prev = prev2;
                    
    //                 // HI20: continue; - Continue
    //                 continue;
    //             }

    //             // HI21: RELEASE_NODE(prev2); - Release prev2
    //             releaseNode(prev2);

    //             // HI22: if CAS(&node.prev,link1,(prev,false)) then - Try to connect node to prev
    //             if (node->prev.compare_exchange_weak(link1, MarkedPtr<T>(prev, false))) {
    //                 // HI23: COPY_NODE(prev); - Copy prev (increment reference count)
    //                 // This is already handled by the hazard guard system
                    
    //                 // HI24: RELEASE_NODE(link1.p); - Release old prev
    //                 releaseNode(link1.getPtr());
                    
    //                 // HI25: if prev.prev.d = true then continue; - If prev's prev is marked, continue
    //                 if (prev->prev.load().getMark()) {
    //                     continue;
    //                 }
                    
    //                 // HI26: break; - Break out of loop
    //                 break;
    //             }

    //             // HI27: Back-Off - Back off
    //             std::this_thread::yield();
    //         }

    //         // HI28: return prev; - Return prev
    //         return prev;
    //     }

    //     // Help complete a delete operation that may have been interrupted
    //     void helpDelete(Node<T>* node) {
    //         // Safety check: return if node is invalid
    //         if (!node) {
    //             return;
    //         }
            
    //         std::cout << "helpDelete: Starting for node=" << node << " (data=" << node->data << "), thread " << std::this_thread::get_id() << std::endl;

    //         // DN1-DN4: Mark the prev pointer of the node (loop until marked)
    //         while (true) {
    //             MarkedPtr<T> link = node->prev.load();
    //             if (link.getMark() || node->prev.compare_exchange_weak(link, MarkedPtr<T>(link.getPtr(), true))) {
    //                 break;
    //             }
    //         }
            
    //         // DN5: lastlink.d:=true; - Initialize lastlink flag
    //         bool lastlink_d = true;
            
    //         // DN6: prev:=READ_DEL_NODE(&node.prev); - Get prev pointer
    //         HazardGuard<Node<T>> Hprev = derefD(&node->prev);
    //         Node<T>* prev = Hprev.ptr;

    //         // DN7: next:=READ_DEL_NODE(&node.next); - Get next pointer
    //         HazardGuard<Node<T>> Hnext = derefD(&node->next);
    //         Node<T>* next = Hnext.ptr;

    //         std::cout << "helpDelete: Initial prev=" << prev << ", next=" << next << ", node=" << node << std::endl;

    //         // Safety checks: if prev or next are null, return
    //         if (!prev || !next) {
    //             std::cout << "helpDelete: prev or next is null, returning" << std::endl;
    //             return;
    //         }
            
    //         int helpDelete_loop_count = 0;
    //         // DN8: while true do - Main loop
    //         while (true) {
    //             helpDelete_loop_count++;
    //             if (helpDelete_loop_count % 100 == 0) {
    //                 std::cout << "helpDelete: Loop iteration " << helpDelete_loop_count << ", prev=" << prev << ", next=" << next << ", node=" << node << ", thread " << std::this_thread::get_id() << std::endl;
    //             }
                
    //             // DN9: if prev = next then break; - If prev equals next, break
    //             if (prev == next) {
    //                 std::cout << "helpDelete: prev == next (" << prev << "), breaking immediately" << std::endl;
    //                 break;
    //             }

    //             // DN10: if next.next.d = true then - If next is marked
    //             if (next && next->next.load().getMark()) {
    //                 std::cout << "helpDelete: next is marked, advancing next pointer" << std::endl;
    //                 // DN11: next2:=READ_DEL_NODE(&next.next); - Get next.next
    //                 HazardGuard<Node<T>> Hnext2 = derefD(&next->next);
    //                 Node<T>* next2 = Hnext2.ptr;
                    
    //                 // DN12: RELEASE_NODE(next); - Release next
    //                 releaseNode(next);
                    
    //                 // DN13: next:=next2; - Move to next2
    //                 Hnext = std::move(Hnext2);
    //                 next = next2;
                    
    //                 // DN14: continue; - Continue loop
    //                 continue;
    //             }

    //             // DN15: prev2:=READ_NODE(&prev.next); - Get prev.next  
    //             HazardGuard<Node<T>> Hprev2 = deref(&prev->next);
    //             Node<T>* prev2 = Hprev2.ptr;

    //             // DN16: if prev2 = NULL then - If prev2 is null (marked)
    //             if (!prev2) {
    //                 // DN17: if lastlink.d = false then - If we haven't called DeleteNext on prev
    //                 if (!lastlink_d) {
    //                     // DN18: DeleteNext(prev); - Call DeleteNext on prev
    //                     helpDelete(prev);
    //                     // DN19: lastlink.d:=true; - Set flag
    //                     lastlink_d = true;
    //                 }
                    
    //                 // DN20: prev2:=READ_DEL_NODE(&prev.prev); - Get prev.prev
    //                 HazardGuard<Node<T>> Hprev2_alt = derefD(&prev->prev);
    //                 Node<T>* prev2_alt = Hprev2_alt.ptr;
                    
    //                 // DN21: RELEASE_NODE(prev); - Release prev
    //                 releaseNode(prev);
                    
    //                 // DN22: prev:=prev2; - Set prev to prev2
    //                 prev = prev2_alt;
                    
    //                 // DN23: continue; - Continue
    //                 continue;
    //             }

    //             // DN24: if prev2 ≠ node then - If prev2 != node
    //             if (prev2 != node) {
    //                 // DN25: lastlink.d:=false; - Set flag for forward traversal
    //                 lastlink_d = false;
                    
    //                 // DN26: RELEASE_NODE(prev); - Release prev
    //                 releaseNode(prev);
                    
    //                 // DN27: prev:=prev2; - Set prev to prev2
    //                 prev = prev2;
                    
    //                 // DN28: continue; - Continue
    //                 continue;
    //             }

    //             // DN29: RELEASE_NODE(prev2); - Release prev2
    //             releaseNode(prev2);

    //             // DN30: if CAS(&prev.next,(node,false),(next,false)) then - Try to connect prev to next
    //             MarkedPtr<T> expected(node, false);
                
    //             if (prev->next.compare_exchange_weak(expected, MarkedPtr<T>(next, false))) {
    //                 // DN31: COPY_NODE(next); - Copy next (increment reference count)
    //                 // This is already handled by the hazard guard system
                    
    //                 // DN32: RELEASE_NODE(node); - Release node
    //                 releaseNode(node);
                    
    //                 // DN33: break; - Break out of loop
    //                 break;
    //             }

    //             // DN34: Back-Off - Back off
    //             std::this_thread::yield();
    //         }
            
    //         // DN35: RELEASE_NODE(prev); - Release prev
    //         releaseNode(prev);
            
    //         // DN36: RELEASE_NODE(next); - Release next
    //         releaseNode(next);
    //     }

    //     // Tries to break cross references between the given node and any of the nodes it references
    //     // This is done by repeatedly updating the prev and next point as long as they reference a fully marked node
    //     void removeCrossReference(Node<T>* node) {
    //         // Safety check: return if node is null or retired
    //         if (!node || node->isRetired.load()) {
    //             return;
    //         }

    //         while (true) {
    //             // Retrieve node->prev pointer
    //             Node<T>* prev = node->prev.load().getPtr();

    //             // Safety check for prev pointer
    //             if (prev && prev->next.load().getMark()) {
    //                 // Retrieve prev->prev
    //                 HazardGuard<Node<T>> Hprev2 = derefD(&prev->prev);
    //                 Node<T>* prev2 = Hprev2.ptr;

    //                 // Replace node->prev with marked node->prev->prev
    //                 node->prev.store(MarkedPtr<T>(prev2, true));

    //                 // Release old node->prev
    //                 releaseNode(prev);

    //                 // Go into the next loop iteration
    //                 continue;
    //             }

    //             // Retrieve node->next pointer
    //             Node<T>* next = node->next.load().getPtr();

    //             // Safety check for next pointer
    //             if (next && next->next.load().getMark()) {
    //                 // Retrieve next->next
    //                 HazardGuard<Node<T>> Hnext2 = derefD(&next->next);
    //                 Node<T>* next2 = Hnext2.ptr;

    //                 // Replace node->next with marked node->next->next
    //                 node->next.store(MarkedPtr<T>(next2, true));

    //                 // Release old node->next
    //                 releaseNode(next);

    //                 // Go into the next loop iteration
    //                 continue;
    //             }

    //             // Break from loop
    //             break;
    //         }
    //     }

    public:

        // Head and tail node pointers
        // public for testing purposes
        Node<T>* head;
        Node<T>* tail;

        LocklessQueue() {
            // Allocate memory for head and tail
            // head and tail will belong to local memory pool
            void* headBlock = pool.allocate();
            void* tailBlock = pool.allocate();

            // Initialize head and tail
            head = new (headBlock) Node<T>(&pool, headBlock);
            tail = new (tailBlock) Node<T>(&pool, tailBlock);

            // Connect nodes
            head->next.store(MarkedPtr<T>(tail, false));
            tail->prev.store(MarkedPtr<T>(head, false));
        }

        // It is assumed that no threads will be using the queue when this runs
        ~LocklessQueue() {
            // Cycle through all remaining items and terminate them
            Node<T>* curr = head->next.load().getPtr();
            Node<T>* next;
            while (curr != tail) {
                next = curr->next.load().getPtr();
                terminateNode(curr, true);
                curr = next;
            }
            terminateNode(head, true);
            terminateNode(tail, true);

            // Clear retire list
            retireList.clear();
        }

        // Returns a node created with the given data
        Node<T>* createNode(T data, GenericMemoryPool* memoryPool) {
            // Try to allocate memory block
            void* memoryBlock;
            try {
                // Allocate memory from the pool
                memoryBlock = memoryPool->allocate();

            // Catch bad alloc
            } catch (std::bad_alloc&) {
                // Attempt to clear retireList to free memory
                updateRetireListQueue<T>(*this);
                
                // Try to allocate from the pool again
                // Bad alloc will be thrown again if nothing was cleared
                memoryBlock = memoryPool->allocate();
            }

            // Create node object
            Node<T>* node = new (memoryBlock) Node<T>(memoryPool, memoryBlock, data);

            // Return node
            return node;
        }

        // Calls Node destructor and deletes it from memory
        // bool destructor should be set to true in the destructor
        // Needs to be public to retire list nodes
        void terminateNode(Node<T>* node, bool destructor = false) {
            // If node is nullptr return
            if (!node) {
                return;
            }

            // Retrieve memory pool
            GenericMemoryPool* ownerPool = node->ownerPool;

            // Explicitly call destructor for Node<T>
            // May not be needed but good to have
            node->~Node<T>();

            // Handle memory block
            ownerPool->deallocate(node->memoryBlock);
        }

        // // Function to push to the left side of the queue
        // // Returns a pointer to the node incase it is needed later
        // // Creation of a node requires a memory block from an external memory pool
        // Node<T>* pushLeft(T data, GenericMemoryPool* memoryPool) {
        //     // Create node object
        //     Node<T>* node = createNode(data, memoryPool);

        //     // Copy head
        //     HazardGuard<Node<T>> Hprev = copy(head);
        //     Node<T>* prev = Hprev.ptr;

        //     // Gets pointer to current prev->next
        //     // node will take it's place
        //     // Will be nullptr if prev->next is marked or null
        //     HazardGuard<Node<T>> Hnext = deref(&prev->next);
        //     Node<T>* next = Hnext.ptr;

        //     while (true) {
        //         // Safety check: if next is null, try again
        //         if (!next) {
        //             Hnext = deref(&prev->next);
        //             next = Hnext.ptr;
        //             std::this_thread::yield();
        //             continue;
        //         }

        //         // if prev->next does not equal unmarked next
        //         if (prev->next.load() != MarkedPtr<T>(next, false)) {
        //             // Redeclare it
        //             Hnext = deref(&prev->next);
        //             next = Hnext.ptr;

        //             // Go into the next loop iteration
        //             continue;
        //         }

        //         // Set values for node->prev and node->next
        //         // These pointers are unmarked
        //         node->prev.store(MarkedPtr<T>(prev, false));
        //         node->next.store(MarkedPtr<T>(next, false));

        //         // Set expected value
        //         MarkedPtr<T> expected(next, false);

        //         // Try to atomically update prev->next pointer to next, if it still points to unmarked next
        //         if (prev->next.compare_exchange_weak(expected, MarkedPtr<T>(node, false))) {

        //             // Break out of loop
        //             break;
        //         }

        //         // Yield the thread before going to next loop iteration
        //         std::this_thread::yield();
        //     }

        //     // Run pushCommon to fully insert node into the queue
        //     pushCommon(node, next);

        //     // Return node
        //     return node;
        // }

        // // Function to push to the right side of the queue
        // // Returns a pointer to the node incase it is needed later
        // // Creation of a node requires a memory block from an external memory pool
        // Node<T>* pushRight(T data, GenericMemoryPool* memoryPool) {
        //     // Increment refCount of head
        //     HazardGuard<Node<T>> Hnext = copy(tail);
        //     Node<T>* next = Hnext.ptr;

        //     // Gets pointer to current prev->next
        //     // node will take it's place
        //     // Will be nullptr if prev->next is marked or null
        //     HazardGuard<Node<T>> Hprev = deref(&next->prev);
        //     Node<T>* prev = Hprev.ptr;

        //     // Create node object
        //     Node<T>* node = createNode(data, memoryPool);

        //     while (true) {
        //         // Safety check: if prev is null, try again
        //         if (!prev) {
        //             Hprev = deref(&next->prev);
        //             prev = Hprev.ptr;
        //             std::this_thread::yield();
        //             continue;
        //         }

        //         // if prev->next does not equal unmarked next
        //         if (prev->next.load() != MarkedPtr<T>(next, false)) {
        //             // Redeclare it from next->prev
        //             Hprev = deref(&next->prev);
        //             prev = Hprev.ptr;

        //             // Go into the next loop iteration
        //             continue;
        //         }

        //         // Set values for node->prev and node->next
        //         // These pointers are unmarked
        //         node->prev.store(MarkedPtr<T>(prev, false));
        //         node->next.store(MarkedPtr<T>(next, false));

        //         // Set expected value
        //         MarkedPtr<T> expected(next, false);

        //         // Try to atomically update prev->next pointer to next, if it still points to unmarked next
        //         if (prev->next.compare_exchange_weak(expected, MarkedPtr<T>(node, false))) {
        //             // Break out of loop
        //             break;
        //         }

        //         // Yield the thread before going to next loop iteration
        //         std::this_thread::yield();
        //     }

        //     // Run pushCommon to fully insert node into the queue
        //     pushCommon(node, next);

        //     // Return node
        //     return node;
        // }

        // // Function to pop from the left side of the queue
        // // Returns the data from the node or nothing
        // std::optional<T> popLeft() {
        //     int popLeft_attempts = 0;
        //     while (true) { // This loop restarts the entire traversal
        //         popLeft_attempts++;
        //         if (popLeft_attempts % 10000 == 0) {
        //             std::cout << "popLeft: Main loop attempt " << popLeft_attempts << ", thread " << std::this_thread::get_id() << std::endl;
        //         }
        //         HazardGuard<Node<T>> Hprev = copy(head);
        //         Node<T>* prev = Hprev.ptr;
                
        //         HazardGuard<Node<T>> Hnode = deref(&prev->next);
        //         Node<T>* node = Hnode.ptr;
                
        //         if (!node || node == tail) {
        //             return std::nullopt;
        //         }

        //         // We have a candidate node to pop.
        //         // Let's try to pop it.
                
        //         MarkedPtr<T> nextLink = node->next.load();

        //         if (nextLink.getMark()) {
        //             // PL10: DeleteNext(node); - Help delete the marked node
        //             std::cout << "popLeft: Found marked node, calling helpDelete, thread " << std::this_thread::get_id() << std::endl;
        //             helpDelete(node);
        //             // PL11: RELEASE_NODE(node); - Release node (handled by hazard guard)
        //             // PL12: continue; - Continue main loop
        //             continue;
        //         }
                
        //         // Try to mark the node for deletion.
        //         if (node->next.compare_exchange_weak(nextLink, MarkedPtr<T>(nextLink.getPtr(), true))) {
        //             // Success!
        //             T data = node->data;
        //             helpDelete(node);
                    
        //             HazardGuard<Node<T>> Hnext = derefD(&node->next);
        //             Node<T>* next = Hnext.ptr;

        //             if (next) {
        //                 prev = helpInsert(prev, next);
        //             }
                    
        //             removeCrossReference(node);
                    
        //             return data;
        //         }
                
        //         // CAS failed. Another thread interfered.
        //         // Yield and restart the whole traversal.
        //         std::this_thread::yield();
        //         // The loop will restart automatically.
        //     }
        // }

        // // Function to pop from the right side of the queue
        // // Returns the data from the node or nothing
        // std::optional<T> popRight() {
        //     // PR1: next:=COPY_NODE(tail);
        //     HazardGuard<Node<T>> Hnext = copy(tail);
        //     Node<T>* next = Hnext.ptr;

        //     // PR2: node:=READ_NODE(&next.prev);
        //     HazardGuard<Node<T>> Hnode = deref(&next->prev);
        //     Node<T>* node = Hnode.ptr;

        //     // PR3: while true do
        //     int popRight_attempts = 0;
        //     while (true) {
        //         popRight_attempts++;
        //         if (popRight_attempts % 10000 == 0) {
        //             std::cout << "popRight: Main loop attempt " << popRight_attempts << ", node=" << node << ", next=" << next << ", thread " << std::this_thread::get_id() << std::endl;
        //         }

        //         // PR4: if node.next ≠ ⟨next,false⟩ then
        //         MarkedPtr<T> nodeNext = node->next.load();
        //         MarkedPtr<T> expectedNext(next, false);
        //         if (nodeNext != expectedNext) {
        //             // PR5: node:=HelpInsert(node,next);
        //             if (popRight_attempts % 10000 == 0) {
        //                 std::cout << "popRight: Node->next changed, calling helpInsert, thread " << std::this_thread::get_id() << std::endl;
        //             }
        //             Node<T>* newNode = helpInsert(node, next);
        //             // Update our node reference
        //             releaseNode(node);
        //             node = newNode;
        //             // PR6: continue;
        //             continue;
        //         }

        //         // PR7: if node = head then
        //         if (node == head) {
        //             // PR8: RELEASE_NODE(node);
        //             // PR9: RELEASE_NODE(next);
        //             // PR10: return ⊥;
        //             if (popRight_attempts % 10000 == 0) {
        //                 std::cout << "popRight: Queue empty, node is head, returning nullopt" << std::endl;
        //             }
        //             return std::nullopt;
        //         }

        //         // PR11: if CAS(&node.next,⟨next,false⟩,⟨next,true⟩) then
        //         if (node->next.compare_exchange_weak(expectedNext, MarkedPtr<T>(next, true))) {
        //             // PR12: HelpDelete(node);
        //             helpDelete(node);

        //             // PR13: prev:=READ_DEL_NODE(&node.prev);
        //             HazardGuard<Node<T>> Hprev = derefD(&node->prev);
        //             Node<T>* prev = Hprev.ptr;

        //             // PR14: prev:=HelpInsert(prev,next);
        //             if (prev) {
        //                 prev = helpInsert(prev, next);
        //             }

        //             // PR15: RELEASE_NODE(prev);
        //             // PR16: RELEASE_NODE(next);
        //             // PR17: value:=node.value;
        //             T data = node->data;
                    
        //             // PR18: break;
        //             // PR20: RemoveCrossReference(node);
        //             removeCrossReference(node);
                    
        //             // PR21: RELEASE_NODE(node);
        //             // PR22: return value;
        //             return data;
        //         }

        //         // PR19: Back-Off
        //         std::this_thread::yield();
        //     }
        // }

        // // Function to remove a given node
        // // Based off of popRight
        // // Returns the data from the node or nothing
        // std::optional<T> removeNode(Node<T>* node) {
        //     // Return nullopt if node is invalid or a dummy node
        //     if (!node || node->isDummy) {
        //         return std::nullopt;
        //     }

        //     // Protect node
        //     HazardGuard<Node<T>> guard = copy(node);

        //     // To be returned
        //     T data = node->data;

        //     while (true) {
        //         // Retrieve marked pointer to node->next
        //         MarkedPtr<T> link = node->next.load();

        //         // If already logically deleted, help finish and return nullopt
        //         if (link.getMark()) {
        //             // Help deletion along
        //             helpDelete(node);

        //             // Return nullopt
        //             return std::nullopt;
        //         }

        //         // Try to atomically update node->next pointer to marked pointer to next, if it is still an unmarked pointer to next
        //         if (node->next.compare_exchange_weak(link, MarkedPtr<T>(link.getPtr(), true))) {
        //             // Call helpDelete to help remove the node now that it is marked
        //             helpDelete(node);

        //             // Retrieve node->prev
        //             HazardGuard<Node<T>> Hprev = derefD(&node->prev);
        //             Node<T>* prev = Hprev.ptr;

        //             // Retrieve node->next
        //             HazardGuard<Node<T>> Hnext = derefD(&node->next);
        //             Node<T>* next = Hnext.ptr;

        //             // Connect prev and next
        //             // If prev or next are marked helpInsert will deal with it
        //             if (next) {
        //                 prev = helpInsert(prev, next);
        //             }

        //             // Break out of loop
        //             break;
        //         }
                
        //         // Yield the thread before going to next loop iteration
        //         std::this_thread::yield();
        //     }
            
        //     // Break possible cyclic references that include node
        //     removeCrossReference(node);

        //     // Return data
        //     return data;
        // }
};

#endif