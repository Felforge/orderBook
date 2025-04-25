#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <cstddef>
#include <vector>

struct alignas(64) Block {
    Block* next;
};

class MemoryPool {
private:
    Block* freeList; // Pointer to the first free block
    size_t blockSize;
    size_t blockCount;
    std::vector<char> pool; // Preallocated memory
public:
    MemoryPool() = default; // If constructor is not called
    MemoryPool(size_t inpBlockSize, size_t inpBlockCount);
    ~MemoryPool();
    void* allocate();
    void deallocate(void* ptr);
};

#endif