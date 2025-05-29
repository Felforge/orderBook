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
    // memory_order_release ensures that freeList is only updated 
    // after all other initializations are completed
    freeList.store(prev, memory_order_release);
}

// MemoryPool destiructor
MemoryPool::~MemoryPool() {
    // Explicitly clear the vector to release memory
    pool.clear();

    // Reduce vector capacity to zero
    pool.shrink_to_fit();

    // Reset the freeList pointer for safety
    freeList.store(nullptr, memory_order_release);
}

// Function to allocate memory block
void* MemoryPool::allocate(bool test) {
    Block* head = nullptr;
    Block* next = nullptr;

    // Do-while ensures that the code is executed at least once
    do {
        // Retrieve next available block
        head = freeList.load(memory_order_acquire);

        // Throw error if memory pool is full
        if (!head) {
            if (!test) {
                cerr << "ALLOCATION ERROR: Memory pool exhausted!" << std::endl;
            }
            throw bad_alloc();
        }

        // Retrieve next value
        next = head->next;

        // Try to swap head with next
    } while (!freeList.compare_exchange_weak(head, next, memory_order_acquire, memory_order_relaxed));
    // memorder_order_acquire swaps the value if CAS succeeds 
    // memory_order_relaxed does nothing if CAS fails

    // Return memory block
    return head;
}

// Function to deallocate memory block
void MemoryPool::deallocate(void* blockPtr) {
    // Retrieve block and convert back to Block object pointer
    Block* block = reinterpret_cast<Block*>(blockPtr);
    Block* head = nullptr;

    // Do-while ensures that the code is executed at least once
    // This loop sets the freed block as the new head
    do {
        // Retrieve current head
        head = freeList.load(memory_order_acquire);

        // Place deallocated block before head
        block->next = head;

        // Try to swap head with new block
    } while (!freeList.compare_exchange_weak(head, block, memory_order_release, memory_order_relaxed));
    // memorder_order_release blocks all other memory operations if CAS succeeds 
    // memory_order_relaxed does nothing if CAS fails
}
