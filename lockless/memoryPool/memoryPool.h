#ifndef LOCKLESSMEMORYPOOL_H
#define LOCKLESSMEMORYPOOL_H

#include <atomic>
#include <cstddef>
#include <vector>

// Memory block struct
// Constructor will not be needed due to the way it is being used
struct Block {
    Block* next;
};

class MemoryPool {
    public:
        MemoryPool(size_t inpBlockSize, size_t inpBlockCount);
        ~MemoryPool();

        void* allocate();
        void deallocate(void* ptr); 
    
    private:
        size_t blockSize;
        size_t blockCount;
        std::vector<char> pool;
        std::atomic<Block*> freeList; // atomic for lock-free
};

#endif