#ifndef HAZARD_POINTERS_H
#define HAZARD_POINTERS_H

#include <atomic>
#include <thread>
#include <cstddef>
#include <cassert>
#include <unordered_set>

// Maximum number of threads that can be supported for hazard pointers
// Would need to be higher for a GPU implementation but is fine for now
constexpr size_t MAX_HAZARD_POINTERS = 128;

// Struct representing a hazard pointer slot for a thread
struct HazardPointer {
    // ptrs is a hasmap of atomic wrapper of pointers the thread is currently protecting
    // It is guarenteed that a thread will only ever write to itself
    std::unordered_set<void*> ptrs;
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
    globalHazardPointers[hazardSlot].ptrs.insert(ptr);
}

// Remove a given hazard pointer
void removeHazardPointer(void* ptr) {
    // Nothing will happen if the key does not exist
    globalHazardPointers[hazardSlot].ptrs.erase(ptr);
}

// Check if any thread is currently protecting the given pointer
// Used to determine if a pointer is "safe" to reclaim/free
bool isHazard(void* ptr) {
    for (size_t i = 0; i < MAX_HAZARD_POINTERS; i++) {
        // If it is being protected return true
        if (globalHazardPointers[i].ptrs.contains(ptr)) {
            return true;
        }
    }

    // Nothing found, return false
    return false;
}

#endif