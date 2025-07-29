#ifndef PARALLELORDERBOOK_H
#define PARALLELORDERBOOK_H

#include <string>
#include <unordered_map>
#include <queue>
#include <atomic>
#include <vector>
#include <thread>
#include "../memoryPool/memoryPool.h"
#include "../lockless/queue/queue.h"
#include "../lockless/MPSCQueue/MPSCQueue.h"

// Default best bid
const double BEST_BID = 0.0;

// Default best ask
const double BEST_ASK = 999999.0;

// NUM_THREADS will be set to the maximum supported amount
const int NUM_THREADS = std::thread::hardware_concurrency();

// Enum for order side
enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};

// Type alias for symbol identifiers
using SymbolID = uint16_t;

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

    // Order side
    Side side;

    // Symbol identifier
    SymbolID symbolID;

    // Constructor
    Order(void* memoryBlock, int orderID, int userID, Side side,
        SymbolID symbolID, int quantity, double price);
  };

// Best orders are being left as PriceLevel to access the whole doubly linked list
struct alignas(64) Symbol {
    void* memoryBlock;
    std::string symbol;
    SymbolID symbolID;

    // Atomic is not needed as only the matching thread will read or write to this
    double bestBid = BEST_BID;
    double bestAsk = BEST_ASK;

    // Priority queues for tracking best bids and asks
    std::priority_queue<double> activeBuyPrices;
    std::priority_queue<double, std::vector<double>, std::greater<double>> activeSellPrices;
    
    // Struct contructor
    Symbol(void* memoryBlock, std::string symbol, SymbolID symbolID) 
        : memoryBlock(memoryBlock), symbol(symbol), symbolID(symbolID) {}
};

// Struct to hold all memory pools for a given thread
// maxOrders is the max per thread
template<int maxOrders>
struct alignas(64) Pools {
    // For actually allocating the orders
    MemoryPool<sizeof(Order), maxOrders> *orderPool;

    // For putting the orders in the order book
    MemoryPool<sizeof(Node<Order*>), maxOrders> *nodePool;

    // For tracking the orders
    // This pool can likely be made smaller but I rather be safe
    MemoryPool<sizeof(Node<Order*>), maxOrders> *trackerNodePool;

    // Constructor
    Pools() {
        // Create orderPool
        orderPool = new MemoryPool<sizeof(Order*), maxOrders>();

        // Create nodePool
        nodePool = new MemoryPool<sizeof(Node<Order*>), maxOrders>();

        // Create trackerNodePool
        trackerNodePool = new MemoryPool<sizeof(Node<Order*>), maxOrders>();
    }
};

// Struct to hold order parameters
struct OrderParams {
    int userID;
    SymbolID symbolID;
    Side side;
    int quantity;
    double price;

    // Constructor
    OrderParams(int userID, SymbolID symbolID, Side side, int quantity, double price)
        : userID(userID), symbolID(symbolID), side(side), quantity(quantity), price(price) {}
};

// Define thread local pools
template<size_t maxOrders>
thread_local Pools<maxOrders> pools;

// Define thread local order tracking
template<size_t maxOrders>
thread_local MPSCQueue<int, maxOrders> myRemovalQueue;
thread_local std::unordered_map<int, Order*> myOrders;

// Max orders is given per thread
// This design uses a thread to match each symbol
// This would not scale to thousands of symbols
// For that many symbols a different approach shopuld be used
template<size_t maxSymbols, size_t maxOrders>
class OrderBook {
    private:
        // Number of worker threads
        const int WORKERS = NUM_THREADS - maxSymbols;

        // Universal order ID
        std::atomic<int> orderID{0};

        // Maps to hold all deques
        std::unordered_map<SymbolID, std::unordered_map<double, LocklessQueue<Order*>>> symbolBuyQueues;
        std::unordered_map<SymbolID, std::unordered_map<double, LocklessQueue<Order*>>> symbolSellQueues;
        
        // Map to hold all symbols with their price level data
        std::unordered_map<SymbolID, Symbol*> symbols;
        
        // Symbol registration maps
        std::unordered_map<std::string, SymbolID> symbolNameToID;
        std::unordered_map<SymbolID, std::string> symbolIDToName;
        std::atomic<SymbolID> nextSymbolID{1};
        
        // Memory pool for symbols
        MemoryPool<sizeof(Symbol), maxSymbols> symbolPool;

