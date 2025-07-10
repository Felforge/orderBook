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
#include "../hazardPointers/hazardPointers.h"

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
        Node<T>* copy(Node<T>* node) {
            // If node is null, return nullptr for safety
            if (!node) {
                return nullptr;
            }

            // Set hazard pointer to protect the node (only for non-dummy nodes)
            if (!node->isDummy) {
                setHazardPointer(node);
            }

            // Return node
            return node;
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

        // Adds node into a hazard pointer for the given thread
        // Returns nullptr if mark is set and returns node pointer otherwise
        Node<T>* deref(std::atomic<MarkedPtr<T>>* ptr) {
            // Safety check: return nullptr if ptr is null
            if (!ptr) {
                return nullptr;
            }

            while (true) {
                // Load MarkedPtr object from atomic
                MarkedPtr<T> mPtr = ptr->load(std::memory_order_acquire);

                // Return nullptr if mark is set
                if (mPtr.getMark()) {
                    return nullptr;
                }

                // Load node from MarkedPtr
                Node<T>* node = mPtr.getPtr();

                // If node is null, return nullptr
                if (!node) {
                    return nullptr;
                }

                // Set hazard pointer BEFORE checking anything about the node
                if (!node->isDummy) {
                    setHazardPointer(node);
                }

                // Re-check that the pointer hasn't changed (ABA protection)
                MarkedPtr<T> mPtr2 = ptr->load(std::memory_order_acquire);
                if (mPtr.getPtr() != mPtr2.getPtr() || mPtr2.getMark()) {
                    // Pointer changed, retry
                    if (!node->isDummy) {
                        removeHazardPointer(node);
                    }
                    if (mPtr2.getMark()) {
                        return nullptr;
                    }
                    std::this_thread::yield();
                    continue;
                }

                // Now safely check if retired
                if (node->isRetired.load()) {
                    if (!node->isDummy) {
                        removeHazardPointer(node);
                    }
                    return nullptr;
                }

                // Node is protected and valid
                return node;
            }

            // After too many retries, give up
            return nullptr;
        }

        // Like deref but a pointer is always returned
        Node<T>* derefD(std::atomic<MarkedPtr<T>>* ptr) {
            // Safety check: return nullptr if ptr is null
            if (!ptr) {
                return nullptr;
            }

            while (true) {
                // Load MarkedPtr object from atomic
                MarkedPtr<T> mPtr = ptr->load(std::memory_order_acquire);

                // Load node from MarkedPtr
                Node<T>* node = mPtr.getPtr();

                // If node is null, return nullptr
                if (!node) {
                    return nullptr;
                }

                // Set hazard pointer BEFORE checking anything about the node
                if (!node->isDummy) {
                    setHazardPointer(node);
                }

                // Re-check that the pointer hasn't changed (ABA protection)
                MarkedPtr<T> mPtr2 = ptr->load(std::memory_order_acquire);
                if (mPtr.getPtr() != mPtr2.getPtr()) {
                    // Pointer changed, retry
                    if (!node->isDummy) {
                        removeHazardPointer(node);
                    }
                    std::this_thread::yield();
                    continue;
                }

                // Now safely check if retired
                if (node->isRetired.load()) {
                    if (!node->isDummy) {
                        removeHazardPointer(node);
                    }
                    return nullptr;
                }

                // Node is protected and valid
                return node;
            }

            // After too many retries, give up
            return nullptr;
        }

        // Atomically sets deletion marker on prev pointer
        // Tells other threads not to use this connection
        void markPrev(Node<T>* node) {
            // Safety check: return if node is invalid
            if (!node) {
                return;
            }

            while (true) {
                // Retrieve marked pointer from node
                MarkedPtr<T> link = node->prev.load();

                // Exchange for marked version of the same pointer
                // Return when exchange is successful
                if (link.getMark() || node->prev.compare_exchange_weak(link, MarkedPtr<T>(link.getPtr(), true))) {
                    return;
                }
            }
        }

        // Common logic for push operations
        void pushCommon(Node<T>* node, Node<T>* next) {
            while (true) {
                // Retrieve next->prev
                MarkedPtr<T> link = next->prev;

                // If link is marked or node->next does not equal unmarked next
                if (link.getMark() || node->next.load() != MarkedPtr<T>(next, false)) {
                    // Break out of loop
                    break;
                }

                // Attempt to swap next->prev from link to unmarked pointer to node
                if (next->prev.compare_exchange_weak(link, MarkedPtr<T>(node, false))) {
                    // Increase refCount of node
                    copy(node);

                    // Release next->prev
                    releaseNode(link.getPtr());

                    // Helps complete incomplete right side of insertion
                    if (node->prev.load().getMark()) {
                        Node<T>* prev2 = copy(node);
                        prev2 = helpInsert(prev2, next);
                        releaseNode(prev2);
                    }
                    
                    // Break out of the loop
                    break;
                }

                // Yield the thread if unsuccessful
                std::this_thread::yield();
            }

            // Release references to next and node
            releaseNode(next);
            releaseNode(node);
        }

        // Helps complete insert operation that was interrupted
        Node<T>* helpInsert(Node<T>* prev, Node<T>* node) {
            // Safety checks: return appropriate values if inputs are invalid
            if (!prev) {
                return node;
            }
            if (!node) {
                return prev;
            }

            // Set mark of last connection
            bool lastMark = true;

            while (true) {
                // Retrieves safe to use copy of prev->next
                Node<T>* prev2 = deref(&prev->next);

                // if prev2 is invalid
                if (!prev2) {
                    // If lastMark is set to false
                    if (!lastMark) {
                        // Calc helpDelete on prev
                        helpDelete(prev);

                        // Set lastMark to true to match helpDelete
                        lastMark = true;
                    }

                    // Update prev2
                    prev2 = derefD(&prev->prev);

                    // Release reference from prev
                    releaseNode(prev);

                    // Update prev to prev2
                    prev = prev2;

                    // Safety check after assignment
                    if (!prev) {
                        return node;
                    }

                    // Go into the next loop iteration
                    continue;
                }

                // Load the current value of node's prev pointer
                MarkedPtr<T> link = node->prev.load();

                // If link is marked
                if (link.getMark()) {
                    // Release prev2 reference
                    releaseNode(prev2);
                    // Break out of loop
                    break;
                }

                // If prev2 doesn't point to node we need to traverse further
                if (prev2 != node) {
                    // Update lastMark
                    lastMark = prev->next.load().getMark();

                    // Release reference from prev
                    releaseNode(prev);

                    // Move forward in the queue
                    prev = prev2;
                    
                    // Go into the next loop iteration
                    continue;
                }

                // It is assumed here that prev2 == node so a reference could be released
                // This is because a reference added to prev2 added it to node
                releaseNode(prev2);

                // CAS is needed to connect prev and node together, as the field above asserted they aren't
                if (node->prev.compare_exchange_weak(link, MarkedPtr<T>(prev, false))) {
                    // Incrament prev's refCount
                    copy(prev);

                    // Release the reference to the old previous node
                    releaseNode(link.getPtr());

                    // If prev's prev pointer is not marked we are done
                    if (prev->prev.load().getMark()) {
                        // Continue into next loop iteration
                        continue;
                    }

                    // Break out of loop otherwise
                    break;
                }
                // Yield the thread if unsuccessful
                std::this_thread::yield();
            }

            // Return the node that now precedes the inserted node
            return prev;
        }

        // Help complete a delete operation that may have been interrupted
        void helpDelete(Node<T>* node) {
            // Safety check: return if node is invalid
            if (!node) {
                return;
            }

            // Mark node's prev connection
            markPrev(node);

            // Retrieve prev and next and increment refCount
            Node<T>* prev = derefD(&node->prev);
            Node<T>* next = derefD(&node->next);

            // Safety checks: if prev or next are null, return
            if (!prev || !next) {
                releaseNode(prev);
                releaseNode(next);
                return;
            }

            // Marker of last link
            // Must be true due to markPrev being called above
            bool lastMark = true;

            while (true) {
                // if prev and next are equal
                // Technically should not be possible but better to be safe
                if (prev == next) {
                    // Break out of loop
                    break;
                }

                // Safety check: if next is null, break
                if (!next) {
                    break;
                }

                // If next->next is marked
                if (next->next.load().getMark()) {
                    // Retrieve next->next and increment refCount
                    Node<T>* next2 = derefD(&next->next);

                    // Release next
                    releaseNode(next);

                    // Set next to next2
                    next = next2;

                    // Safety check after assignment
                    if (!next) {
                        break;
                    }

                    // Go into the next loop iteration
                    continue;
                }

                // Safety check: if prev is null, break
                if (!prev) {
                    break;
                }

                // Retrieve prev->next
                Node<T>* prev2 = deref(&prev->next);

                // If prev2 is nullptr
                // This happens if it is marked
                if (!prev2) {
                    // If lastMark is false
                    if (!lastMark) {
                        // Calc helpDelete on prev
                        helpDelete(prev);

                        // Set lastMark to true to match helpDelete
                        lastMark = true;
                    }

                    // Update prev2
                    prev2 = derefD(&prev->prev);

                    // Release reference from prev
                    releaseNode(prev);

                    // Update prev to prev2
                    prev = prev2;

                    // Safety check after assignment
                    if (!prev) {
                        break;
                    }

                    // Go into the next loop iteration
                    continue;
                }

                // If prev2 is not equal to node
                // prev2 == node is the expected behavior
                if (prev2 != node) {
                    // Set lastMark to false
                    lastMark = false;

                    // Release reference from prev
                    releaseNode(prev);

                    // Set prev equal to prev2
                    // This is done for backtracking
                    prev = prev2;

                    // Go into the next loop iteration
                    continue;
                }

                // Release reference from prev2
                releaseNode(prev2);

                // Created expected value
                MarkedPtr<T> expected(node, false);

                // Safety check: ensure prev and next are still valid
                if (!prev || !next) {
                    break;
                }

                // Try to atomically update prev's next pointer to next, if it still points to node
                if (prev->next.compare_exchange_weak(expected, MarkedPtr<T>(next, false))) {
                    // Increment next refCount
                    copy(next);

                    // Release node
                    releaseNode(node);

                    // Break out of loop
                    break;
                }

                // Yield the thread before going to next loop iteration
                std::this_thread::yield();
            }

            // Release prev and next
            releaseNode(prev);
            releaseNode(next);
        }

        // Tries to break cross references between the given node and any of the nodes it references
        // This is done by repeatedly updating the prev and next point as long as they reference a fully marked node
        void removeCrossReference(Node<T>* node) {
            // Safety check: return if node is null or retired
            if (!node || node->isRetired.load()) {
                return;
            }

            while (true) {
                // Retrieve node->prev pointer
                Node<T>* prev = node->prev.load().getPtr();

                // Safety check for prev pointer
                if (prev && prev->next.load().getMark()) {
                    // Retrieve prev->prev
                    Node<T>* prev2 = derefD(&prev->prev);

                    // Replace node->prev with marked node->prev->prev
                    node->prev.store(MarkedPtr<T>(prev2, true));

                    // Release old node->prev
                    releaseNode(prev);

                    // Go into the next loop iteration
                    continue;
                }

                // Retrieve node->next pointer
                Node<T>* next = node->next.load().getPtr();

                // Safety check for next pointer
                if (next && next->next.load().getMark()) {
                    // Retrieve next->next
                    Node<T>* next2 = derefD(&next->next);

                    // Replace node->next with marked node->next->next
                    node->next.store(MarkedPtr<T>(next2, true));

                    // Release old node->next
                    releaseNode(next);

                    // Go into the next loop iteration
                    continue;
                }

                // Break from loop
                break;
            }
        }

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

        // Function to push to the left side of the queue
        // Returns a pointer to the node incase it is needed later
        // Creation of a node requires a memory block from an external memory pool
        Node<T>* pushLeft(T data, GenericMemoryPool* memoryPool) {
            // Create node object
            Node<T>* node = createNode(data, memoryPool);

            // Copy head
            Node<T>* prev = copy(head);

            // Gets pointer to current prev->next
            // node will take it's place
            // Will be nullptr if prev->next is marked or null
            Node<T>* next = deref(&prev->next);

            while (true) {
                // Safety check: if next is null, try again
                if (!next) {
                    next = deref(&prev->next);
                    std::this_thread::yield();
                    continue;
                }

                // if prev->next does not equal unmarked next
                if (prev->next.load() != MarkedPtr<T>(next, false)) {
                    // Release current next
                    releaseNode(next);
                    
                    // Redeclare it
                    next = deref(&prev->next);

                    // Go into the next loop iteration
                    continue;
                }

                // Set values for node->prev and node->next
                // These pointers are unmarked
                node->prev.store(MarkedPtr<T>(prev, false));
                node->next.store(MarkedPtr<T>(next, false));

                // Set expected value
                MarkedPtr<T> expected(next, false);

                // Try to atomically update prev->next pointer to next, if it still points to unmarked next
                if (prev->next.compare_exchange_weak(expected, MarkedPtr<T>(node, false))) {
                    // Break out of loop
                    break;
                }

                // Yield the thread before going to next loop iteration
                std::this_thread::yield();
            }

            // Run pushCommon to fully insert node into the queue
            pushCommon(node, next);

            // Return node
            return node;
        }

        // Function to push to the right side of the queue
        // Returns a pointer to the node incase it is needed later
        // Creation of a node requires a memory block from an external memory pool
        Node<T>* pushRight(T data, GenericMemoryPool* memoryPool) {
            // Increment refCount of head
            Node<T>* next = copy(tail);

            // Gets pointer to current prev->next
            // node will take it's place
            // Will be nullptr if prev->next is marked or null
            Node<T>* prev = deref(&next->prev);

            // Create node object
            Node<T>* node = createNode(data, memoryPool);

            while (true) {
                // Safety check: if prev is null, try again
                if (!prev) {
                    prev = deref(&next->prev);
                    std::this_thread::yield();
                    continue;
                }

                // if prev->next does not equal unmarked next
                if (prev->next.load() != MarkedPtr<T>(next, false)) {
                    // Release current prev
                    releaseNode(prev);
                    
                    // Redeclare it from next->prev
                    prev = deref(&next->prev);

                    // Go into the next loop iteration
                    continue;
                }

                // Set values for node->prev and node->next
                // These pointers are unmarked
                node->prev.store(MarkedPtr<T>(prev, false));
                node->next.store(MarkedPtr<T>(next, false));

                // Set expected value
                MarkedPtr<T> expected(next, false);

                // Try to atomically update prev->next pointer to next, if it still points to unmarked next
                if (prev->next.compare_exchange_weak(expected, MarkedPtr<T>(node, false))) {
                    // Break out of loop
                    break;
                }

                // Yield the thread before going to next loop iteration
                std::this_thread::yield();
            }

            // Run pushCommon to fully insert node into the queue
            pushCommon(node, next);

            // Return node
            return node;
        }

        // Function to pop from the left side of the queue
        // Returns the data from the node or nothing
        std::optional<T> popLeft() {
            // Start traversal from head
            Node<T>* prev = copy(head);
            
            while (true) {
                // Try to get the next node from current position
                Node<T>* next = deref(&prev->next);
                
                // If next is null or tail, queue is empty
                if (!next || next == tail) {
                    return std::nullopt;
                }
                
                // Check if next node is marked for deletion
                MarkedPtr<T> nextLink = next->next.load();
                if (nextLink.getMark()) {
                    // Node is being deleted, help complete deletion
                    helpDelete(next);
                    
                    // Try to repair connection and continue traversal
                    Node<T>* afterNext = derefD(&next->next);
                    if (afterNext) {
                        helpInsert(prev, afterNext);
                        releaseNode(afterNext);
                    }
                    
                    releaseNode(next);
                    std::this_thread::yield();

                    // Continue into next loop iteration
                    continue;
                }
                
                // Try to mark next node for deletion (our pop operation)
                if (next->next.compare_exchange_weak(nextLink, MarkedPtr<T>(nextLink.getPtr(), true))) {
                    // Successfully marked node for deletion
                    T data = next->data;
                    
                    // Help complete the deletion process
                    helpDelete(next);
                    
                    // Connect prev to the node after next
                    Node<T>* afterNext = derefD(&next->next);
                    if (afterNext) {
                        helpInsert(prev, afterNext);
                        releaseNode(afterNext);
                    }
                    
                    // Clean up cross references
                    removeCrossReference(next);
                    
                    // Release the deleted node
                    releaseNode(next);
                    
                    // Release prev reference
                    releaseNode(prev);
                    
                    return data;
                }
                
                // CAS failed, the node changed while we were working
                // Continue from current position without restarting
                releaseNode(next);
                std::this_thread::yield();
            }
        }

        // Function to pop from the right side of the queue
        // Returns the data from the node or nothing
        std::optional<T> popRight() {
            Node<T>* next = copy(tail);
            
            while (true) {
                // Get the previous node from current position
                Node<T>* prev = deref(&next->prev);
                
                // If prev is null or we've reached the head, queue is truly empty
                if (!prev || prev == head) {
                    releaseNode(next);
                    return std::nullopt;
                }
                
                // Check if this node is already marked for deletion
                MarkedPtr<T> prevNext = prev->next.load();
                if (prevNext.getMark()) {
                    // Help complete the deletion of this marked node
                    helpDelete(prev);
                    releaseNode(prev);

                    // Go into next loop iteration
                    continue;
                }
                
                // Check if the structure is consistent
                if (prevNext.getPtr() != next) {
                    // Structure is inconsistent, help repair it
                    helpInsert(prev, next);
                    releaseNode(prev);

                    // Go into next loop iteration
                    continue;
                }
                
                // Try to mark this node for deletion (this is our pop operation)
                MarkedPtr<T> expected(next, false);
                if (prev->next.compare_exchange_weak(expected, MarkedPtr<T>(next, true))) {
                    // Successfully marked the node for deletion
                    T data = prev->data;
                    
                    // Help complete the deletion
                    helpDelete(prev);
                    
                    // Get the node before the one we're deleting
                    Node<T>* beforePrev = derefD(&prev->prev);
                    
                    // Connect beforePrev to next, bypassing the deleted node
                    if (beforePrev) {
                        helpInsert(beforePrev, next);
                        releaseNode(beforePrev);
                    }
                    
                    // Clean up cross references
                    removeCrossReference(prev);
                    
                    // Release references
                    releaseNode(prev);
                    releaseNode(next);
                    
                    return data;
                }
                
                // CAS failed, the node changed while we were working
                releaseNode(prev);
                std::this_thread::yield();
            }
        }

        // Function to remove a given node
        // Based off of popRight
        // Returns the data from the node or nothing
        std::optional<T> removeNode(Node<T>* node) {
            // Return nullopt if node is invalid or a dummy node
            if (!node || node->isDummy) {
                return std::nullopt;
            }

            // Check if already marked for deletion
            MarkedPtr<T> nextLink = node->next.load();
            if (nextLink.getMark()) {
                return std::nullopt;
            }

            // Try to mark the node for deletion
            if (node->next.compare_exchange_weak(nextLink, MarkedPtr<T>(nextLink.getPtr(), true))) {
                // Successfully marked node for deletion
                T data = node->data;
                
                // Help complete the deletion
                helpDelete(node);
                
                // Clean up cross references
                removeCrossReference(node);
                
                return data;
            }
            
            // Failed to mark for deletion
            return std::nullopt;
        }
};

#endif
