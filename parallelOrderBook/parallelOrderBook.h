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
#include "../lockless/queue/queue.h"

// NUM_THREADS will be set to the maximum supported amount
const int NUM_THREADS = std::thread::hardware_concurrency();

// Order struct
struct Order {
    // For memory pool
    void* memoryBlock;

    // Unique identifier
    int orderID;

    // User/client ID
    int userID;

    // Order price
    double price;

    // Order quantity
    int quantity;

    // "BUY" or "SELL"
    std::string side;

    // Symbol name
    std::string ticker;

    // Constructor matching cpuOrderBook interface
    Order(void* memoryBlock, int orderID, int userID, std::string side,
        std::string ticker, int quantity, double price);
  };

// Best orders are being left as PriceLevel to access the whole doubly linked list
struct alignas(64) Ticker {
    void* memoryBlock;
    std::string ticker;
    
    // Only accessed by a single thread
    std::unordered_map<double, Order*> buyOrderMap;
    std::unordered_map<double, Order*> sellOrderMap;

    // Atomic for cross-thread reads
    std::atomic<double> bestBuyPrice{0.0};
    std::atomic<double> bestSellPrice{0.0};
    
    Ticker(void* memoryBlock, std::string ticker) : memoryBlock(memoryBlock), ticker(ticker) {}
};

// Struct to hold all memory pools for a given thread
struct alignas(64) Pools {

};

template<size_t maxTickers>
class OrderBook {
    private:
        // Universal order ID
        std::atomic<int> orderID{0};

        // Hold matching threads
        // One thread per ticker
        std::thread matchingThreads[maxTickers];

        // Orders to be tracked
        LocklessQueue<Order> toTrack;

        // Map to hold all deques
        std::unordered_map<std::string, LocklessQueue<Order>*> symbolQueues;

        // Thread function to intake a given order
        void intakeOrder(int userID, std::string ticker, std::string side, int quantity, double price) {
            // Create order object
            Order* order = nullptr; // Add createOrder later

            // Add order to be tracked
            toTrack.pushRight(order, nullptr); // Add thread local memory pool later

            // Add order to order book
            symbolQueues[ticker]->pushRight(order, nullptr); // Add thread local memory pool later
        }

    public:
        OrderBook(int numTickers, int maxOrders, double inpMinPrice, double inpMaxPrice);
        ~OrderBook();
        void addOrder(int userID, std::string ticker, std::string side, int quantity, double price, bool print = true);

        // For testing
        std::atomic<int> ordersProcessed;
    };

#endif