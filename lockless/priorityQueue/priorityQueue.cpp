#include "priorityQueue.h"

PriorityQueue::PriorityQueue(bool inpReverse) {
    // Initialize head and tail
    head.store(nullptr);
    tail.store(nullptr);

    // Save reverse bool
    reverse = inpReverse;
}

void PriorityQueue::insert(Node<int>* nodePtr) {
    Node<int>* headPtr = head.load();
    Node<int>* tailPtr = tail.load();
    Node<int>* current = headPtr;

    // Find spot
    // Contention should not affect this logic
    while (current && (!reverse && current->data > nodePtr->data || reverse && current->data < nodePtr->data)) {
        current = current->next.load();
    }

    // Insert node
    while(true) {
        if (!headPtr) {
            // List is empty, set both head and tail to newNode
            if (head.compare_exchange_weak(headPtr, nodePtr)) {
                tail.store(nodePtr);
                return;
            }
        } else if (!current) {
            // Update the next pointer of the current tail
            Node<int>* nullNode = nullptr;
            if (tailPtr->next.compare_exchange_weak(nullNode, nodePtr)) {
                // Successfully updated tail's next, now update tail
                nodePtr->prev.store(tailPtr);
                tail.store(nodePtr);
                return;
            } else {
                // Failed to update, reload tailPtr and retry
                tailPtr = tail.load();
            }
        } else if (!current->prev.load()) {
            // Insert as head
            Node<int>* nullNode = nullptr;
            if (headPtr->prev.compare_exchange_weak(nullNode, nodePtr)) {
                // Successfully updated tail's next, now update tail
                nodePtr->next.store(headPtr);
                head.store(nodePtr);
                return;
            } else {
                // Failed to update, reload tailPtr and retry
                tailPtr = tail.load();
            }
        } else {
            // Insert before current
            if (current->prev.load()->next.compare_exchange_weak(current, nodePtr)) {
                current->prev.store(nodePtr);
                return;
            }
        }
    }
}