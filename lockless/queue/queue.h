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

// Define SPIN_PAUSE based on architecture
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    #include <immintrin.h>
    #define SPIN_PAUSE() _mm_pause()
#else
    #define SPIN_PAUSE() std::atomic_signal_fence(std::memory_order_seq_cst)
#endif

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
        data(), isDummy(true), isRetired(false), memoryBlock(memoryBlock), ownerPool(ownerPool) {
    }
    
    // Used if parameters are provided
    Node(GenericMemoryPool* ownerPool, void* memoryBlock, const T& val) : prev(MarkedPtr<T>(nullptr, false)), next(MarkedPtr<T>(nullptr, false)), 
        data(val), isDummy(false), isRetired(false), memoryBlock(memoryBlock), ownerPool(ownerPool) {
    }
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
    // Return if node is a dummy node
    // isRetired is already set by releaseNode()
    if (node->isDummy) {
        return;
    }

    // Add the node to this thread's retire list
    retireList.push_back(static_cast<void*>(node));
}

template<typename T>
class LocklessQueue {
    private:
        // Dummy node memory pool
        // Size must be assigned in constructor
        MemoryPool<sizeof(Node<T>), 2> pool; 

        // Function to back off for unsuccessful operations
        // The number of spins could be messed with
        void spinBackoff(int spinCount = 1) {
            for (int i = 0; i < spinCount; ++i) {
                // Run spin pause
                SPIN_PAUSE();
            }
        }

        // Copy sets a hazard pointer and returns a node pointer
        // This should only be called on nodes that are already protected or are dummy nodes
        HazardGuard<Node<T>> DeRefLink(Node<T>* node) {
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

            // If node is no longer protected retire it
            if (!isHazard(node)) {
                // Only retire if not already retired (atomic check-and-set)
                if (!node->isRetired.exchange(true)) {
                    retireQueueNode<T>(node, *this);
                }
            } else {
                // Release protection of node
                removeHazardPointer(node);
            }
        }

        // Set mark on a node given the link
        void setMark(std::atomic<MarkedPtr<T>>* link) {
            // Start infinite loop
            while (true) {
                // Retrieve node with proper memory ordering
                MarkedPtr<T> current = link->load(std::memory_order_acquire);
                Node<T>* node = current.getPtr();

                // If already marked, we're done
                if (current.getMark()) {
                    break;
                }

                // Declare expected value
                MarkedPtr<T> expected(node, false);

                // Try to set mark with proper memory ordering
                if (link->compare_exchange_strong(expected, MarkedPtr<T>(node, true), 
                                                std::memory_order_release, std::memory_order_relaxed)) {
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
            // Protect node
            HazardGuard<Node<T>> Hnode = HazardGuard<Node<T>>(node);

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
                    // If node->prev is marked correct it
                    if (node->prev.load().getMark()) {
                        Hnode = correctPrev(std::move(Hnode), next);
                    }

                    // Break out of the loop
                    break;
                }

                // Back off
                spinBackoff();
            }
        }

