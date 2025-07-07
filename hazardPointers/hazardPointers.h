#ifndef HAZARD_POINTERS_H
#define HAZARD_POINTERS_H

#include <atomic>
#include <thread>
#include <cstddef>
#include <cassert>
#include <vector>

// Tomorrow:
// Implement array as ptrs in HazardPointer struct
// Make sure heavy concurrency test passes
// Create heavy concurrency test for all other lockless objects

// Maximum number of threads that can be supported for hazard pointers
// Would need to be higher for a GPU implementation but is fine for now
constexpr size_t MAX_HAZARD_POINTERS = 128;

// Struct representing a hazard pointer slot for a thread
struct HazardPointer {
    // ptrs is a vector of atomic wrapper of pointers the thread is currently protecting
    // It is guarenteed that a thread will only ever write to itself
    std::atomic<void*> ptrs;
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
    std::atomic<void*> aPtr(ptr);
    globalHazardPointers[hazardSlot].ptrs.push_back(std::move(aPtr));
}

// Remove a given hazard pointer
// Nothing will happen if the pointer is not protected
// Time is O(n) but there shouldn't be many protections at a time
void removeHazardPointer(void* ptr) {
    // surivors will hold still hazardous nodes
    std::vector<std::atomic<void*>> survivors;

    for (auto &p : globalHazardPointers[hazardSlot].ptrs) {
        // Only reclaim the node if no thread is currently protecting it with a hazard pointer
        if (p.load() == ptr) {
            // Still in use, keep it for later checking
            survivors.push_back(ptr);
        }
    }

    // Set retireList to survivors
    globalHazardPointers[hazardSlot].ptrs = std::move(survivors);
}

// Check if any thread is currently protecting the given pointer
// Used to determine if a pointer is "safe" to reclaim/free
// Time is O(n^2) but there shouldn't be many protections at a time
bool isHazard(void* ptr) {
    for (size_t i = 0; i < MAX_HAZARD_POINTERS; i++) {
        for (auto &p: globalHazardPointers[i].ptrs) {
            // If it is being protected return true
            if (p.load() == ptr) {
                return true;
            }
        }
    }

    // Nothing found, return false
    return false;
}

#endif