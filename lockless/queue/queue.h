#ifndef QUEUE_H
#define QUEUE_H

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <thread>
#include <optional>
#include <iostream>
#include "../memoryPool/memoryPool.h"
#include "../../hazardPointers/hazardRetire.h"
#include "../../hazardPointers/hazardPointers.h"

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

    // Number of threads that are currently holding a reference to this node
    std::atomic<size_t> refCount;

    // Track if this is a dummy node or not
    bool isDummy;

    // For memory pool allocation
    void* memoryBlock;

    // Owner memory pool
    // Generic reference to MmemoryPool
    GenericMemoryPool* ownerPool;

    // Used if no parameters are provided
    Node(GenericMemoryPool* ownerPool, void* memoryBlock) : prev(MarkedPtr<T>(nullptr, false)), next(MarkedPtr<T>(nullptr, false)), 
        data(), refCount(1), isDummy(true), memoryBlock(memoryBlock), ownerPool(ownerPool) {}
    
    // Used if parameters are provided
    Node(GenericMemoryPool* ownerPool, void* memoryBlock, const T& val) : prev(MarkedPtr<T>(nullptr, false)), next(MarkedPtr<T>(nullptr, false)), 
        data(val), refCount(1), isDummy(false), memoryBlock(memoryBlock), ownerPool(ownerPool) {}
};

template<typename T>
class LocklessQueue {
    private:
        // Dummy node memory pool
        // Size must be assigned in constructor
        MemoryPool<sizeof(Node<T>), 2> pool; 

        // Copy sets a hazard pointer and returns a node pointer
        Node<T>* copy(Node<T>* node) {
            // If node is valid and not a dummy node
            if (node && !node->isDummy) {
                // Set hazard pointer
                setHazardPointer(node);
            }

            // Return node
            return node;
        }

        // Calls Node destructor and deletes it from memory
        // destrurctor flag should be true if and only if called from the destructor
        void terminateNode(Node<T>* node, bool destructor=false) {
            // If node is nullptr return
            if (!node) {
                return;
            }

            if (!destructor) {
                releaseNode(node->prev.load().getPtr());
                releaseNode(node->next.load().getPtr());
            }

            GenericMemoryPool* ownerPool = node->ownerPool;

            // Explicitly call destructor for Node<T>
            // May not be needed but good to have
            node->~Node<T>();

            // Handle memory block
            ownerPool->deallocate(node->memoryBlock);
        }

        // Self explanatory
        void releaseNode(Node<T>* node) {
            // Return if node is invalid or is a dummy node
            // Dummy nodes should never be deleted during runtime and should thus be ignored
            if (!node || node->isDummy) {
                return;
            }

            // Release protection of node
            removeHazardPointer(node);

            // If node is no longer protected retire it
            if (!isHazard(node)) {
                retireObj(node, staticTerminateNode);
            }
        }

        // Adds node into a hazard pointer for the given thread
        // Returns nullptr if mark is set and returns node pointer otherwise
        Node<T>* deref(std::atomic<MarkedPtr<T>>* ptr) {
            // Load MarpedPtr object from atomic
            MarkedPtr<T> mPtr = ptr->load(std::memory_order_acquire);

            // Return nullptr if mark is set or ptr is nullptr
            if (mPtr.getMark()) {
                return nullptr;
            }

            // Load node from MarkedPtr
            Node<T>* node = mPtr.getPtr();

            // If node is invalid copy handles it
            return copy(node);
        }

        // Like deref but a pointer is always returned
        Node<T>* derefD(std::atomic<MarkedPtr<T>>* ptr) {
            // Load MarpedPtr object from atomic
            MarkedPtr<T> mPtr = ptr->load(std::memory_order_acquire);

            // Load node from MarkedPtr
            Node<T>* node = mPtr.getPtr();

            // If node is invalid copy handles it
            return copy(node);
        }