        // Create flag to control worker threads
        std::atomic<bool> running{false};

        // Lockless queues of pending requests
        LocklessQueue<OrderParams> pendingAddOrders;
        LocklessQueue<int> pendingRemoveOrders;

        // Memory pool for pending order nodes
        // Number of blocks might need to be higher
        MemoryPool<sizeof(Node<OrderParams>), maxOrders> *pendingAddPool;
        MemoryPool<sizeof(Node<int>), maxOrders> *pendingRemovePool;

        // Vector to hold threads
        std::vector<std::thread> threads;

        // To track removal queues
        // This is used to take an Order ID out of foreign thread local tracking
        MPSCQueue<int, maxOrders>* allRemovalQueues[WORKERS];

        // To track all orders universally
        std::unordered_map<int, Order*>* allOrderMaps[WORKERS];

        // MPSC Queues to hold requested price levels
        std::unordered_map<SymbolID, MPSCQueue<double>> requestedBuyPriceLevels;
        std::unordered_map<SymbolID, MPSCQueue<double>> requestedSellPriceLevels;

        // Register a symbol and return its fast ID
        SymbolID registerSymbol(const std::string& symbolName) {
            // Make sure limit is not exceeded
            assert(symbolNameToID.size() < maxSymbols && "Cannot add another symbol, limit reached");

            // Check if already registered
            auto it = symbolNameToID.find(symbolName);
            if (it != symbolNameToID.end()) {
                return it->second;
            }
            
            // Create new symbol ID
            SymbolID newID = nextSymbolID.fetch_add(1);
            
            // Register mappings
            symbolNameToID[symbolName] = newID;
            symbolIDToName[newID] = symbolName;

            // Create price level request queues
            requestedBuyPriceLevels[symbolID] = MPSCQueue<double>();
            requestedSellPriceLevels[symbolID] = MPSCQueue<double>();
            
            // Create symbol object with price tracking
            void* symbolMemory = symbolPool.allocate();
            symbols[newID] = new (symbolMemory) Symbol(symbolMemory, symbolName);

            // Start matching thread on this symbol
            threads.emplace_back([&, newID]{
                matchOrders(newID);
            })
            
            // Return symbol ID
            return newID;
        }
        
        // Convert side string to enum
        Side parseSide(const std::string& sideStr) {
            // Parse input
            if (sideStr == "BUY") {
                return Side::BUY;
            } else if (sideStr == "SELL") { 
                return Side::SELL;
            }

            // Throw error if side is invalid
            throw std::invalid_argument("Invalid side: " + sideStr);
        }

        // Create and return an order pointer
        // This function is meant to be run on an external thread
        Order* createOrder(int userID, SymbolID symbolID, Side side, int quantity, double price) {
            // Allocate memory block
            void* memoryBlock = pools<maxOrders>.orderPool.allocate();

            // Retrieve order ID
            int currID  = orderID.fetch_add(1);

            // Return order pointer
            return new (memoryBlock) Order(memoryBlock, currID, userID, side, symbolID, quantity, price);
        }

        // Thread function to intake a given order
        void intakeOrder(int userID, SymbolID symbolID, Side side, int quantity, double price) {
            // Create order object
            Order* order = createOrder(userID, symbolID, side, quantity, price);

            // Track order locally
            myOrders[order->orderID] = order;

            // Check if price level exists and if not create it
            if (side == 0) {
                // Cache symbol buy queues reference
                auto& symbolBuyLevels = symbolBuyQueues[symbolID];
                
                // Check if key exists
                if (symbolBuyLevels.find(price) == symbolBuyLevels.end()) {
                    // Create request to add price level
                    requestedBuyPriceLevels[symbolID].push(price);

                    // Yield while price level is not added
                    while (symbolBuyLevels.find(price) == symbolBuyLevels.end()) {
                        std::this_thread::yield();
                    }
                }

                // Push order to cached price level
                symbolBuyLevels[price].pushRight(order, pools<maxOrders>.nodePool);
            } else { // side == 1
                // Cache symbol sell queues reference
                auto& symbolSellLevels = symbolSellQueues[symbolID];
                
                // Check if key exists
                if (symbolSellLevels.find(price) == symbolSellLevels.end()) {
                    // Create request to add price level
                    requestedSellPriceLevels[symbolID].push(price);

                    // Yield while price level is not added
                    while (symbolSellLevels.find(price) == symbolSellLevels.end()) {
                        std::this_thread::yield();
                    }
                }
                // Push order to cached price level
                symbolSellLevels[price].pushRight(order, pools<maxOrders>.nodePool);
            }
        }

