#ifndef HAZARD_POINTERS_H
#define HAZARD_POINTERS_H

#include <atomic>
#include <thread>
#include <cstddef>
#include <cassert>
#include <vector>
#include <cstdio>

// Maximum number of threads that can be supported for hazard pointers
// 32 is the highest I am using for now
constexpr size_t MAX_HAZARD_POINTERS = 32;

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

// Global counter for new slot allocation (inline ensures single instance across all TUs)
inline std::atomic<size_t> globalHazardSlotCounter{0};

// Bitset tracking which slots are in use (lock-free)
inline std::atomic<bool> slotInUse[MAX_HAZARD_POINTERS] = {};

// Allocate a hazard pointer slot (try to reuse freed slots first)
inline size_t allocateHazardSlot() {
    // First pass: try to claim a previously used slot
    for (size_t i = 0; i < MAX_HAZARD_POINTERS; ++i) {
        bool expected = false;
        if (slotInUse[i].compare_exchange_strong(expected, true, std::memory_order_acquire)) {
            return i;
        }
    }

    // No free slots, allocate a new one from the counter
    size_t slot = globalHazardSlotCounter.fetch_add(1, std::memory_order_relaxed);

    // DEBUG: Print diagnostic info before asserting
    if (slot >= MAX_HAZARD_POINTERS) {
        size_t inUseCount = 0;
        for (size_t i = 0; i < MAX_HAZARD_POINTERS; ++i) {
            if (slotInUse[i].load()) inUseCount++;
        }
        fprintf(stderr, "HAZARD SLOT ERROR: counter=%zu, inUse=%zu, MAX=%zu\n",
                slot, inUseCount, MAX_HAZARD_POINTERS);
    }

    assert(slot < MAX_HAZARD_POINTERS && "Too many threads for hazard pointer table");
    slotInUse[slot].store(true, std::memory_order_release);
    return slot;
}

// Free a hazard pointer slot for reuse
inline void freeHazardSlot(size_t slot) {
    // Clear all hazard pointers for this slot
    for (size_t i = 0; i < HAZARD_POINTERS_PER_THREAD; ++i) {
        globalHazardPointers[slot].ptrs[i].store(nullptr, std::memory_order_relaxed);
    }
    // Mark slot as free
    slotInUse[slot].store(false, std::memory_order_release);
}

// RAII guard to automatically free slot when thread exits
struct HazardSlotGuard {
    size_t slot;
    HazardSlotGuard() : slot(allocateHazardSlot()) {}
    ~HazardSlotGuard() { freeHazardSlot(slot); }
};

// Each thread gets a unique slot via RAII guard (auto-freed on thread exit)
thread_local HazardSlotGuard hazardSlotGuard;
thread_local size_t hazardSlot = hazardSlotGuard.slot;

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