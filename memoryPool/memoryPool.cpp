#include "memoryPool.h"

MemoryPool::MemoryPool(size_t inpBlockSize, size_t inpBlockCount) {
    // Assign variables
    blockSize = inpBlockSize;
    blockCount = inpBlockCount;
    freeList = nullptr;

    // Preallocate memory for the pool
    pool.resize(blockSize * blockCount);

    // Initialize free list
    for (size_t i = 0; i < blockCount; i++) {
        Block* block = reinterpret_cast<Block*>(&pool[i * blockSize]);
        block->next = freeList;
        freeList = block;
    }
}

// Avoid scary memory leak
MemoryPool::~MemoryPool() {
    // Explicitly clear the vector to release memory
    pool.clear();

    // Reduce capacity to 0
    pool.shrink_to_fit();

    // Reset the free list pointer for safety
    freeList = nullptr;
}

void* MemoryPool::allocate() {
    if (!freeList) {
        throw std::bad_alloc();
    }
    Block* block = freeList;
    freeList = freeList->next;
    return block;
}

void MemoryPool::deallocate(void* ptr) {
    Block* block = reinterpret_cast<Block*>(ptr);
    block->next = freeList;
    freeList = block;
}