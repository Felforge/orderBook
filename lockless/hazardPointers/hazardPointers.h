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

// Number of hazard pointers per thread
constexpr size_t HAZARD_POINTERS_PER_THREAD = 8;

// Struct representing two hazard pointer slot for a thread
struct HazardPointer {
    // Four atomic hazard pointer slots
    std::atomic<void*> ptrs[HAZARD_POINTERS_PER_THREAD];

    // Set both slots to nullptr on construction
    HazardPointer() {
        for (size_t i = 0; i < HAZARD_POINTERS_PER_THREAD; ++i) {
            ptrs[i].store(nullptr, std::memory_order_relaxed);
        }
    }
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
    // Skip if pointer is null
    if (!ptr) {
        return;
    }

    // Check all ptrs one by one
    for (size_t i = 0; i < HAZARD_POINTERS_PER_THREAD; i++) {
        // If the slot is empty, set it to the pointer
        if (globalHazardPointers[hazardSlot].ptrs[i].load() == nullptr) {
            globalHazardPointers[hazardSlot].ptrs[i].store(ptr, std::memory_order_release);

            // Exit after setting the pointer
            return;
        }
    }

    assert(false && "No free hazard pointer slot for this thread");
}

// Remove a given hazard pointer
// Nothing will happen if the pointer is not protected
void removeHazardPointer(void* ptr) {
    // Check for ptr
    for (size_t i = 0; i < HAZARD_POINTERS_PER_THREAD; i++) {
        // If the slot equals to ptr set it to nullptr
        if (globalHazardPointers[hazardSlot].ptrs[i].load() == ptr) {
            globalHazardPointers[hazardSlot].ptrs[i].store(nullptr, std::memory_order_release);

            // Exit after removing the pointer
            return;
        }
    }
}

// Check if any thread is currently protecting the given pointer
// Used to determine if a pointer is "safe" to reclaim/free
// Time is O(n^2) but there shouldn't be many protections at a time
bool isHazard(void* ptr) {
    for (size_t i = 0; i < MAX_HAZARD_POINTERS; i++) {
        // Check all four slots, if it is being protected return true
        for (size_t j = 0; j < HAZARD_POINTERS_PER_THREAD; j++) {
            // If the slot equals to ptr return true
            if (globalHazardPointers[i].ptrs[j].load(std::memory_order_acquire) == ptr) {
                return true;
            }
        }
    }

    // Nothing found, return false
    return false;
}

#endif