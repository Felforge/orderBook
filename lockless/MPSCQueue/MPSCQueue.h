#ifndef MPSCQUEUE_H
#define MPSCQUEUE_H

#include <atomic>
#include <cstddef>
#include <vector>

// Multiple-producer single-consumer (MPSC) lock-free queue implemented as a ring buffer
// Used for remote free queues between threads
// T is the type of element stored in the queue
// Capacity The capacity of the queue (must be a power of two)
template<typename T, size_t Capacity>
class MPSCQueue {
    private:
        // Index for the next enqueue
        std::atomic<size_t> head;

        // Index for the next enqueue
        std::atomic<size_t> tail;

    public:
        // Ring buffer storage
        // Public for testing purposes
        std::atomic<T*> buffer[Capacity];

        // Constructor
        SPSCQueue() {
            // Initalize head and tail
            head.store(0);
            tail.store(0);

            // Make sure Capacity is a power of 2
            // This makes wrapping an index far more efficient
            // Wrapping an index is settinh the index back to zero at the end of a loop
            static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

            // Initialize empty values for the buffer
            for (size_t i = 0; i < Capacity; i++) {
                buffer[i].store(nullptr, std::memory_order_relaxed);
            }
        }

        // Push a pointer to a value item into the queue
        // Thread safe for multiple producers
        // Returns true if the push was successful
        // Returns false if the queue is full
        // NOTE: This implementation stores pointers, so you must manage memory outside the queue
        bool push(T* item) {
            // Retrieve new position
            // memory_order_acq_rel is used for operations that both read and write
            size_t pos = head.fetch_add(1, std::memory_order_acq_rel);

            // Retrieve index in ring buffer
            size_t idx = pos & (Capacity - 1);

            // Attempt to exchange nullptr for the item at idx
            // Strong will have worse performance as opposed to weak but it is more reliable
            // It is needed to check if the queue is truly full reliably
            // memory_order_release applies if the exchange is successful
            // This ensures other threads will see the newly initialized object
            if (!buffer[idx].compare_exchange_strong(expected, std::memory_order_release, std::memory_order_relaxed)) {
                // The slot was not empty, this means the queue is full
                return false;
            }

            // Success, return true
            return true;
        }

        // Pops an item from the queue and assigns to result
        // result is a reference to a pointer
        // NOT safe for multiple threads
        // return true if the pop was successful
        // returns false if the queue is empty
        bool pop(T*& result) {
            // Retrieve tail
            size_t curTail = tail.load(std::memory_order_relaxed);

            // Retrieve index in ring buffer
            size_t idx = curTail & (Capacity - 1);

            // Retrieve pointer to item
            // memory_order_acquire ensures that no read and writes that appear 
            // after this call in the program can not be moved before it
            T* item = buffer[idx].load(std::memory_order_acquire);
            if (item == nullptr) {
                // If item is nullptr this means the queue is empty
                return false;
            }

            // Take ownership of the item
            result = item;

            // Clear the slot
            buffer[idx].store(nullptr, std::memory_order_release);
            tail.store(tail + 1, std::memory_order_relaxed);

            // Success, return true
            return true;
        }

        // Returns true if the queue is empty
        bool isEmpty() {
            // Retrieve tail
            size_t curTail = tail.load(std::memory_order_relaxed);

            // Retrieve index of tail in ring buffer
            size_t idx = curTail & (Capacity - 1);

            // Tail must be nullptr if the queue is empty
            return buffer[idx].load(std::memory_order_acquire) == nullptr;
        }

        // Returns true if the queue is full
        bool isFull() {
            // Retrieve head
            size_t curHead = head.load(std::memory_order_relaxed);

            // Retrieve index of head in ring buffer
            size_t idx = curHead & (Capacity - 1);

            // Head must not be nullptr if the queue is full
            return buffer[idx].load(std::memory_order_acquire) != nullptr;
        }
};

#endif