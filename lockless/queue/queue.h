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
    // Return if node is null or is a dummy node
    if (!node || node->isDummy) {
        return;
    }
    
    // Return if node is already retired
    // If node->isRetired is false exchange it for true
    if (node->isRetired.exchange(true)) {
        return;
    }

    // Add the node to this thread's retire list
    // We can safely cast to void* since we checked isDummy above
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
        HazardGuard<Node<T>> DeRefLink(Node<T>* node) {
            // If node is null, return nullptr for safety
            if (!node) {
                return HazardGuard<Node<T>>(nullptr);
            }

            // Dummy nodes don't need protection as they're never deleted
            // Only protect non-dummy nodes
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

            // Always remove our protection first
            removeHazardPointer(node);
            
            // Then try to retire the node if no other threads are protecting it
            // This check is now safe because we've removed our protection
            if (!isHazard(node)) {
                retireQueueNode<T>(node, *this);
            }
        }

        // Set mark on a node given the link
        void setMark(std::atomic<MarkedPtr<T>>* link) {
            // Start infinite loop
            while (true) {
                // Retrieve node
                Node<T>* node = link->load().getPtr();

                // Declare expected value
                MarkedPtr<T> expected(node, false);

                // If node is marked or CAS succeeds break
                if (link->load().getMark() || link->compare_exchange_strong(expected, MarkedPtr<T>(node, true))) {
                    break;
                }
            }
        }

        // Swap references
        // Here to follow the pseudocode
        bool CASRef(std::atomic<MarkedPtr<T>>* link, MarkedPtr<T> oldVal, MarkedPtr<T> newVal) {
            return link->compare_exchange_strong(oldVal, newVal);
        }

        // Common push function
        void pushEnd(Node<T>* node, Node<T>* next) {
            // Start infinite loop
            while (true) {
                // Get next->prev link
                MarkedPtr<T> link1 = next->prev.load();

                // If link1 is marked or node->next does not equal unmarked next break
                if (link1.getMark() || node->next.load() != MarkedPtr<T>(next, false)) {
                    break;
                }

                // next->prev is swapped from link1 to unmarked node
                if (CASRef(&next->prev, link1, MarkedPtr<T>(node, false))) {
                    // If node->prev is makred correct it
                    if (node->prev.load().getMark()) {
                        node = correctPrev(node, next);
                    }

                    // Break out of the loop
                    break;
                }

                // Back off
                std::this_thread::yield();
            }
        }

        // CorrectPrev function from Figure 15 of the paper
        Node<T>* correctPrev(Node<T>* prev, Node<T>* node) {
            // Set lastLink as nullptr
            Node<T>* lastLink = nullptr;

            // Protect prev and node
            HazardGuard<Node<T>> Hprev = DeRefLink(prev);
            HazardGuard<Node<T>> Hnode = DeRefLink(node);

            // lastLink protection
            HazardGuard<Node<T>> Hlast(nullptr);

            // Start infinite loop
            while (true) {
                // Retrieve node-> prev
                MarkedPtr<T> link1 = node->prev.load();

                // Retrieve prev->next
                MarkedPtr<T> Mprev2 = prev->next.load();
                Node<T>* prev2 = Mprev2.getPtr();
                
                // Skip if prev2 is null (shouldn't happen in correct implementation)
                if (!prev2) {
                    break;
                }
                
                HazardGuard<Node<T>> Hprev2 = DeRefLink(prev2);
                prev2 = Hprev2.ptr;

                // If prev2 is marked
                if (Mprev2.getMark()) {
                    // If lastlink is valid
                    if (lastLink) {
                        // Set mark on node
                        setMark(&prev->prev);

                        // Swap references - help complete the deletion
                        if (CASRef(&lastLink->next, MarkedPtr<T>(prev, false), MarkedPtr<T>(prev2, false))) {
                            // Successfully helped complete the deletion
                            // Release the node that was unlinked
                            releaseNode(prev);
                        }

                        // Set prev to lastlink
                        prev = lastLink;

                        // Transfer protection from Hlast to Hprev
                        Hprev = std::move(Hlast);
                        Hlast = HazardGuard<Node<T>>(nullptr);

                        // Clear lastlink
                        lastLink = nullptr;

                        // Continue into next loop iteration
                        continue;
                    }
                    
                    // retrieve prev->prev
                    Hprev2 = DeRefLink(prev->prev.load().getPtr());
                    prev2 = Hprev2.ptr;

                    // Set prev to prev2
                    prev = prev2;

                    // Transfer protection
                    Hprev = std::move(Hprev2);

                    // Continue into next loop iteration
                     continue;
                }

                // If prev2 does not eqaul node
                if (prev2 != node) {
                    // Set lastLink to prev
                    lastLink = prev;

                    // Move up prev2
                    prev = prev2;

                    // Swap protection
                    Hlast = std::move(Hprev);
                    Hprev = std::move(Hprev2);
                }

                // Destroy prev2 protection
                Hprev2 = HazardGuard<Node<T>>(nullptr);

                // Try to swap node->prev from link1 to unmarked prev
                if (CASRef(&node->prev, link1, MarkedPtr<T>(prev, false))) {
                    // If prev->prev is marked then loop again
                    if (prev->prev.load().getMark()) {
                        continue;
                    }

                    // Break out of loop
                    break;
                }

                // Back off
                std::this_thread::yield();
            }

            // Return prev
            return prev;
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

            // Update retire list
            // All protection should be removed by this point
            updateRetireListQueue<T>(*this);
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
        // Based on Figure 9 PushLeft procedure from the paper
        Node<T>* pushLeft(T data, GenericMemoryPool* memoryPool) {
            // create new node
            Node<T>* node = createNode(data, memoryPool);

            // retrieve prev
            HazardGuard<Node<T>> Hprev = DeRefLink(head);
            Node<T>* prev = Hprev.ptr;

            // retrieve next
            HazardGuard<Node<T>> Hnext = DeRefLink(prev->next.load().getPtr());
            Node<T>* next = Hnext.ptr;

            // While true
            while (true) {
                // StoreRef(&node.prev,⟨prev,false⟩)
                node->prev.store(MarkedPtr<T>(prev, false));

                // StoreRef(&node.next,⟨next,false⟩)
                node->next.store(MarkedPtr<T>(next, false));

                // Attempt to swap prev->next from unmarked next to unmarked node
                MarkedPtr<T> expected(next, false);
                if (CASRef(&prev->next, expected, MarkedPtr<T>(node, false))) {
                    break;
                }

                // Transfer protection from old to new next
                Hnext = DeRefLink(prev->next.load().getPtr());
                
                // redefine next
                next = Hnext.ptr;

                // Back-Off
                std::this_thread::yield();
            }

            // Finish push
            pushEnd(node, next);

            return node;
        }

        // Function to push to the right side of the queue  
        // Based on Figure 9 PushRight procedure from the paper
        Node<T>* pushRight(T data, GenericMemoryPool* memoryPool) {
            // create new node
            Node<T>* node = createNode(data, memoryPool);

            // retrieve next
            HazardGuard<Node<T>> Hnext = DeRefLink(tail);
            Node<T>* next = Hnext.ptr;

            // retrieve prev
            HazardGuard<Node<T>> Hprev = DeRefLink(next->prev.load().getPtr());
            Node<T>* prev = Hprev.ptr;

            // Loop infinitely
            while (true) {
                // Connect node to prev
                node->prev.store(MarkedPtr<T>(prev, false));

                // Connect node to next
                node->next.store(MarkedPtr<T>(next, false));

                // Attempt to swap prev->next from unmarked next to unmarked node
                MarkedPtr<T> expected(next, false);
                if (CASRef(&prev->next, expected, MarkedPtr<T>(node, false))) {
                    break;
                }

                // Correct the error
                prev = correctPrev(prev, next);

                // Back-Off
                std::this_thread::yield();
            }

            // Finish push
            pushEnd(node, next);

            return node;
        }

        // Function to pop from the left side of the queue
        // Based on Figure 10 PopLeft function from the paper
        std::optional<T> popLeft() {
            // Declare prev
            HazardGuard<Node<T>> Hprev = DeRefLink(head);
            Node<T>* prev = Hprev.ptr;

            // Create variable to hold return data
            T data;

            // Hold node
            Node<T>* node;

            // Loop infinitely
            while (true) {
                // Retrieve node
                MarkedPtr<T> Mnode = prev->next.load();
                HazardGuard<Node<T>> Hnode = DeRefLink(Mnode.getPtr());
                node = Hnode.ptr;

                // If queue is empty return nullopt
                if (node == tail) {
                    return std::nullopt;
                }

                // Declare next
                MarkedPtr<T> Mnext = node->next.load();
                HazardGuard<Node<T>> Hnext = DeRefLink(Mnext.getPtr());
                Node<T>* next = Hnext.ptr;

                // If next is marked
                if (Mnext.getMark()) {
                    // mark node->prev
                    setMark(&node->prev);

                    // Switch prev->next from node to unmarked next
                    if (CASRef(&prev->next, MarkedPtr<T>(node, false), MarkedPtr<T>(next, false))) {
                        // Successfully helped complete the deletion
                        // Release the node that was unlinked
                        releaseNode(node);
                    }

                    // Go into next loop iteration
                    continue;
                }

                // if node->next can be swapped from unmarked next to marked next
                if (CASRef(&node->next, Mnext, MarkedPtr<T>(next, true))) {
                    // Connect prev->next directly to next (bypassing deleted node)
                    CASRef(&prev->next, Mnode, MarkedPtr<T>(next, false));

                    // Connect prev and next
                    prev = correctPrev(prev, next);

                    // Store data
                    data = node->data;

                    // Break out of loop
                    break;
                }

                // Back-Off
                std::this_thread::yield();
            }
            
            // Release node for deletion
            releaseNode(node);

            // Return data
            return data;
        }

        // Function to pop from the left side of the queue
        // Based on Figure 10 PopRight function from the paper
        std::optional<T> popRight() {
            // Declare next
            HazardGuard<Node<T>> Hnext = DeRefLink(tail);
            Node<T>* next = Hnext.ptr;

            // Declare node
            HazardGuard<Node<T>> Hnode = DeRefLink(next->prev.load().getPtr());
            Node<T>* node = Hnode.ptr;

            // Create variable to hold return data
            T data;

            // Loop infinitely
            while (true) {
                // If node->next does not equal unmarked next correct the connection
                if (node->next.load() != MarkedPtr<T>(next, false)) {
                    // correctPrev returns a node that should already be protected
                    node = correctPrev(node, next);
                    // Update protection to the new node
                    Hnode = DeRefLink(node);

                    // Continue into next loop iteration
                    continue;
                }

                // If queue is empty return nullopt
                if (node == head) {
                    return std::nullopt;
                }

                // if node->next can be swapped from unmarked next to marked next
                // Note: 'next' should always be the tail according to the paper
                MarkedPtr<T> expected(next, false);
                if (CASRef(&node->next, expected, MarkedPtr<T>(next, true))) {
                    // Declare prev
                    HazardGuard<Node<T>> Hprev = DeRefLink(node->prev.load().getPtr());
                    Node<T>* prev = Hprev.ptr;

                    // Connect prev->next directly to next (bypassing deleted node)
                    // If CAS succeeds remove protection from node
                    MarkedPtr<T> expected(node, false);
                    if (CASRef(&prev->next, expected, MarkedPtr<T>(next, false))) {
                        releaseNode(node);
                    }

                    // Connect prev and next
                    prev = correctPrev(prev, next);

                    // Store data
                    data = node->data;

                    // Break out of loop
                    break;
                }

                // Back-Off
                std::this_thread::yield();
            }

            // Release node for deletion
            releaseNode(node);

            // Return data
            return data;
        }

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