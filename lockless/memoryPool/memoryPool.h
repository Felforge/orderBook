#ifndef LOCKLESSMEMORYPOOL_H
#define LOCKLESSMEMORYPOOL_H

#include <atomic>
#include <cstddef>
#include <vector>

// Memory block struct
struct Block {
    size_t ownerThreadID;
    Block* next;
};

class MemoryPool {
    public:
        MemoryPool(size_t inpBlockSize, size_t inpBlockCount);
        ~MemoryPool();

        void* allocate(bool test=false);
        void deallocate(void* ptr); 
    
    private:
        size_t blockSize;
        size_t blockCount;
        std::vector<char> pool;
        std::atomic<TaggedPtr> freeList;
};

#endif