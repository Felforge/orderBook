#include <iostream>
#include "orderList.h"
using namespace std;

// Constructor for Order
Order::Order(void* memoryBlock, int orderID, int userID, string side, string ticker, int quantity, double price)
    : memoryBlock(memoryBlock), orderID(orderID), userID(userID), side(side), ticker(ticker), quantity(quantity), price(price) {}

// Constructor for Order Node
// This is a node of the doubly linked list
OrderNode::OrderNode(void* memoryBlock, Order* order) 
    : memoryBlock(memoryBlock), order(order) {
        prev.store(nullptr);
        next.store(nullptr);
    }

OrderList::OrderList(void* memoryBlock, MemoryPool& orderPool, MemoryPool& nodePool)
    : memoryBlock(memoryBlock), orderPool(orderPool), nodePool(nodePool) {}

OrderList::~OrderList() {
    OrderNode* current = tail.load();
    OrderNode* prev;
    while (head.load() != current) {
        prev = current->prev.load();
        orderPool.deallocate(current->order->memoryBlock);
        nodePool.deallocate(current->memoryBlock);
        current = prev;
    }
    if (current) {
        orderPool.deallocate(current->order->memoryBlock);
        nodePool.deallocate(current->memoryBlock);
    }
}

// Lock-free deletion
OrderNode* OrderList::remove(OrderNode* nodePtr) {
    if (!nodePtr) {
        cout << "Order Book Error: Invalid Pointer Removal" << endl;
        return nullptr;
    }

    OrderNode* prev, *next, *headPtr, *tailPtr;

    while (true) {
        prev = nodePtr->prev.load();
        next = nodePtr->next.load();
        headPtr = head.load();
        tailPtr = tail.load();

        if (!headPtr) {
            // List is now empty
            return nullptr;
        } else if (!prev && !next) {
            // Node is the only element in the list
            if (head.compare_exchange_weak(headPtr, nullptr)) {
                tail.store(nullptr); // List is now empty
                orderPool.deallocate(nodePtr->order->memoryBlock);
                nodePool.deallocate(nodePtr->memoryBlock);
                return nodePtr;
            }
        } else if (!prev) {
            // Node is the head
            if (head.compare_exchange_weak(headPtr, next)) {
                next->prev.store(nullptr); // Detach from head
                orderPool.deallocate(nodePtr->order->memoryBlock);
                nodePool.deallocate(nodePtr->memoryBlock);
                return nodePtr;
            }
        } else if (!next) {
            // Node is the tail
            if (tail.compare_exchange_weak(tailPtr, prev)) {
                prev->next.store(nullptr); // Detach from tail
                orderPool.deallocate(nodePtr->order->memoryBlock);
                nodePool.deallocate(nodePtr->memoryBlock);
                return nodePtr;
            }
        } else {
            // Node is in the middle
            if (prev->next.compare_exchange_weak(nodePtr, next)) {
                next->prev.store(prev);
                orderPool.deallocate(nodePtr->order->memoryBlock);
                nodePool.deallocate(nodePtr->memoryBlock);
                return nodePtr;
            }
        }
        // Retry if CAS failed
    }
}
