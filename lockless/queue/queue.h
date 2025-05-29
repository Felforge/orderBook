#ifndef QUEUE_H
#define QUEUE_H

#include <atomic>
#include <cstdint>
#include <cstddef>
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
            // ~MARK_MASK inverts tje bits of MARK_MASK
            // The return statement clears the first bit and casts it to Node<T>*
            return (Node<T>*)(bits & ~MARK_MASK);
        }
        
        // Marked as const as to not modify any member variables of the parent class
        bool getMark() const {
            // Isolates the lowest bit
            // If the result is nonzero it means the mark is set and returns true
            return bits & MARK_MASK;
        }

        // Comparison operators for MakredPtr objects
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
        data(nullptr), refCount(1), isDummy(true), memoryBlock(memoryBlock) {}
    
    // Used if parameters are provided
    Node(void* memoryBlock, const T& val) : prev(MarkedPtr<T>(nullptr, false)), next(MarkedPtr<T>(nullptr, false)), 
        data(val), refCount(1), value(val), isDummy(false), memoryBlock(memoryBlock) {}
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

        // Returns a node pointer and incraments the reference count
        Node<T>* copy(Node<T>* node) {
            if (node) {
            // Incrament counter while avoiding contention
                node->refCount.fetch_add(1, std::memory_order_relaxed);
            }
            return node;
        }

        // Predeclare for usage in terminateNode
        void releaseNode(Node<T>* node);

        template <typename T>
        void terminateNode(Node<T>* node) {
            if (!node) {
                return;
            }
            releaseNode(node->prev.load().getPtr());
            releaseNode(node->next.load().getPtr());

            // Handle memory block
            pool.deallocate(node->memoryBlock);
        }

        // Self explanatory
        void releaseNode(Node<T>* node) {
            if (!node) {
                return;
            }
            // Only reference is being deleted so node can also be terminated
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