        // Thread function to remove a given order ID
        // For this implementation it is assumed that there will not be duplicate cancellations
        void wokrerRemoveOrder(int threadNum, int orderID) {
            // Get order pointer
            Order* orderPtr = nullptr;

            // Check local storage
            auto it = myOrders.find(orderID);

            // If key is not found
            if (it == myOrders.end()) {
                // Check all threads
                for (int i = 0; i < WORKERS; i++) {
                    // Already checked
                    if (i == threadNum) {
                        continue;
                    }

                    // Check foreign thread in the same way
                    it = allOrderMaps[i]->find(orderID);

                    // If it is found
                    if (it != allOrderMaps[i]->end()) {
                        // Set orderPtr
                        orderPtr = it->second;

                        // Store for removal
                        allRemovalQueues[i]->push(orderID);

                        // Break out of loop
                        break;
                    }
                }
            
            // Else it is found locally, do everything right here
            } else {
                // Set orderPtr
                orderPtr = it->second;

                // Erase order from map
                myOrders.erase(orderID);
            }

            // If order is never found throw an invalid argument error
            if (!orderPtr) {
                throw std::invalid_argument;
            }

            // Run removeNode on order pointer
            symbolQueues[orderPtr->symbolID].removeNode(orderPtr);
        }

        // Intake worker thread function
        void workerThread(int numThread) {
            // Create order tracking mechanism
            allRemovalQueues[numThread] = &myRemovalQueue<maxOrders>;
            allOrderMaps[numThread] = &myOrders;

            // While running flag is set
            while (running.load()) {
                // Attempt to pop from pendingAddOrders
                std::optional<OrderParams> params = pendingAddOrders.popLeft();

                // If params has a value intake it
                if (params.has_value()) {
                    // Get params out of optional wrapper
                    OrderParams paramsObj = *params;
                    intakeOrder(paramsObj.userID, paramsObj.symbolID, paramsObj.side, paramsObj.quantity, paramsObj.price);

                    // Go into next loop iteration
                    continue;
                }

                // Attempt to pop from pendingRemoveOrders;
                std::optional<int> ID = pendingRemoveOrders.popLeft();

                // If ID had a value remove it
                if (ID.has_value()) {
                    // Remove the given ID
                    wokrerRemoveOrder(threadNum, *ID);

                    // Go into next loop iteration
                    continue;
                }

                // Check if any orders need to be removed from tracking
                int removalID = -1;
                while (myRemovalQueue.pop(&removalID)) {
                    // Erase ID
                    myOrders.erase(removalID);
                }

                // If removalID was used avoid the yield
                if (removalID != -1) {
                    continue;
                }

                // Yield if nothing to do
                std::this_thread::yield();
            }
        }