        // Atomically sets deletion marker on prev pointer
        // Tells other threads not to use this connection
        void markPrev(Node<T>* node) {
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

                    // Decrement link refCount
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
            // Get mark of last connection
            bool lastMark = prev->next.load().getMark();

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
            // Mark node's prev connection
            markPrev(node);

            // Retrieve prev and next and increment refCount
            Node<T>* prev = derefD(&node->prev);
            Node<T>* next = derefD(&node->next);

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

                // If next->next is marked
                if (next->next.load().getMark()) {
                    // Retrieve next->next and increment refCount
                    Node<T>* next2 = derefD(&next->next);

                    // Release next
                    releaseNode(next);

                    // Set next to next2
                    next = next2;

                    // Go into the next loop iteration
                    continue;
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
            while (true) {
                // Retrieve node->prev pointer
                Node<T>* prev = node->prev.load().getPtr();

                // If prev->next is marked
                if (prev->next.load().getMark()) {
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

                // If next->next is marked
                if (next->next.load().getMark()) {
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
            // Allocate memory from pool
            void* memoryBlock = memoryPool->allocate();

            // Create node object
            Node<T>* node = new (memoryBlock) Node<T>(memoryPool, memoryBlock, data);

            // Return node
            return node;
        }

        // Function to push to the left side of the queue
        // Returns a pointer to the node incase it is needed later
        // Creation of a node requires a memory block from an external memory pool
        Node<T>* pushLeft(T data, GenericMemoryPool* memoryPool) {
            // Create node object
            Node<T>* node = createNode(data, memoryPool);

            // Increment refCount of head
            Node<T>* prev = copy(head);

            // Gets pointer to current prev->next
            // node will take it's place
            // Will be nullptr if prev->next is marked
            Node<T>* next = deref(&prev->next);

            while (true) {
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
                    // Increment node refCount
                    copy(node);

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
            // Create node object
            Node<T>* node = createNode(data, memoryPool);

            // Increment refCount of head
            Node<T>* next = copy(tail);

            // Gets pointer to current prev->next
            // node will take it's place
            // Will be nullptr if prev->next is marked
            Node<T>* prev = deref(&next->prev);

            while (true) {
                // if prev->next does not equal unmarked next
                if (prev->next.load() != MarkedPtr<T>(next, false)) {
                    prev = helpInsert(prev, next);

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
                    // Increment node refCount
                    copy(node); 

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
            // To be returned
            T data;

            // Retrieve prev which is head for the left side
            Node<T>* prev = copy(head);

            // Declare outside of loop for later use
            Node<T>* node;

            while (true) {
                // Retrieve node
                // Will return nullptr if prev->next is marked
                node = deref(&prev->next);

                // Check if queue is empty
                if (node == tail) {
                    // No decrements needed as head and tail refCounts are irrelevant
                    // Return nullptr as the queue is empty
                    return std::nullopt;
                }

                // Get MarkedPtr of node->next
                MarkedPtr<T> link = node->next.load();

                // If link is marked
                if (link.getMark()) {
                    // Help unlink node from logically delete node->next
                    helpDelete(node);

                    // release node before the next loop iteration
                    // Will be incremented again next loop
                    releaseNode(node);

                    // Go into the next loop iteration
                    continue;
                }

                // Try to atomically update node->next pointer to marked link, if it still points to unmarked link
                if (node->next.compare_exchange_weak(link, MarkedPtr<T>(link.getPtr(), true))) {
                    // Call helpDelete to help remove the node now that it is marked
                    helpDelete(node);

                    // Retrieve next
                    // Was checked above for if it was marked
                    Node<T>* next = derefD(&node->next);

                    // Connect prev and next
                    prev = helpInsert(prev, next);

                    // Decrement prev and next
                    releaseNode(prev);
                    releaseNode(next);

                    // Initialize data
                    data = node->data;

                    // Break out of loop
                    break;
                }

                // release node before the next loop iteration
                // Will be incremented again next loop
                releaseNode(node);
                
                // Yield the thread before going to next loop iteration
                std::this_thread::yield();
            }

            // Break possible cyclic references that include node
            removeCrossReference(node);

            // Fully decrement node for deletion
            releaseNode(node);

            // Return data
            return data;
        }

        // Function to pop from the right side of the queue
        // Returns the data from the node or nothing
        std::optional<T> popRight() {
            // To be returned
            T data;

            // Retrieve next which is tail for the right side
            Node<T>* next = copy(tail);

            // Retrieve node
            // Will return nullptr if prev->next is marked
            Node<T>* node = deref(&next->prev);

            while (true) {

                // Check if node->next is incorrect
                if (node->next.load() != MarkedPtr<T>(next, false)) {
                    // Call helpInsert to update the prev pointer of next to node
                    node = helpInsert(node, next);

                    // Go into the next loop iteration
                    continue;

                // Check if queue is empty
                } else if (node == head) {
                    // Return nullptr as the queue is empty
                    return std::nullopt;
                }

                // Declare expected value
                MarkedPtr<T> expected(next, false);

                // Try to atomically update node->next pointer to marked pointer to next, if it is still an unmarked pointer to next
                if (node->next.compare_exchange_weak(expected, MarkedPtr<T>(next, true))) {
                    // Call helpDelete to help remove the node now that it is marked
                    helpDelete(node);

                    // Retrieve prev
                    Node<T>* prev = derefD(&node->prev);

                    // Connect prev and next
                    // If prev is marked helpInsert will deal with it
                    prev = helpInsert(prev, next);

                    // Decrement prev and next
                    releaseNode(prev);
                    releaseNode(next);

                    // Initialize data
                    data = node->data;

                    // Break out of loop
                    break;
                }
                
                // Yield the thread before going to next loop iteration
                std::this_thread::yield();
            }

            // Break possible cyclic references that include node
            removeCrossReference(node);

            // Fully decrement node for deletion
            releaseNode(node);

            // Return data
            return data;
        }

        // Function to remove a given node
        // Based off of popRight
        // Returns the data from the node or nothing
        std::optional<T> removeNode(Node<T>* node) {
            // Return nullopt if node is invalid or a dummy node
            if (!node || node->isDummy) {
                return std::nullopt;
            }

            // increment node's refCount
            copy(node);

            // To be returned
            T data = node->data;

            while (true) {
                // Retrieve marked pointer to node->next
                MarkedPtr<T> link = node->next.load();

                // If already logically deleted, help finish and return nullopt
                if (link.getMark()) {
                    // Help deletion along
                    helpDelete(node);

                    // Release node and next
                    releaseNode(node);

                    // Return nullopt
                    return std::nullopt;
                }

                // Try to atomically update node->next pointer to marked pointer to next, if it is still an unmarked pointer to next
                if (node->next.compare_exchange_weak(link, MarkedPtr<T>(link.getPtr(), true))) {
                    // Call helpDelete to help remove the node now that it is marked
                    helpDelete(node);

                    // Retrieve node->prev
                    Node<T>* prev = derefD(&node->prev);

                    // Retrieve node->next
                    Node<T>* next = derefD(&node->next);

                    // Connect prev and next
                    // If prev or next are marked helpInsert will deal with it
                    prev = helpInsert(prev, next);

                    // Decrement prev and next
                    releaseNode(prev);
                    releaseNode(next);

                    // Break out of loop
                    break;
                }
                
                // Yield the thread before going to next loop iteration
                std::this_thread::yield();
            }

            // Break possible cyclic references that include node
            removeCrossReference(node);

            // Fully decrement node for deletion
            releaseNode(node);

            // Return data
            return data;
        }
};

#endif