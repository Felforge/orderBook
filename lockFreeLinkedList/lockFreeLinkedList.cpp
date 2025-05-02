#include <iostream>
#include "lockFreeLinkedList.h"
using namespace std;

// Constructor for Order
Order::Order(void* memoryBlock, int orderID, int userID, string side, string ticker, int quantity, double price)
    : memoryBlock(memoryBlock), orderID(orderID), userID(userID), side(side), ticker(ticker), quantity(quantity), price(price) {}

// Constructor for Order Node
// This is a node of the doubly linked list
OrderNode::OrderNode(void* memoryBlock, Order* order) 
    : memoryBlock(memoryBlock), order(order), prev(nullptr), next(nullptr) {}

LockFreeLinkedList::LockFreeLinkedList(MemoryPool& nodePool)
    : nodePool(nodePool) {}

LockFreeLinkedList::~LockFreeLinkedList() {
    OrderNode* current = tail.load();
    OrderNode* next;
    while (head.load() != current) {
        next = current->next.load();
        nodePool.deallocate(current->memoryBlock);
        current = next;
    }
    nodePool.deallocate(current->memoryBlock);
}

// Lock-free insertion
void LockFreeLinkedList::insert(Order* orderPtr) {
    // Allocate memory and declare OrderNode
    void* memoryBlock = nodePool.allocate();
    OrderNode* newNode = new (memoryBlock) OrderNode(memoryBlock, orderPtr);

    OrderNode* tailPtr = tail.load();
    
    while (true) {
        if (!tailPtr) {
            // List is empty, set both head and tail to newNode
            if (head.compare_exchange_weak(tailPtr, newNode)) {
                tail.store(newNode);
                return;
            }
        } else {
            // Update the next pointer of the current tail
            OrderNode* nullNode = nullptr;
            if (tailPtr->next.compare_exchange_weak(nullNode, newNode)) {
                // Successfully updated tail's next, now update tail
                tail.compare_exchange_weak(tailPtr, newNode);
                return;
            } else {
                // Failed to update, reload tailPtr and retry
                tailPtr = tail.load();
            }
        }
    }
}

// Lock-free deletion
void LockFreeLinkedList::remove(OrderNode* nodePtr) {
    if (!nodePtr) {
        cout << "Order Book Error: Invalid Pointer Removal" << endl;
        return;
    }

    OrderNode* prev = nodePtr->prev.load();
    OrderNode* next = nodePtr->next.load();
    OrderNode* headPtr = head.load();
    OrderNode* tailPtr = tail.load();

    if (!prev && !next) {
        // Swap head with nullptr
        if (head.compare_exchange_weak(headPtr, nullptr)) {
            tail.store(nullptr);
        }
    } else if (!prev) {
        // Swap and deattach head
        while (!head.compare_exchange_weak(headPtr, next)) {
            headPtr = head.load(); // CAS failed, retry
        }
        next->prev.store(nullptr);
    } else if (!next) {
        // Swap and deattach tail
        while (!tail.compare_exchange_weak(tailPtr, prev)) {
            tailPtr = tail.load(); // CAS failed, retry
        }
        prev->next.store(nullptr);
    } else {
        // Removing a node in the middle
        prev->next.store(next);
        next->prev.store(prev);
    }
    nodePool.deallocate(nodePtr->memoryBlock);
}