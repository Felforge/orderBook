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
#include "../lockless/priorityQueue/priorityQueue.h"

const int MAX_THREADS = 8; // Laptop
// const int MAX_THREADS = 24; // PC

// For buy requests
struct alignas(64) BuyRequest {
    void* memoryBlock;
    OrderNode* orderNode;
    void* levelBlock;
    void* queueBlock;
    std::atomic<BuyRequest*> prev;
    std::atomic<BuyRequest*> next;
    BuyRequest(void* memoryBlock, OrderNode* orderNode, void* levelBlock, void* queueBlock);
};

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
    std::vector<OrderList*> buyOrderList;
    std::vector<OrderList*> sellOrderList;
    std::atomic<int> bestBuyIdx; // List index of best buy order
    std::atomic<int> bestSellIdx; // List index of best sell order
    
    // Priority queues for active price levels
    std::vector<bool> activeBuyLevels; // Track active buy price levels
    std::vector<bool> activeSellLevels; // Track active sell price levels
    PriorityQueue priorityBuyPrices; // Queue of buy prices
    PriorityQueue prioritySellPrices; // Reverse queue of sell prices
    
    Ticker(void* memoryBlock, std::string ticker, int numLevels);
};

class OrderBook {
    private:
        int orderID;
        int maxTickers; // Max number of tickers that can be added
        int maxOrders; // Max number of orders that can be added
        double minPrice; // Minimum price available for orders
        double maxPrice; // Maximum price available for orders
        int numLevels; // Number of available price levels
        MemoryPool orderPool; // Memory pool for allocating Orders
        MemoryPool nodePool; // Memory pool for allocating OrderNodes
        MemoryPool priceLevelPool; // Memory pool for allocation PriceLevels
        MemoryPool tickerPool; // Memory pool for allocating Tickers
        MemoryPool buyRequestPool; // Memory pool for allocating buy requests
        MemoryPool removeRequestPool; // Memory pool for allocating remove requests
        MemoryPool priceQueuePool; // Memory pool for allocating price queue nodes

        // Hold threads
        std::vector<std::thread> threads;

        // Requests
        Queue<BuyRequest> buyQueue;
        Queue<RemoveRequest> sellQueue;

        // For order lists
        int getListIdx(double price);

        // For multithreading
        bool running;
        void startup();
        void shutdown();
        void receiveRequests();
        void processBuy(BuyRequest* nodePtr);
        void processSell(RemoveRequest* nodePtr);

        // Utility functions
        void updateBestIdx(PriorityQueue &queue, std::vector<bool> &activeLevels, std::atomic<int> &bestIdx);

    public:
        // Easy access to every order
        // Just make sure to erase from here if the order gets deleted
        std::vector<OrderNode*> orders;
        std::unordered_map<std::string, Ticker*> tickerMap;
        OrderBook(int numTickers, int maxOrders, double inpMinPrice, double inpMaxPrice);
        ~OrderBook();
        void addTicker(std::string ticker);
        void addOrder(int userID, std::string ticker, std::string side, int quantity, double price, bool print = true);
        void removeOrder(int id, bool print = true);
        void matchOrders(std::string ticker, bool print=true, int count=0);

        // For testing
        std::atomic<int> ordersProcessed;
    };

#endif