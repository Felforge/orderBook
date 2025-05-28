#include <iostream>
#include "memoryPool.h"

MemoryPool::MemoryPool(size_t inpBlockSize, size_t inpBlockCount) 
    : blockSize(inpBlockSize), blockCount(inpBlockCount), freeList(nullptr) {

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
        std::cerr << "ERROR: Memory pool exhausted!" << std::endl;
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

// int main() {
//     try {
//         std::cout << "DEBUG: Creating MemoryPool object" << std::endl;
//         MemoryPool pool(64, 1000); // Test with 1000 blocks of 64 bytes
//         std::cout << "DEBUG: MemoryPool created successfully" << std::endl;

//         // Allocate and deallocate memory
//         void* ptr1 = pool.allocate();
//         std::cout << "DEBUG: Allocated block at " << ptr1 << std::endl;

//         void* ptr2 = pool.allocate();
//         std::cout << "DEBUG: Allocated block at " << ptr2 << std::endl;

//         pool.deallocate(ptr1);
//         std::cout << "DEBUG: Deallocated block at " << ptr1 << std::endl;

//         pool.deallocate(ptr2);
//         std::cout << "DEBUG: Deallocated block at " << ptr2 << std::endl;
//     } catch (const std::exception& e) {
//         std::cerr << "ERROR: Exception: " << e.what() << std::endl;
//     }
//     return 0;
// }