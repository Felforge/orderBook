#ifndef HAZARD_GUARD_H
#define HAZARD_GUARD_H

#include <thread>
#include "hazardPointers.h"

// RAII wrapper for hazard pointers
template<typename T>
class HazardGuard {
public:  // Make ptr public to match your usage pattern
    T* ptr;

    explicit HazardGuard(T* p, bool isDummy=false) : ptr(p) {
        if (ptr != nullptr && !isDummy) {  // Check for null pointer
            setHazardPointer(ptr);
        }
    }

    ~HazardGuard() {
        if (ptr != nullptr) {  // Check for null pointer
            removeHazardPointer(ptr);
        }
    }

    // Get pointer method - returns pointer, not value
    T* getPtr() const {
        return ptr;
    }

    // Prevent copying
    HazardGuard(const HazardGuard&) = delete;
    HazardGuard& operator=(const HazardGuard&) = delete;
    
    // Allow moving to transfer ownership
    HazardGuard(HazardGuard&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;  // Prevent double release
    }
    
    // Add move assignment to allow reassignment in your code
    HazardGuard& operator=(HazardGuard&& other) noexcept {
        if (this != &other) {
            if (ptr != nullptr) {
                removeHazardPointer(ptr);
            }
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
};

#endif