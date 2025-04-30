#ifndef PARALLELORDERBOOK_H
#define PARALLELORDERBOOK_H

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include "../memoryPool/memoryPool.h"

const int MAX_THREADS = 8; // Laptop
// const int MAX_THREADS = 24; // PC

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

        int getListIndex(double price); // Gets index in order array for given price

        MemoryPool orderPool; // Pointer to Memory pool for allocating Orders
        MemoryPool nodePool; // Pointer to Memory pool for allocating OrderNodes
        MemoryPool priceLevelPool; // Pointer to Memory pool for allocation PriceLevels
        MemoryPool tickerPool; // Pointer to Memory pool for allocating Tickers

        void processRequests();
        void addTicker(std::string ticker);
        void addOrder(Order* order, bool print);
        void removeOrder(int orderID, bool print);

    public:
        std::unordered_map<int, OrderNode*> orderMap;
        std::unordered_map<std::string, Ticker*> tickerMap;
        WorkerThread(int numTickers, int numOrders);
        ~WorkerThread();
        void requestAddTicker(std::string ticker);
        void requestAddOrder(int userID, std::string ticker, std::string side, int quantity, double price, bool print);
        void requestRemoveOrder(int id, bool print);
};

class OrderBook {
    public:
        OrderBook(int numTickers, int numOrders);
        ~OrderBook();
        void addTicker(std::string ticker);
        void addOrder(int userID, std::string ticker, std::string side, int quantity, double price, bool print = true);
        void removeOrder(int id, bool print = true);
};

#endif