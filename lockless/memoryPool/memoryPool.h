#ifndef LOCKLESSMEMORYPOOL_H
#define LOCKLESSMEMORYPOOL_H

#include <atomic>
#include <cstddef>
#include <vector>

// Memory block struct
// Constructor will not be needed due to the way it is being used
struct Block {
    std::atomic<Block*> next;
};

// Used to avoid the ABA problem
struct TaggedPtr {
    Block* ptr;
    uint64_t tag;
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