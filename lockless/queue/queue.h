#ifndef QUEUE_H
#define QUEUE_H

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <thread>
#include <optional>
#include "../memoryPool/memoryPool.h"

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
// Custom destructor may possibly be needed but not at the moment
// It will be needed if there is memory management to be done
template <typename T>
struct Node {
    std::atomic<MarkedPtr<T>> prev;
    std::atomic<MarkedPtr<T>> next;
    T data;

    // Number of threads that are currently holding a reference to this node
    std::atomic<size_t> refCount;

    // Track if this is a dummy node or not
    bool isDummy;

    // For memory pool allocation if applicable
    void* memoryBlock;

    // Used if no parameters are provided
    Node(void* memoryBlock) : prev(MarkedPtr<T>(nullptr, false)), next(MarkedPtr<T>(nullptr, false)), 
        data(), refCount(1), isDummy(true), memoryBlock(memoryBlock) {}
    
    // Used if parameters are provided
    Node(void* memoryBlock, const T& val) : prev(MarkedPtr<T>(nullptr, false)), next(MarkedPtr<T>(nullptr, false)), 
        data(val), refCount(1), isDummy(false), memoryBlock(memoryBlock) {}
};

template<typename T>
class LocklessQueue {
    private:
        // Node memory pool
        // Size must be assigned in constructor
        MemoryPool pool; 

        // Returns a node pointer and increments the reference count
        Node<T>* copy(Node<T>* node) {
            if (node) {
            // Increment counter while avoiding contention
                node->refCount.fetch_add(1, std::memory_order_relaxed);
            }
            return node;
        }

        // Calls Node destructor and deletes it from memory
        void terminateNode(Node<T>* node) {
            if (!node) {
                return;
            }

            releaseNode(node->prev.load().getPtr());
            releaseNode(node->next.load().getPtr());

            // Explicitly call destructor for Node<T>
            node->~Node<T>();

            // Handle memory block
            pool.deallocate(node->memoryBlock);
        }

        // Self explanatory
        void releaseNode(Node<T>* node) {
            if (!node) {
                return;
            }

            // Subtracts 1 from refCount atomically
            // If refCount is one the node may be terminated as refCount will drop to 0
            if (node->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                terminateNode(node);
            }
        }

        // Increments the node's reference count so that the node won't be deleted while being used
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
            // Load node from atomic MarkedPtr
            Node<T>* node = ptr->load(std::memory_order_acquire).getPtr();

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
                    if (node->next.load().getMark()) {
                        copy(node);
                        helpInsert(head, node);
                        releaseNode(node);
                    }

                    // Decrement node refCount
                    releaseNode(node);
                    
