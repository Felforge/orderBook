#ifndef CPUORDERBOOK_H
#define CPUORDERBOOK_H

#include <string>
#include <unordered_map>
#include <queue>
#include "../memoryPool/memoryPool.h"

const int MAX_PRICE_IDX = 100000;

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
    OrderNode* prev;
    OrderNode* next;
    OrderNode(void* memoryBlock, Order* order);
};

struct alignas(64) PriceLevel {
    void* memoryBlock;
    OrderNode* head;
    OrderNode* tail;
    PriceLevel(void* memoryBlock, OrderNode* orderNode);
};

// Best orders are being left as PriceLevel to access the whole doubly linked list
struct alignas(64) Ticker {
    void* memoryBlock;
    std::string ticker;
    PriceLevel* buyOrderList[MAX_PRICE_IDX];
    PriceLevel* sellOrderList[MAX_PRICE_IDX];
    PriceLevel* bestBuyOrder; // pointer to current best buy order
    PriceLevel* bestSellOrder; // pointer to current best sell order
    
    // Priority queues for active price levels
    std::priority_queue<int> activeBuyPrices; // Max-heap for buy prices
    std::priority_queue<int, std::vector<int>, std::greater<int>> activeSellPrices; // Min-heap for sell prices
    
    Ticker(void* memoryBlock, std::string ticker);
};

class OrderBook {
    private:
        int orderID;
        int getListIndex(double price); // Gets index in order array for given price
        MemoryPool orderPool; // Memory pool for allocating Orders
        MemoryPool nodePool; // Memory pool for allocating OrderNodes
        MemoryPool priceLevelPool; // Memory pool for allocation PriceLevels
        MemoryPool tickerPool; // Memory pool for allocating Tickers

    public:
        // Easy access to every order
        // Just make sure to erase from here if the order gets deleted
        std::unordered_map<int, OrderNode*> orderMap;
        std::unordered_map<std::string, Ticker*> tickerMap;
        OrderBook();
        ~OrderBook();
        void addTicker(std::string ticker);
        void addOrder(int userID, std::string ticker, std::string side, int quantity, double price, bool print = true);
        void removeOrder(int id, bool print = true);
        void matchOrders(std::string ticker, bool print=true, int count=0);
    };

#endif