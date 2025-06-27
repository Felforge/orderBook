#include <iostream>
#include "memoryPool.h"
using namespace std;

MemoryPool::MemoryPool(size_t inpBlockSize, size_t inpBlockCount) 
    : blockSize(inpBlockSize), blockCount(inpBlockCount) {
    
    // Set memory pool size
    pool.resize(blockSize * blockCount);

    // Initialize freeList with atomic
    Block* prev = nullptr;
    for (size_t i = 0; i < blockCount; i++) {
        Block* block = reinterpret_cast<Block*>(&pool[i * blockSize]);
        block->next = prev;
        prev = block;
    }

    // Set pointer to first available memory block
    // The pointer is a tagger pointer so a tag value is needed
    // memory_order_release ensures that freeList is only updated 
    // after all other initializations are completed
    freeList.store({prev, 0}, memory_order_release);
}

// MemoryPool destiructor
MemoryPool::~MemoryPool() {
    // Explicitly clear the vector to release memory
    pool.clear();

    // Reduce vector capacity to zero
    pool.shrink_to_fit();

    // Reset the freeList pointer for safety
    freeList.store({nullptr, 0}, memory_order_release);
}

// Function to allocate memory block
void* MemoryPool::allocate(bool test) {
    TaggedPtr old, next;

    // Do-while ensures that the code is executed at least once
    do {
        // Retrieve next available memory block
        old = freeList.load(memory_order_acquire);

        // Throw error if memory pool is full
        if (!old.ptr) {
            if (!test) {
                cerr << "ALLOCATION ERROR: Memory pool exhausted!" << std::endl;
            }
            throw bad_alloc();
        }

        // Retrieve following memory block
        next.ptr = old.ptr->next.load(std::memory_order_relaxed);

        // Try to swap old with next
    } while (!freeList.compare_exchange_weak(old, next, memory_order_acquire, memory_order_relaxed));
    // memorder_order_acquire swaps the value if CAS succeeds 
    // memory_order_relaxed does nothing if CAS fails

    // Return memory block
    return old.ptr;
}

// Function to deallocate memory block
void MemoryPool::deallocate(void* blockPtr) {
    // Retrieve block and convert back to Block object pointer
    Block* block = reinterpret_cast<Block*>(blockPtr);
    TaggedPtr old, next;

    // Do-while ensures that the code is executed at least once
    // This loop sets the freed block as the new head
    do {
        // Retrieve current head
        old = freeList.load(memory_order_acquire);

        // Place deallocated block before head
        block->next.store(old.ptr, std::memory_order_relaxed);

        // Moves new block to the front of the queue
        next.ptr = block;

        // Increments tag by 1
        // Used to avoid the ABA problem
        next.tag = old.tag + 1;

        // Try to swap old with next
    } while (!freeList.compare_exchange_weak(old, next, memory_order_release, memory_order_relaxed));
    // memorder_order_release blocks all other memory operations if CAS succeeds 
    // memory_order_relaxed does nothing if CAS fails
}
