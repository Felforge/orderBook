#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <cstddef>
#include <vector>

struct Block {
    Block* next;
};

class MemoryPool {
private:
    Block* freeList; // Pointer to the first free block
    size_t blockSize;
    size_t blockCount;
    std::vector<char> pool; // Preallocated memory
public:
    MemoryPool(size_t inpBlockSize, size_t inpBlockCount);
    ~MemoryPool();
    void* allocate(bool test=false);
    void deallocate(void* ptr);
};

#endif