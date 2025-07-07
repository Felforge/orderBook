#ifndef HAZARD_RETIRE_H
#define HAZARD_RETIRE_H

#include <vector>
#include <functional>
#include "hazardPointers.h"

// Each thread keeps a local list of nodes it has retired
// A node will only be reclaimed once no other threads protect it
thread_local std::vector<void*> retireList;

// Free nodes from retireList if no longer protected
template<typename Node>
void updateRetireList(std::function<void(Node*)> deletionFunc) {
    // surivors will hold still hazardous nodes
    std::vector<void*> survivors;

    for (void* ptr : retireList) {
        // Only reclaim the node if no thread is currently protecting it with a hazard pointer
        if (isHazard(ptr)) {
            // Still in use, keep it for later checking
            survivors.push_back(ptr);
        } else {
            // Safe to free node
            deletionFunc(static_cast<Node*>(ptr));
        }
    }

    // Set retireList to survivors
    retireList = std::move(survivors);
}

// Retire a given node for later deletion
// deletionFunc is a function which destroys the node when safe
template<typename Node>
void retireNode(Node* node, std::function<void(Node*)> deletionFunc) {
    // Add the node to this thread's retire list
    retireList.push_back(node);

    // If the retire list has grown large enough, attempt to reclaim nodes
    // This threshold can be tuned for performance/memory tradeoff
    // Potentially find a way to eliminate this boundary later
    if (retireList.size() >= 64) {
        updateRetireList<Node>(deletionFunc);
    }
}

#endif