#ifndef PARALLELORDERBOOK_H
#define PARALLELORDERBOOK_H

#include <string>
#include <unordered_map>
#include <queue>
#include <atomic>
#include <vector>
#include <thread>
#include <vector>
#include "../memoryPool/memoryPool.h"
#include "../lockless/orderList/orderList.h"
#include "../lockless/queue/queue.h"

const int MAX_THREADS = 8; // Laptop
// const int MAX_THREADS = 24; // PC

// For remove requests
struct alignas(64) RemoveRequest {
    void* memoryBlock;
    int ID;
    std::atomic<RemoveRequest*> prev;
    std::atomic<RemoveRequest*> next;
    RemoveRequest(void* memoryBlock, int ID);
};

// Best orders are being left as PriceLevel to access the whole doubly linked list
struct alignas(64) Ticker {
    void* memoryBlock;
    std::string ticker;
    std::unordered_map<double, OrderList*> buyOrderMap;
    std::unordered_map<double, OrderList*> sellOrderMap;
    OrderList* bestBuyOrder; // pointer to current best buy order
    OrderList* bestSellOrder; // pointer to current best sell order
    
    // Priority queues for active price levels
    std::priority_queue<double> priorityBuyPrices; // Max-heap for buy prices
    std::priority_queue<double, std::vector<double>, std::greater<double>> prioritySellPrices; // Min-heap for sell prices
    
    Ticker(void* memoryBlock, std::string ticker);
};

class OrderBook {
    private:
        int orderID;
        int maxTickers; // Max number of tickers that can be added
        int maxOrders; // Max number of orders that can be added
        MemoryPool orderPool; // Memory pool for allocating Orders
        MemoryPool nodePool; // Memory pool for allocating OrderNodes
        MemoryPool priceLevelPool; // Memory pool for allocation PriceLevels
        MemoryPool tickerPool; // Memory pool for allocating Tickers
        MemoryPool removeRequestPool; // Memory pool for allocating remove requests

        // Hold threads
        vector<thread> threads;

        // Requests
        Queue<OrderNode> buyQueue;
        Queue<RemoveRequest> sellQueue;

        // For multithreading
        bool running;
        void startup();
        void shutdown();
        void receiveRequests();
        void processBuy(OrderNode* nodePtr);
        void processSell(RemoveRequest* nodePtr);

    public:
        // Easy access to every order
        // Just make sure to erase from here if the order gets deleted
        std::unordered_map<int, OrderNode*> orderMap;
        std::unordered_map<std::string, Ticker*> tickerMap;
        OrderBook(int numTickers, int maxOrders);
        ~OrderBook();
        void updateBestBuyOrder(std::string ticker);
        void updateBestSellOrder(std::string ticker);
        // void removePriceLevel(std::string side, std::string ticker, double price, PriceLevel* levelPtr);
        void addTicker(std::string ticker);
        void addOrder(int userID, std::string ticker, std::string side, int quantity, double price, bool print = true);
        void removeOrder(int id, bool print = true);
        void matchOrders(std::string ticker, bool print=true, int count=0);
    };

#endif