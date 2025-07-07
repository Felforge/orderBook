#ifndef HAZARD_POINTERS_H
#define HAZARD_POINTERS_H

#include <atomic>
#include <thread>
#include <cstddef>
#include <cassert>
#include <vector>

// Maximum number of threads that can be supported for hazard pointers
// Would need to be higher for a GPU implementation but is fine for now
constexpr size_t MAX_HAZARD_POINTERS = 128;

// Struct representing two hazard pointer slot for a thread
struct HazardPointer {
    // Two atomic hazard pointer slots
    std::atomic<void*> ptr1;
    std::atomic<void*> ptr2;

    // Set both slots to nullptr on construction
    HazardPointer() : ptr1(nullptr), ptr2(nullptr) {}
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
    // Check if ptr1 is available
    if (globalHazardPointers[hazardSlot].ptr1.load()) {
        // Not available, use ptr2
        globalHazardPointers[hazardSlot].ptr2.store(ptr);
    } else {
        // Is available, use it
        globalHazardPointers[hazardSlot].ptr1.store(ptr);
    }
}

// Remove a given hazard pointer
// Nothing will happen if the pointer is not protected
void removeHazardPointer(void* ptr) {
    if (globalHazardPointers[hazardSlot].ptr1.load() == ptr) {
        // Matches ptr1, clear it
        globalHazardPointers[hazardSlot].ptr1.store(nullptr);
    } else if (globalHazardPointers[hazardSlot].ptr2.load() == ptr) {
        // Matches ptr2, clear it
        globalHazardPointers[hazardSlot].ptr2.store(nullptr);
    }
    // else do nothing
}

// Check if any thread is currently protecting the given pointer
// Used to determine if a pointer is "safe" to reclaim/free
// Time is O(n) but there shouldn't be many protections at a time
bool isHazard(void* ptr) {
    for (size_t i = 0; i < MAX_HAZARD_POINTERS; i++) {
        // Check both slots, if it is being protected return true
        if (globalHazardPointers[i].ptr1 == ptr || globalHazardPointers[i].ptr2 == ptr) {
            return true;
        }
    }

    // Nothing found, return false
    return false;
}

#endif