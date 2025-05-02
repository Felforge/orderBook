#include <atomic>
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

class LockFreeLinkedList {
    private:
        MemoryPool& nodePool; // reference to MemoryPool object

    public:
        std::atomic<OrderNode*> head;
        std::atomic<OrderNode*> tail;

        LockFreeLinkedList(MemoryPool& nodePool);
        ~LockFreeLinkedList();
        void insert(Order* orderPtr);
        void remove(OrderNode* nodePtr);
};