        // CorrectPrev function from Figure 15 of the paper
        // Returns a HazardGuard protecting the corrected previous node
        HazardGuard<Node<T>> correctPrev(HazardGuard<Node<T>> Hprev, Node<T>* node) {
            // Return null if either parameter is null
            if (!Hprev.ptr || !node) {
                return HazardGuard<Node<T>>(nullptr);
            }

            // Retrieve prev
            Node<T>* prev = Hprev.ptr;

            // Set lastLink as nullptr
            Node<T>* lastLink = nullptr;
            
            // Protection for lastLink
            HazardGuard<Node<T>> Hlast(nullptr);

            // Start infinite loop
            while (true) {
                // Retrieve node-> prev
                MarkedPtr<T> link1 = node->prev.load();

                // Break if link1 is marked
                if (link1.getMark()) {
                    break;
                }

                // Check if prev is still valid before accessing it
                if (!prev) {
                    return HazardGuard<Node<T>>(nullptr);
                }

                // Retrieve prev->next
                MarkedPtr<T> Mprev2 = prev->next.load();
                HazardGuard<Node<T>> Hprev2 = DeRefLink(Mprev2.getPtr());
                Node<T>* prev2 = Hprev2.ptr;

                // If prev2 is marked
                if (Mprev2.getMark()) {

                    // If lastlink is valid
                    if (lastLink) {
                        // Set mark on node
                        setMark(&prev->prev);

                        // Swap references
                        CASRef(&lastLink->next, MarkedPtr<T>(prev, false), MarkedPtr<T>(prev2, false));

                        // Release prev
                        releaseNode(prev);

                        // Release prev2
                        releaseNode(prev2);

                        // Set prev to lastlink and transfer protection
                        prev = lastLink;
                        Hprev = std::move(Hlast);

                        // Clear lastlink
                        lastLink = nullptr;

                        // Continue into next loop iteration
                        continue;
                    }
                    
                    // Check if prev is still valid before accessing prev->prev
                    if (!prev) {
                        return HazardGuard<Node<T>>(nullptr);
                    }

                    // retrieve prev->prev
                    HazardGuard<Node<T>> Hprev2 = DeRefLink(prev->prev.load().getPtr());
                    prev2 = Hprev2.ptr;

                    // Release prev
                    releaseNode(prev);

                    // Set prev to prev2
                    prev = prev2;

                    // Transfer protection
                    Hprev = std::move(Hprev2);

                    // Continue into next loop iteration
                    continue;
                }

                // If prev2 does not eqaul node
                if (prev2 != node) {
                    // Transfer protection to lastLink before setting it
                    Hlast = std::move(Hprev);
                    
                    // Set lastLink to prev (now protected by Hlast)
                    lastLink = prev;

                    // Move protection to prev2
                    Hprev = std::move(Hprev2);
                    
                    // Move up prev2
                    prev = prev2;
                    
                    // Continue into next loop iteration per paper CP20
                    continue;
                }

                // Destroy prev2 protection
                Hprev2 = HazardGuard<Node<T>>(nullptr);

                // Try to swap node->prev from link1 to unmarked prev
                if (CASRef(&node->prev, link1, MarkedPtr<T>(prev, false))) {
                    // Check if prev is still valid before accessing it
                    if (!prev) {
                        continue;
                    }

                    // If prev->prev is marked then loop again
                    if (prev->prev.load().getMark()) {
                        continue;
                    }

                    // Break out of loop
                    break;
                }

                // Back off
                spinBackoff();
            }

            // Release lastLink protection if applicable
            // Note: lastLink protection is handled by Hlast which will be destroyed automatically

            // Return prev
            return std::move(Hprev);
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
                terminateNode(curr);
                curr = next;
            }
            terminateNode(head);
            terminateNode(tail);