                    // Break out of the loop
                    break;
                }

                // Yield the thread if unsuccessful
                std::this_thread::yield();
            }

            // Release references to next and node
            releaseNode(node);
            releaseNode(next);
        }

        // Helps complete insert operation that was interrupted
        Node<T>* helpInsert(Node<T>* prev, Node<T>* node) {
            // Used to remember a node that separates prev and node if we need to backtrack
            Node<T>* last = nullptr;

            while (true) {
                // Retrieves safe to use copy of prev->next
                Node<T>* prev2 = deref(&prev->next);

                // if prev2 is invalid
                if (!prev2) {
                    // If we previously saved a valid node that separates prev and node
                    if (last) {
                        // Atomically mark prev's prev pointer
                        markPrev(prev);

                        // Get a safe pointer to prev's next again
                        Node<T>* next2 = derefD(&prev->next);

                        // Prepare an expected value for CAS
                        MarkedPtr<T> expected(prev, false);

                        // Try to atomically update last's next pointer to next2, if it still points to prev
                        if (last->next.compare_exchange_weak(expected, MarkedPtr<T>(next2, false))) {
                            // If successful, release reference to prev as it's now fixed or transitioned
                            releaseNode(prev);
                        } else {
                            // If CAS failed, release reference to next2
                            // It will be reinstated next cycle if needed
                            releaseNode(next2);
                        }

                        // Release reference to prev regardless
                        releaseNode(prev);

                        // Set prev to last
                        prev = last;

                        // Reset last to nullptr
                        last = nullptr;

                    } else {
                        // If last does not exist try moving back to prev->prev
                        prev2 = derefD(&prev->prev);

                        // Release the former prev
                        releaseNode(prev);

                        // Move backwards
                        prev = prev2;
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
                    // If last exists release it
                    if (last) {
                        releaseNode(last);
                    }

                    // Save the current prev for backtracking
                    last = prev;

                    // Move forward in the queue
                    prev = prev2;
                    
                    // Go into the next loop iteration
                    continue;
                }

                // It is assumed here that prev2 == node so a reference could be released
                // This is because a reference added to prev2 added it to node
                releaseNode(prev2);

                // If link already points to prev the process is complete
                if (link.getPtr() == prev) {
                    // Success, break out of the loop
                    break;
                }

                // If prev->next still equals node and CAS is successfully completed
                // CAS is needed to prev and node together, as the field above asserted they aren't
                if (prev->next.load() == MarkedPtr<T>(node, false) && 
                    node->prev.compare_exchange_weak(link, MarkedPtr<T>(prev, false))) {
                    // Increment prev's refCount for memory safety
                    copy(prev);

                    // Release the reference to the old previous node
                    releaseNode(link.getPtr());

                    // If prev's prev pointer is not marked we are done
                    if (!prev->prev.load().getMark()) {
                        // Break out of the loop
                        break;
                    }
                }
                // Yield the thread if unsuccessful
                std::this_thread::yield();
            }

            // Clean up remaining saved node if it exists
            if (last) {
                releaseNode(last);
            }

            // Return the node that now precedes the inserted node
            return prev;
        }

        // Help complete a delete operation that may have been interrupted
        void helpDelete(Node<T>* node) {
            // Mark node's prev connection
            markPrev(node);

            // Initialize last as nullptr
            Node<T>* last = nullptr;

            // Retrieve prev and next and increment refCount
            Node<T>* prev = derefD(&node->prev);
            Node<T>* next = derefD(&node->next);

            while (true) {
                // if prev and next are equal
                if (prev == next) {
                    // Break out of loop
                    break;
                }

                // If next->next is marked
                if (next->next.load().getMark()) {
                    // Mark node through the connection from next
                    markPrev(next);

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
                    // If last is not nullptr
                    if (last) {
                        // Mark prev->prev
                        markPrev(prev);

                        // Retrieve node through prev->next
                        Node<T>* next2 = derefD(&prev->next);

                        // Created expected value
                        MarkedPtr<T> expected(prev, false);

                        // Try to atomically update last's next pointer to next2, if it still points to prev
                        if (last->next.compare_exchange_weak(expected, MarkedPtr<T>(next2, false))) {
                            // If successful, release reference to prev as it's now fixed or transitioned
                            releaseNode(prev);
                        } else {
                            // If CAS failed, release reference to next2
                            // It will be reinstated next cycle if needed
                            releaseNode(next2);
                        }

                        // Release prev
                        releaseNode(prev);

                        // Set prev to last
                        prev = last;

                        // Reset last to nullptr
                        last = nullptr;
                    } else {
                        // Retrieve prev->prev
                        prev2 = derefD(&prev->prev);

                        // Release prev
                        releaseNode(prev);

                        // Set prev to prev2
                        // This is done for backtracking
                        prev = prev2;
                    }

                    // Go into the next loop iteration
                    continue;
                }

                // If prev2 is not equal to node
                // prev2 == node is the expected behavior
                if (prev2 != node) {
                    // If last is not nullptr
                    if (last) {
                        // Release last
                        releaseNode(last);
                    }

                    // Set last equal to prev
                    last = prev;

                    // Set prev equal to prev2
                    // This is done for backtracking
                    prev = prev2;

                    // Go into the next loop iteration
                    continue;
                }

                // Release prev2
                releaseNode(prev2);

                // Created expected value
                MarkedPtr<T> expected(node, false);

                // Try to atomically update prev's next pointer to next, if it still points to node
                if (prev->next.compare_exchange_weak(expected, MarkedPtr<T>(next, false))) {
                    // Increment node refCount
                    copy(node);

                    // Release node
                    releaseNode(node);

                    // Break out of loop
                    break;
                }

                // Yield the thread before going to next loop iteration
                std::this_thread::yield();
            }

            // If last is not nullptr then release it
            if (last) {
                releaseNode(last);
            }

            // Release prev and next
            releaseNode(prev);
            releaseNode(next);
        }

        // Tries to break cross references between the given node and any of the nodes it references
        // This is done by repeatedly updating the prev and next point as long as they reference a fully marked node
        void removeCrossReference(Node<T>* node) {
            while (true) {
                // Retrieve node->prev marked pointer
                MarkedPtr<T> prev = node->prev.load();

                // If prev->next is marked
                // Make sure prev's pointer is not nullptr
                if (prev.getPtr() && !prev.getPtr()->isDummy && prev.getPtr()->next.load().getMark()) {
                    // Retrieve prev->prev
                    Node<T>* prev2 = derefD(&prev.getPtr()->prev);

                    // Replace node->prev with marked node->prev->prev
                    node->prev.store(MarkedPtr<T>(prev2, true));

                    // Release old node->prev
                    releaseNode(prev.getPtr());

                    // Go into the next loop iteration
                    continue;
                }

                // Retrieve node->next marked pointer
                MarkedPtr<T> next = node->next.load();

                // If next->next is marked
                // Make sure next's pointer is not nullptr
                if (next.getPtr() && !next.getPtr()->isDummy && next.getPtr()->next.load().getMark()) {
                    // Retrieve next->next
                    Node<T>* next2 = derefD(&next.getPtr()->next);

                    // Replace node->next with marked node->next->next
                    node->next.store(MarkedPtr<T>(next2, true));

                    // Release old node->next
                    releaseNode(next.getPtr());

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

        LocklessQueue(size_t numNodes)
            // 2 is added to account for head and tail dummy nodes
            : pool(sizeof(Node<T>), numNodes + 2) {

            // Allocate memory for head and tail
            void* headBlock = pool.allocate();
            void* tailBlock = pool.allocate();

            // Initialize head and tail
            head = new (headBlock) Node<T>(headBlock);
            tail = new (tailBlock) Node<T>(tailBlock);

            // Connect nodes
            head->next.store(MarkedPtr<T>(tail, false));
            tail->prev.store(MarkedPtr<T>(head, false));
        }

        ~LocklessQueue() {
            // Cycle through all remaining items and release them
            Node<T>* curr = head->next.load().getPtr();
            Node<T>* next;
            while (curr != tail) {
                next = curr->next.load().getPtr();
                releaseNode(curr);
                curr = next;
            }
            releaseNode(head);
            releaseNode(tail);
        }

        // Returns a node created with the given data
        Node<T>* createNode(T data) {
            // Allocate memory
            void* block = pool.allocate();

            // Create node object
            Node<T>* node = new (block) Node<T>(block, data);

            // Return node
            return node;
        }

        // Function to push to the left side of the queue
        // Returns a pointer to the node incase it is needed later
        Node<T>* pushLeft(T data) {
            // Create node object
            Node<T>* node = createNode(data);

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
        Node<T>* pushRight(T data) {
            // Create node object
            Node<T>* node = createNode(data);

            // Increment refCount of head
            Node<T>* next = copy(tail);

            // Gets pointer to current prev->next
            // node will take it's place
            // Will be nullptr if prev->next is marked
            Node<T>* prev = deref(&next->prev);

            while (true) {
                // if prev->next does not equal unmarked next
                if (prev->next.load() != MarkedPtr<T>(next, false)) {
                    // prev->next is broken so run helpInsert on it
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

                // Check if node is nullptr or node->next is incorrect
                if (!node) {
                    // node is marked but not yet deleted
                    // This means another thread is already trying to delete it
                    // call helpDelete on prev to help the process along
                    // This will update connected nodes, deleting marked ones
                    helpDelete(node);

                    // release reference to node
                    releaseNode(node);

                    // Go into the next loop iteration
                    continue;

                // Check if queue is empty
                } else if (node == tail) {
                    // Release head and tail
                    // They were incremented above
                    releaseNode(head);
                    releaseNode(tail);

                    // Return nullptr as the queue is empty
                    return std::nullopt;
                }

                // Get MarkedPtr of node->next
                MarkedPtr<T> link = node->next;

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

            std::cout << prev->next.load().getPtr()->prev.load().getPtr() << std::endl;

            // Break possible cyclic references that include node
            removeCrossReference(node);

            std::cout << prev->next.load().getPtr()->prev.load().getPtr() << std::endl;

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

                // Check if node is nullptr
                if (!node) {
                    // node is marked but not yet deleted
                    // This means another thread is already trying to delete it
                    // call helpDelete on next to help the process along
                    // This will update connected nodes, deleting marked ones
                    helpDelete(next);

                    // Update node value
                    // Must be done here as it isn't updated within the loop
                    node = deref(&next->prev);

                    // Go into the next loop iteration
                    continue;

                // Check if node->next is incorrect
                } else if (node->next.load() != MarkedPtr<T>(next, false)) {
                    // Call helpInsert to update the prev pointer of next
                    node = helpInsert(node, next);

                    // Go into the next loop iteration
                    continue;

                // Check if queue is empty
                } else if (node == head) {
                    // Release head and tail
                    // They were incremented above
                    releaseNode(head);
                    releaseNode(tail);

                    // Return nullptr as the queue is empty
                    return std::nullopt;
                }

                // Try to atomically update node->next pointer to marked pointer to next, if it is still an unmarked pointer to next
                if (node->next.compare_exchange_weak(MarkedPtr<T>(next, false), MarkedPtr<T>(next, true))) {
                    // Call helpDelete to help remove the node now that it is marked
                    helpDelete(node);

                    // Retrieve prev
                    Node<T>* prev = derefD(&node->prev);

                    // Connect prev and next
                    // If prev is marked helpInsert will deal with it
                    helpInsert(prev, next);

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
        // node must not be nullptr or dummy node
        // Returns the data from the node or nothing
        std::optional<T> removeNode(Node<T>* node) {

            // Check for head and tail remove
            if (node->prev.load() == MarkedPtr<T>(head, false)) {
                // Removing from head, return popLeft
                return popLeft();
            } else if (node->next.load() == MarkedPtr<T>(tail, false)) {
                // Removing from tail, return popRight
                return popRight();
            }

            // To be returned
            T data = node->data;

            while (true) {

                // Get MarkedPtr of node->prev
                MarkedPtr<T> link1 = node->prev;

                // Get MarkedPtr of node-next
                MarkedPtr<T> link2 = node->next;

                // If link1 or link2 are marked
                if (link1.getMark() || link2.getMark()) {
                    // Help unlink node from logically delete node->next
                    helpDelete(node);

                    // Go into the next loop iteration
                    continue;
                }

                // Try to atomically update node->next pointer to marked link2, if it still points to unmarked link2
                if (node->next.compare_exchange_weak(link2, MarkedPtr<T>(link2.getPtr(), true))) {
                    // Call helpDelete to help remove the node now that it's connection is marked
                    helpDelete(node);

                    // Retrieve prev
                    // Was checked above for if it was marked
                    Node<T>* prev = derefD(&node->prev);

                    // Retrieve next
                    // Was checked above for if it was marked
                    Node<T>* next = derefD(&node->next);

                    // Connect prev and next
                    helpInsert(prev, next);

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

            // Return data
            return data;
        }
};

#endif