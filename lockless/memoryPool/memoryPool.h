#ifndef LOCKLESS_MEMORY_POOL_H
#define LOCKLESS_MEMORY_POOL_H

#include <thread>
#include <bit>
#include <cstring>
#include "freeList.h"
#include "../MPSCQueue/MPSCQueue.h"

// Generic Memory Pool Type
// Needed to deallocate in queue.h
// Other methods are not needed for this reason
// Destructor also needed for potential cleanup
struct GenericMemoryPool {
    virtual void* allocate() = 0;
    virtual void deallocate(void* ptr) = 0;
    virtual ~GenericMemoryPool() = default;
};

// Get next power of 2 for capacity of remoteFree
// constexpr instructs the program to evaluate the function at compile time
constexpr size_t nextPow2(size_t x) {
    return x <= 1 ? 1 : 1ull << (std::bit_width(x - 1));
}

// SPSC Memory Pool with remote free support via MPSC queue
// NumBlocks is the number of objects in the free list
template<size_t BlockSize, size_t NumBlocks>
class MemoryPool : public GenericMemoryPool {
    private:
        // Private memory block structure
        // Next will not be used but it is needed
        // Otherwise it will be too small for FreeList
        // FreeList requires a data type of 8 bytes or more
        struct Block {
            char data[BlockSize];
            Block* next;
        };

        // SPSC free list for the owner thread
        FreeList<Block> freeList;

        // MPSC queue for remote frees
        // Capacity must be at least the same as the number of blocks for safety
        // Capacity must also be a power of 2 for performance reasons
        MPSCQueue<Block, nextPow2(NumBlocks)> remoteFree;

        // Thread id of the owner (set at construction)
        std::thread::id owner = std::this_thread::get_id();

        // Array to track all allocated blocks for proper cleanup
        Block* allBlocks[NumBlocks];

    public:
        // Constructor
        // Preallocates NumBlocks objects and adds them to the free list
        MemoryPool() {
            std::cout << "MemoryPool constructor: NumBlocks=" << NumBlocks << " BlockSize=" << sizeof(Block) << std::endl;
            
            // Make sure construction parameters are valid
            static_assert(NumBlocks > 0, "Number of blocks must be an integer greater than zero!");

            for (size_t i = 0; i < NumBlocks; ++i) {
                try {
                    // Allocate memory block
                    Block* block = new Block();
                    
                    // Store in allBlocks array for cleanup
                    allBlocks[i] = block;
                    
                    // Add memory block to freeList
                    freeList.push(block);
                } catch (const std::bad_alloc& e) {
                    // Exception safety: clean up any blocks already allocated
                    Block* cleanup;
                    while (!freeList.isEmpty()) {
                        cleanup = freeList.pop();
                        delete cleanup;
                    }
                    throw; // Re-throw the exception
                }
            }
        }

        // Destructor
        // Deletes all blocks that were allocated by this pool
        ~MemoryPool() {
            std::cout << "MemoryPool destructor: NumBlocks=" << NumBlocks << " BlockSize=" << sizeof(Block) << std::endl;
            // Delete all blocks allocated by this pool
            for (size_t i = 0; i < NumBlocks; ++i) {
                delete allBlocks[i];
            }
            std::cout << "MemoryPool destructor completed: NumBlocks=" << NumBlocks << std::endl;
        }

        // Allocate an object from the pool
        void* allocate() override {
            // Reclaim objects returned by remote threads
            drainRemoteFree();

            // Get next block from freeList
            Block* block = freeList.pop();

            // Throw bad_alloc if block is nullptr as pool is full
            if (!block) {
                throw std::bad_alloc();
            }

            // Return object
            return block;
        }

        // Deallocate a memory block
        // Overrides the placeholder in GenericMmemoryPool
        void deallocate(void* ptr) override {
            std::thread::id threadId = std::this_thread::get_id();
            std::cout << "[DEALLOC THREAD " << threadId << "] deallocating block " << ptr << " isOwner=" << isOwnerThread() << std::endl;
            
            // Convert void pointer back into Block
            Block* block = static_cast<Block*>(ptr);

            // Poison the block memory
            // Poison means to fill the block with garbage  
            // std::memset(block, 0xDEADBEEF, sizeof(Block));

            if (isOwnerThread()) {
                // Is owner thread, push to free List
                std::cout << "[DEALLOC THREAD " << threadId << "] returning to local freeList" << std::endl;
                freeList.push(block);

            } else {
                // If the queue is full yield until space is availbe
                std::cout << "[DEALLOC THREAD " << threadId << "] pushing to remoteFree queue" << std::endl;
                while (remoteFree.isFull()) {
                    std::this_thread::yield();
                }

                // Push to remoteFree
                remoteFree.push(block);
            }
        }

        // Drain all blocks from the remote free queue back into the local free list
        // Should be called periodically by the owner thread
        void drainRemoteFree() {
            Block* block;
            int drainedCount = 0;

            // Drain remoteFree and add it to freeList
            // remoteFree.pop returns a bool indicatng if it succeeded
            // However, it will only fail if empty which is not possible here
            while (!remoteFree.isEmpty()) {
                remoteFree.pop(block);
                freeList.push(block);
                drainedCount++;
            }
            
            if (drainedCount > 0) {
                std::thread::id threadId = std::this_thread::get_id();
                std::cout << "[DRAIN THREAD " << threadId << "] drained " << drainedCount << " blocks from remoteFree" << std::endl;
            }
        }

        // Returns true if this thread is the owner of the memory pool
        bool isOwnerThread() {
            return std::this_thread::get_id() == owner;
        }

        // Returns true if the memory pool is drained
        // For testing purposes
        bool isDrained() {
            return freeList.isEmpty();
        }

        // Returns true if the remoteFree is empty
        // For testing purposes
        bool isRemoteFreeEmpty() {
            return remoteFree.isEmpty();
        }

        // Returns true if the remoteFree is full
        // For testing purposes
        bool isRemoteFreeFull() {
            return remoteFree.isFull();
        }
};

#endif