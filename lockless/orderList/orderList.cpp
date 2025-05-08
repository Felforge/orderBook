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
    : memoryBlock(memoryBlock), orderPool(orderPool), nodePool(nodePool) {
        // Initialize head and tail
        head.store(nullptr);
        tail.store(nullptr);
    }

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

        if (!headPtr) {
            // List is now empty
            return;
        } else if (!prev && !next) {
            // Node is the only element in the list
            if (head.compare_exchange_weak(headPtr, nullptr)) {
                tail.store(nullptr); // List is now empty
                orderPool.deallocate(nodePtr->order->memoryBlock);
                nodePool.deallocate(nodePtr->memoryBlock);
                return;
            }
        } else if (!prev) {
            // Node is the head
            if (head.compare_exchange_weak(headPtr, next)) {
                next->prev.store(nullptr); // Detach from head
                orderPool.deallocate(nodePtr->order->memoryBlock);
                nodePool.deallocate(nodePtr->memoryBlock);
                return;
            }
        } else if (!next) {
            // Node is the tail
            if (tail.compare_exchange_weak(tailPtr, prev)) {
                prev->next.store(nullptr); // Detach from tail
                orderPool.deallocate(nodePtr->order->memoryBlock);
                nodePool.deallocate(nodePtr->memoryBlock);
                return;
            }
        } else {
            // Node is in the middle
            if (prev->next.compare_exchange_weak(nodePtr, next)) {
                next->prev.store(prev);
                orderPool.deallocate(nodePtr->order->memoryBlock);
                nodePool.deallocate(nodePtr->memoryBlock);
                return;
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
