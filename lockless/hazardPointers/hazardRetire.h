#ifndef HAZARD_RETIRE_H
#define HAZARD_RETIRE_H

#include <vector>
#include <functional>
#include "hazardPointers.h"

// Each thread keeps a local list of nodes it has retired
// A node will only be reclaimed once no other threads protect it
thread_local std::vector<void*> retireList;

// Free nodes from retireList if no longer protected
void updateRetireList(std::function<void(void*)> deletionFunc) {
    // surivors will hold still hazardous nodes
    std::vector<void*> survivors;

    // iterate through all nodes
    for (void* ptr : retireList) {
        // Only reclaim the node if no thread is currently protecting it with a hazard pointer
        if (isHazard(ptr)) {
            // Still in use, keep it for later checking
            survivors.push_back(ptr);
        } else {
            // Safe to free node
            deletionFunc(ptr);
        }
    }

    // Set retireList to survivors
    retireList = std::move(survivors);
}

// Retire a given node for later deletion
// deletionFunc is a function which destroys the object when safe
void retireObj(void* obj, std::function<void(void*)> deletionFunc) {
    // Add the object to this thread's retire list
    retireList.push_back(obj);

    // If the retire list has grown large enough, attempt to reclaim nodes
    // This threshold can be tuned for performance/memory tradeoff
    // Potentially find a way to eliminate this boundary later
    if (retireList.size() >= 64) {
        updateRetireList(deletionFunc);
    }
}

#endif