#include "memoryPool.h"

MemoryPool::MemoryPool(size_t inpBlockSize, size_t inpBlockCount) {
    // Assign variables
    blockSize = inpBlockSize;
    blockCount = inpBlockCount;
    freeList = nullptr;

    // Preallocate memory for the pool
    pool.resize(blockSize * blockCount);

    // Initialize free list
    for (size_t i = 0; i < blockCount; ++i) {
        Block* block = reinterpret_cast<Block*>(&pool[i * blockSize]);
        block->next = freeList;
        freeList = block;
    }
}

void* MemoryPool::allocate() {
    if (!freeList) throw std::bad_alloc();

    Block* block = freeList;
    freeList = freeList->next;
    return block;
}

void MemoryPool::deallocate(void* ptr) {
    Block* block = reinterpret_cast<Block*>(ptr);
    block->next = freeList;
    freeList = block;
}