#ifndef QUEUE_H
#define QUEUE_H

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <thread>
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

        // Head and tail node pointers
        Node<T>* head;
        Node<T>* tail;

        // Returns a node pointer and increments the reference count
        Node<T>* copy(Node<T>* node) {
            if (node) {
            // Increment counter while avoiding contention
                node->refCount.fetch_add(1, std::memory_order_relaxed);
            }
            return node;
        }

        // Predeclare for usage in terminateNode
        void releaseNode(Node<T>* node);

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
            
            // Return nullptr if mark is set
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

        // Predeclare for usage in pushCommon
        Node<T>* helpInsert(Node<T>* prev, Node<T>* node);

        // Common logic for push operations
        // Pushed to back if back is set to true
        void pushCommon(Node<T>* node, Node<T>* next) {
            while (true) {
                // Retrieve next->prev
                MarkedPtr<T> link = next->prev;

                // If link is marked or node->next does not equal unmarked next
                if (link.getMark() || node->next != MarkedPtr<T>(next, false)) {
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

                    // Yield the thread before going to next loop iteration
                    std::this_thread::yield();
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
                    
                    // Yield the thread before going to next loop iteration
                    std::this_thread::yield();
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

                    // Yield the thread before going to next loop iteration
                    std::this_thread::yield();
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

                    // Yield the thread before going to next loop iteration
                    std::this_thread::yield();
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

                    // Yield the thread before going to next loop iteration
                    std::this_thread::yield();
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
        // This is done by repeatdely updating the prev and next point as long as they reference a fully marked node
        void removeCrossReference(Node<T>* node) {
            while (true) {
                // Retrieve node->prev marked pointer
                MarkedPtr<T> prev = node->prev.load();

                // If prev is marked
                if (prev.getMark()) {
                    // Retrieve prev->prev
                    Node<T>* prev2 = derefD(&prev.getPtr()->prev);

                    // Replace node->prev with marked node->prev->prev
                    node->prev.store(MarkedPtr<T>(prev2, true));

                    // Release old node->prev
                    releaseNode(prev.getPtr());

                    // Yield the thread before going to next loop iteration
                    std::this_thread::yield();
                    continue;
                }

                // Retrieve node->next marked pointer
                MarkedPtr<T> next = node->next.load();

                // If next is marked
                if (next.getMark()) {
                    // Retrieve next->next
                    Node<T>* next2 = derefD(&next.getPtr()->next);

                    // Replace node->next with marked node->next->next
                    node->next.store(MarkedPtr<T>(next2, true));

                    // Release old node->next
                    releaseNode(next.getPtr());

                    // Yield the thread before going to next loop iteration
                    std::this_thread::yield();
                    continue;
                }

                // Break from loop
                break;
            }
        }

    public:
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
};

#endif