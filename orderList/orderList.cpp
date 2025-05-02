#include <iostream>
#include "orderList.h"
using namespace std;

// Constructor for Order
Order::Order(void* memoryBlock, int orderID, int userID, string side, string ticker, int quantity, double price)
    : memoryBlock(memoryBlock), orderID(orderID), userID(userID), side(side), ticker(ticker), quantity(quantity), price(price) {}

// Constructor for Order Node
// This is a node of the doubly linked list
OrderNode::OrderNode(void* memoryBlock, Order* order) 
    : memoryBlock(memoryBlock), order(order), prev(nullptr), next(nullptr) {}

OrderList::OrderList(void* memoryBlock, MemoryPool& orderPool, MemoryPool& nodePool)
    : memoryBlock(memoryBlock), orderPool(orderPool), nodePool(nodePool) {}

OrderList::~OrderList() {
    OrderNode* current = tail.load();
    OrderNode* next;
    while (head.load() != current) {
        next = current->next.load();
        orderPool.deallocate(current->order->memoryBlock);
        nodePool.deallocate(current->memoryBlock);
        current = next;
    }
    orderPool.deallocate(current->order->memoryBlock);
    nodePool.deallocate(current->memoryBlock);
}

// Lock-free insertion
void OrderList::insert(Order* orderPtr) {
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
void OrderList::remove(OrderNode* nodePtr) {
    if (!nodePtr) {
        cout << "Order Book Error: Invalid Pointer Removal" << endl;
        return;
    }

    OrderNode* prev, *next, *headPtr, *tailPtr;

    while (true) {
        prev = nodePtr->prev.load();
        next = nodePtr->next.load();
        headPtr = head.load();
        tailPtr = tail.load();

        if (!prev && !next) {
            // Node is the only element in the list
            if (head.compare_exchange_weak(headPtr, nullptr)) {
                tail.store(nullptr); // List is now empty
                nodePool.deallocate(nodePtr->memoryBlock);
                return; // Successfully removed
            }
        } else if (!prev) {
            // Node is the head
            if (head.compare_exchange_weak(headPtr, next)) {
                next->prev.store(nullptr); // Detach from head
                nodePool.deallocate(nodePtr->memoryBlock);
                return; // Successfully removed
            }
        } else if (!next) {
            // Node is the tail
            if (tail.compare_exchange_weak(tailPtr, prev)) {
                prev->next.store(nullptr); // Detach from tail
                nodePool.deallocate(nodePtr->memoryBlock);
                return; // Successfully removed
            }
        } else {
            // Node is in the middle
            if (prev->next.compare_exchange_weak(nodePtr, next)) {
                next->prev.store(prev); // Link prev to next
                nodePool.deallocate(nodePtr->memoryBlock);
                return; // Successfully removed
            }
        }
        // Retry if CAS failed
    }
}

void deleteOrderList(double price, unordered_map<double, atomic<OrderList*>>& orderMap, MemoryPool& orderListPool) {
    // Retrieve list
    OrderList* listPtr = orderMap[price].load();

    // Delete list from map
    if (orderMap[price].compare_exchange_weak(listPtr, nullptr)) {
        // Clear list from memory
        // Destructor is triggered here deleting anything within that could remain
        orderListPool.deallocate(listPtr->memoryBlock);

        // Erase price from map
        orderMap.erase(price);
    }
}