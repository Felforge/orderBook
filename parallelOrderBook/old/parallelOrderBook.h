#ifndef PARALLELORDERBOOK_H
#define PARALLELORDERBOOK_H

#include <iostream>
#include <thread>
#include <queue>
#include <string>
#include <vector>
#include <unordered_map>
#include "../memoryPool/memoryPool.h"

const int MAX_THREADS = 8; // Laptop
// const int MAX_THREADS = 24; // PC

const int MAX_PRICE_IDX = 100000;

int getListIndex(double price);

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
    
    Ticker(void* memoryBlock, std::string ticker);
};

struct alignas(64) AddOrderRequest {
    Order* order;
    bool print;

    AddOrderRequest(Order* order, bool print);
};

struct alignas(64) RemoveOrderRequest {
    int orderID;
    bool print;

    RemoveOrderRequest(int orderiD, bool print);
};

class WorkerThread {
    private:
        std::queue<std::string> tickerQueue;
        std::queue<AddOrderRequest> addQueue;
        std::queue<RemoveOrderRequest> removeQueue;
        std::thread worker;
        bool stop;

        int maxTickers; // Max number of tickers that can be added
        int maxOrders; // Max number of orders that can be added

        MemoryPool orderPool; // Pointer to Memory pool for allocating Orders
        MemoryPool nodePool; // Pointer to Memory pool for allocating OrderNodes
        MemoryPool priceLevelPool; // Pointer to Memory pool for allocation PriceLevels
        MemoryPool tickerPool; // Pointer to Memory pool for allocating Tickers

        void processRequests();
        void addTicker(std::string ticker);
        void addOrder(Order* order, bool print);
        void removeOrder(int orderID, bool print);

    public:
        void* memoryBlock; // Store memory address for deallocation
        std::unordered_map<int, OrderNode*> orderMap;
        std::unordered_map<std::string, Ticker*> tickerMap;
        WorkerThread(void* inpMemoryBlock, int numTickers, int numOrders);
        ~WorkerThread();
        void requestAddTicker(std::string ticker);
        void requestAddOrder(int userID, int orderID, std::string ticker, std::string side, int quantity, double price, bool print);
        void requestRemoveOrder(int id, bool print);
};

class OrderBook {
    public:
        OrderBook(int numTickers, int numOrders);
        ~OrderBook();
        void addTicker(std::string ticker);
        void addOrder(int userID, std::string ticker, std::string side, int quantity, double price, bool print = true);
        void removeOrder(int id, bool print = true);

    private:
        // Stores all the worker thread instances
        WorkerThread* instances[MAX_THREADS - 1];

        // Counter for order ID
        int orderID;
        
        MemoryPool threadPool; // Pointer to Memory pool for allocating Worker Threads

        // Priority queues for active price levels
        std::priority_queue<int> activeBuyPrices; // Max-heap for buy prices
        std::priority_queue<int, std::vector<int>, std::greater<int>> activeSellPrices; // Min-heap for sell prices

        // Queue for tracking recent orders for matching
        std::queue<std::pair<int, int>> buyQueue[MAX_PRICE_IDX];
        std::queue<std::pair<int, int>> sellQueue[MAX_PRICE_IDX];
};

#endif