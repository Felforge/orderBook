#ifndef ORDERLIST_H
#define ORDERLIST_H

#include <atomic>
#include <unordered_map>
#include <string>
#include "../../memoryPool/memoryPool.h"
#include "../queue/queue.h"

struct alignas(64) Order {
    void* memoryBlock;
    int orderID;
    int userID;
    double price;
    int quantity;
    std::string side;
    std::string ticker;
    Order(void* memoryBlock, int orderID, int userID, std::string side, std::string ticker, int quantity, double price);
};

struct alignas(64) OrderNode {
    void* memoryBlock;
    Order* order;
    std::atomic<OrderNode*> prev;
    std::atomic<OrderNode*> next;
    OrderNode(void* memoryBlock, Order* order);
};

class OrderList : public Queue<OrderNode> {
    private:
        MemoryPool& orderPool; // reference to orderPool object
        MemoryPool& nodePool; // reference to nodePool object

    public:
        void* memoryBlock; // Pointer to memory block that OrderList is stored in

        OrderList(void* memoryBlock, MemoryPool& orderPool, MemoryPool& nodePool);
        ~OrderList();
        OrderNode* remove(OrderNode* nodePtr) override;
};

// Lock-free deletion of OrderList object
void deleteOrderList(double price, std::unordered_map<double, std::atomic<OrderList*>> orderMap, MemoryPool& orderListPool);

#endif