        // Thread function to match orders on a given symbol
        void matchOrders(SymbolID symbol) {
            // Start infinite loop when start flag is thrown
            while (running.load()) {
                // Add price levels if requested
                while (!requestedBuyPriceLevels[symbol].isEmpty()) {
                    // Create variable to hold result
                    double newPriceLevel;

                    // Get requested price level
                    requestedBuyPriceLevels[symbol].pop(&newPriceLevel);
                    
                    // If it has already been added go on
                    if (symbolBuyQueues[symbol].find(newPriceLevel) != symbolBuyQueues[symbol].end()) {
                        continue;
                    }

                    // Add price level
                    symbolBuyQueues[symbol][newPriceLevel] = LocklessQueue<Order*>();

                    // Add to active buy prices
                    symbols[symbol]->activeBuyPrices.push(newPriceLevel);

                    // Set new best buy price if applicable
                    if (newPriceLevel > symbols[symbol]->bestBid) {
                        symbols[symbol]->bestBid = newPriceLevel;
                    }
                }

                while (!requestedSellPriceLevels[symbol].isEmpty()) {
                    // Create variable to hold result
                    double newPriceLevel;

                    // Get requested price level
                    requestedSellPriceLevels[symbol].pop(&newPriceLevel);

                    // If it has already been added go on
                    if (symbolSellQueues[symbol].find(newPriceLevel) != symbolSellQueues[symbol].end()) {
                        continue;
                    }

                    // Add price level
                    symbolSellQueues[symbol][newPriceLevel] = LocklessQueue<Order*>();

                    // Add to active sell prices
                    symbols[symbol]->activeSellPrices.push(newPriceLevel);

                    // Set new best buy price if applicable
                    if (newPriceLevel < symbols[symbol]->bestAsk) {
                        symbols[symbol]->bestAsk = newPriceLevel;
                    }
                }

                // Match orders if needed
                // Add if statement to yield if no matches
                if (symbols[symbol]->bestBid >= symbols[symbol]->bestAsk) {
                    // Cache symbol data and queues references
                    Symbol* symbolData = symbols[symbol];
                    auto& buyQueues = symbolBuyQueues[symbol];
                    auto& sellQueues = symbolSellQueues[symbol];
                    
                    while (symbolData->bestBid >= symbolData->bestAsk) {
                        // Get best buy order
                        Order* bestBuyOrder = *buyQueues[symbolData->bestBid].getLeft();

                        // Get best sell order
                        Order* bestSellOrder = *sellQueues[symbolData->bestAsk].getLeft();

                        // If the buy quantity is greater than the sell quantity
                        if (bestBuyOrder->quantity > bestSellOrder->quantity) {
                            // Lower the quantity of the best buy order
                            bestBuyOrder->quantity -= bestSellOrder->quantity;

                            // Pop the best sell order
                            sellQueues[symbolData->bestAsk].popLeft();

                        // Else if the sell quantity is greater than the buy quantity
                        } else if (bestSellOrder->quantity > bestBuyOrder->quantity) {
                            // Lower the quantity of the best buy order
                            bestSellOrder->quantity -= bestBuyOrder->quantity;

                            // Pop the best sell order
                            buyQueues[symbolData->bestBid].popLeft();

                        // Else they are even
                        } else {
                            // Lower the quantity of the best buy order
                            bestBuyOrder->quantity -= bestSellOrder->quantity;

                            // Pop the best sell order
                            buyQueues[symbolData->bestBid].popLeft();
                        }

                        // If the buy price level is now empty remove it from active and set a new best bid
                        if (buyQueues[symbolData->bestBid].isEmpty()) {
                            // Pop it
                            symbolData->activeBuyPrices.pop();

                            // If it is now empty reset it
                            if (symbolData->activeBuyPrices.size() == 0) {
                                symbolData->bestBid = BEST_BID;

                            // Else set it to the next best
                            } else {
                                symbolData->bestBid = symbolData->activeBuyPrices.top();
                            }
                        } 

                        // If the sell price level is now empty remove it from active and set a new best ask
                        if (sellQueues[symbolData->bestAsk].isEmpty()) {
                            // Pop it
                            symbolData->activeSellPrices.pop();

                            // If it is now empty reset it
                            if (symbolData->activeSellPrices.size() == 0) {
                                symbolData->bestAsk = BEST_ASK;

                            // Else set it to the next best
                            } else {
                                symbolData->bestAsk = symbolData->activeSellPrices.top();
                            }
                        } 
                    }
                } else {
                    std::this_thread::yield();
                }
            }
        }

    public:
        // Constructor
        OrderBook() {
            // Make sure number of matching and working threads are valid
            static_assert(NUM_THREADS > maxSymbols, "Too many tickers for the number of threads");

            // Create pending pools
            pendingAddOrders = new MemoryPool<sizeof(Node<OrderParams>), maxOrders>();
            pendingRemovePool = new MemoryPool<sizeof(Node<int>), maxOrders>;

            // Launch working threads
            for (int i = 0; i < WORKERS; i++) {
                threads.emplace_back([&, i]{
                    workerThread(i);
                })
            }

            // Start threads
            running.store(true);
        }

        // Destructor
        ~OrderBook() {
            // Stop threads
            running.store(false);

            // Collect threads
            for (auto& thread : threads) {
                thread.join();
            }
        }

        // Add order using IDs and enums directly
        // Some way to get the order ID back to the user should probably be added
        void addOrder(int userID, SymbolID symbolID, Side side, int quantity, double price) {
            // Basic validation
            if (quantity <= 0 || price <= 0.0) {
                return;
            }
            
            // Push add order
            pendingAddOrders.pushRight(OrderParams(userID, symbolID, side, quantity, price), pendingAddPool);
        }

        // Remove an order given the ID
        void removeOrder(int orderID) {
            pendingRemoveOrders.pushRight(orderID, pendingRemovePool);
        }
    };

#endif