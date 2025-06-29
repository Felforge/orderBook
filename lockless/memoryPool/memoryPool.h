#ifndef LOCKLESSMEMORYPOOL_H
#define LOCKLESSMEMORYPOOL_H

#include <thread>
#include <bit>
#include "freeList.h"
#include "../MPSCQueue/MPSCQueue.h"

// Get next power of 2 for capacity of remoteFree
// constexpr instructs the program to evaluate the function at compile time
constexpr size_t nextPow2(size_t x) {
    return x <= 1 ? 1 : 1ull << (std::bit_width(x - 1));
}

// SPSC Memory Pool with remote free support via MPSC queue
// NumBlocks is the number of objects in the free list
template<size_t BlockSize, size_t NumBlocks>
class MemoryPool {
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

    public:
        // Constructor
        // Preallocates NumBlocks objects and adds them to the free list
        MemoryPool() {
            // Make sure construction parameters are valid
            static_assert(NumBlocks > 0, "Number of blocks must be an integer greater than zero!");

            for (size_t i = 0; i < NumBlocks; ++i) {
                // Allocate memory block
                Block* block= new Block();

                // Add memory block to freeList
                freeList.push(block);
            }
        }

        // Destructor
        // Deletes all objects remaining in the local free list and remote queue
        ~MemoryPool() {
            // Drain and delete everything from the local free list
            Block* block;
            while (!freeList.isEmpty()) {
                block = freeList.pop();
                delete block;
            }

            // Drain and delete anything left in the remote free queue
            // remoteFree.pop returns a bool indicatng if it succeeded
            // However, it will only fail if empty which is not possible here
            while (!remoteFree.isEmpty()) {
                remoteFree.pop(block);
                delete block;
            }        
        }

        // Allocate an object from the pool
        void* allocate() {
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
        void deallocate(void* ptr) {
            // Convert void pointer back into Block
            Block* block = static_cast<Block*>(ptr);

            if (isOwnerThread()) {
                // Is owner thread, push to free List
                freeList.push(block);

            } else {
                // If the queue is full yield until space is availbe
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

            // Drain remoteFree and add it to freeList
            // remoteFree.pop returns a bool indicatng if it succeeded
            // However, it will only fail if empty which is not possible here
            while (!remoteFree.isEmpty()) {
                remoteFree.pop(block);
                freeList.push(block);
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