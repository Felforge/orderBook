#ifndef HAZARD_POINTERS_H
#define HAZARD_POINTERS_H

#include <atomic>
#include <thread>
#include <cstddef>
#include <cassert>

// Maximum number of threads that can be supported for hazard pointers
// Would need to be higher for a GPU implementation but is fine for now
constexpr size_t MAX_HAZARD_POINTERS = 128;

// Struct representing a hazard pointer slot for a thread
struct HazardPointer {
    // ptr is an atomic wrapper of the pointer the thread is currently protecting
    std::atomic<void*> ptr;
};

// Global array of hazard pointers, one slot per thread
// Poentially wasted space but the number isn't very high
HazardPointer globalHazardPointers[MAX_HAZARD_POINTERS];

// Each thread gets a unique slot in the global hazard pointer table
// This lambda is run the first time a thread accesses hazardSlot
thread_local size_t hazardSlot = []{
    // next is only initialized on first execution due to static
    static std::atomic<size_t> next{0};

    // set slot as current next and then increment next
    size_t slot = next.fetch_add(1);

    // Make sure the supported thread count is not exceeded
    assert(slot < MAX_HAZARD_POINTERS && "Too many threads for hazard pointer table");

    // Return slot number
    return slot;
}();

// Set the current thread's hazard pointer to a given pointer
void setHazardPointer(void* ptr) {
    globalHazardPointers[hazardSlot].ptr.store(ptr, std::memory_order_release);
}

// Clear the current thread's hazard pointer
void clearHazardPointer() {
    globalHazardPointers[hazardSlot].ptr.store(nullptr, std::memory_order_release);
}

// Check if any thread is currently protecting the given pointer
// Used to determine if a pointer is "safe" to reclaim/free
bool isHazard(void* ptr) {
    for (size_t i = 0; i < MAX_HAZARD_POINTERS; i++) {
        // If it is being protected return true
        // Won't be enough threads for the O(n) time to matter
        // Can probably be switched later if needed
        if (globalHazardPointers[i].ptr.load(std::memory_order_acquire) == ptr) {
            return true;
        }
    }

    // Nothing found, return false
    return false;
}

#endif