            // Clear retire list
            retireList.clear();
        }

        // Returns a node created with the given data, protected by a hazard guard
        HazardGuard<Node<T>> createNode(T data, GenericMemoryPool* memoryPool) {
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

            // Return node with hazard protection
            return HazardGuard<Node<T>>(node);
        }

        // Calls Node destructor and deletes it from memory
        // bool destructor should be set to true in the destructor
        // Needs to be public to retire list nodes
        void terminateNode(Node<T>* node) {
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
            // create new node with hazard protection
            HazardGuard<Node<T>> Hnode = createNode(data, memoryPool);
            Node<T>* node = Hnode.ptr;

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
                spinBackoff();
            }

            // Finish push
            pushEnd(node, next);

            return node;
        }

        // Function to push to the right side of the queue  
        // Based on Figure 9 PushRight procedure from the paper
        Node<T>* pushRight(T data, GenericMemoryPool* memoryPool) {
            // create new node with hazard protection
            HazardGuard<Node<T>> Hnode = createNode(data, memoryPool);
            Node<T>* node = Hnode.ptr;

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
                Hprev = correctPrev(std::move(Hprev), next);
                prev = Hprev.ptr;  // Update prev pointer

                // Back-Off
                spinBackoff();
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
                    CASRef(&prev->next, Mnode, MarkedPtr<T>(next, false));

                    // Go into next loop iteration
                    continue;
                }

                // if node->next can be swapped from unmarked next to marked next
                if (CASRef(&node->next, Mnext, MarkedPtr<T>(next, true))) {
                    // Connect prev->next directly to next (bypassing deleted node)
                    CASRef(&prev->next, Mnode, MarkedPtr<T>(next, false));

                    // Connect prev and next
                    Hprev = correctPrev(std::move(Hprev), next);
                    prev = Hprev.ptr;  // Update prev pointer

                    // Store data
                    data = node->data;

                    // Break out of loop
                    break;
                }

                // Back-Off
                spinBackoff();
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
                    Hnode = correctPrev(std::move(Hnode), next);
                    node = Hnode.ptr;  // Update node pointer
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
                    MarkedPtr<T> expected(node, false);
                    CASRef(&prev->next, expected, MarkedPtr<T>(next, false));

                    // Connect prev and next
                    Hprev = correctPrev(std::move(Hprev), next);

                    // Store data
                    data = node->data;

                    // Release node from protection
                    releaseNode(node);

                    // Break out of loop
                    break;
                }

                // Back-Off
                spinBackoff();
            }

            // Release node for deletion
            releaseNode(node);

            // Return data
            return data;
        }

        // Function to remove a given node
        // Returns the data from the node or nothing
        std::optional<T> removeNode(Node<T>* node) {
            // Return nullopt if node is invalid or a dummy node
            if (!node || node->isDummy) {
                return std::nullopt;
            }

            // Protect node
            HazardGuard<Node<T>> Hnode = DeRefLink(node);

            // To be returned
            T data = node->data;
            
            // Start infinite loop
            while (true) {
                // Retrieve node->next
                MarkedPtr<T> Mnext = node->next.load();
                HazardGuard<Node<T>> Hnext = DeRefLink(Mnext.getPtr());
                Node<T>* next = Hnext.ptr;

                // If next is marked return nullopt
                if (Mnext.getMark()) {
                    return std::nullopt;
                }

                // Attempt to mark node->next
                if (CASRef(&node->next, Mnext, MarkedPtr<T>(next, true))) {
                    // Hold prev for outside use
                    HazardGuard<Node<T>> Hprev(nullptr);
                    Node<T>* prev;

                    // Start infinite loop
                    while (true) {
                        // Retrieve node->prev
                        MarkedPtr<T> Mprev = node->prev.load();
                        Hprev = DeRefLink(Mprev.getPtr());
                        prev = Hprev.ptr;

                        // If prev is already marked or is successfully marked break
                        if(Mprev.getMark() || CASRef(&node->prev, Mprev, MarkedPtr<T>(prev, true))) {
                            break;
                        }

                        // Back off if failed
                        spinBackoff();
                    }

                    // Connect prev and next
                    Hprev = correctPrev(std::move(Hprev), next);

                    // Release the protection from node
                    releaseNode(node);

                    // Release node for retirement
                    releaseNode(node);

                    // Return data
                    return data;
                }
                
                // Back off if failed
                spinBackoff();
            }
        }

        // Function to get left node data if not empty
        // Returns the data from the node or nothing
        // Not neccessarily thread safe on the same side
        std::optional<T> getLeft() {
            // Get node ptr
            Node<T>* ptr = head->next.load().getPtr();

            // If queue is empty return nullopt
            if (ptr == tail) {
                return std::nullopt;
            }

            // Else return the value
            return ptr->data;
        }

        // Function to get right node data if not empty
        // Returns the data from the node or nothing
        // Not neccessarily thread safe on the same side
        std::optional<T> getRight() {
            // Get node ptr
            Node<T>* ptr = tail->prev.load().getPtr();

            // If queue is empty return nullopt
            if (ptr == head) {
                return std::nullopt;
            }

            // Else return the value
            return ptr->data;
        }

        // Check if the queue is empty
        // Not neccessarily thread safe on the same side
        bool isEmpty() {
            return head->next.load().getPtr() == tail;
        }
};

#endif