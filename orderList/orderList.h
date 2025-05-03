#include <atomic>
#include <unordered_map>
#include "../memoryPool/memoryPool.h"

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

class OrderList {
    private:
        MemoryPool& orderPool; // reference to orderPool object
        MemoryPool& nodePool; // reference to nodePool object

    public:
        void* memoryBlock; // Pointer to memory block that OrderList is stored in
        std::atomic<OrderNode*> head;
        std::atomic<OrderNode*> tail;

        OrderList(void* memoryBlock, MemoryPool& orderPool, MemoryPool& nodePool);
        ~OrderList();
        void insert(OrderNode* nodePtr);
        void remove(OrderNode* nodePtr);
};

// Lock-free deletion of OrderList object
void deleteOrderList(double price, std::unordered_map<double, std::atomic<OrderList*>> orderMap, MemoryPool& orderListPool);