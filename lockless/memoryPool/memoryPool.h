#ifndef LOCKLESSMEMORYPOOL_H
#define LOCKLESSMEMORYPOOL_H

#include <thread>
#include "freeList.h"
#include "../MPSCQueue/MPSCQueue.h"

// SPSC Memory Pool with remote free support via MPSC queue
// T is the type of the object managed by the pool
// PoolCapacity is the number of objects in the free list
// RemoteFreeCapacity is the number of objects that can be waiting in the MPSC queue
template<typename T, size_t PoolCapacity, size_t RemoteFreeCapacity>
class MemoryPool {
    private:
        // SPSC free list for the owner thread
        FreeList<T> freeList;

        // MPSC queue for remote frees
        MPSCQueue<T, RemoteFreeCapacity> remoteFree;

        // Thread id of the owner (set at construction)
        std::thread::id owner = std::this_thread::get_id();

    public:
        // Constructor
        // Preallocates PoolCapacity objects and adds them to the free list
        MemoryPool() {
            for (size_t i = 0; i < PoolCapacity; ++i) {
                // Allocate memory block
                T* obj = new T();

                // Add memory block to freeList
                freeList.push(obj);
            }
        }

        // Destructor
        // Deletes all objects remaining in the local free list and remote queue
        ~MemoryPool() {
            // Drain and delete everything from the local free list
            T* obj;
            while (!freeList.isEmpty()) {
                obj = freeList.pop();
                delete obj;
            }

            // Drain and delete anything left in the remote free queue
            while (!remoteFree.isEmpty()) {
                obj = remoteFree.pop();
                delete obj;
            }        
        }

        // Allocate an object from the pool
        T* allocate() {
            // Reclaim objects returned by remote threads
            drainRemoteFree();

            // Get next block from freeList
            T* obj = freeList.pop();

            // Throw bad_alloc if object is nullptr as pool is full
            if (!obj) {
                throw std::bad_alloc();
            }

            // Return object
            return obj;
        }

        // Deallocate (free) an object
        void deallocate(T* obj) {
            if (isOwnerThread()) {
                // Is owner thread, push to free List
                freeList.push(obj);

            } else {
                // If the queue is full yield until space is availbe
                while (remoteFree.isFull()) {
                    std::this_thread::yield();
                }

                // Push to remoteFree
                remoteFree.push(obj);
            }
        }

        // Drain all objects from the remote free queue back into the local free list
        // Should be called periodically by the owner thread
        void drainRemoteFree() {
            T* obj;
            while (!remoteFree.isEmpty()) {
                obj = remoteFree.pop();
                freeList.push(obj);
            }
        }

        // Returns true if this thread is the owner of the memory pool
        bool isOwnerThread() {
            return std::this_thread::get_id() == owner;
        }
};

#endif