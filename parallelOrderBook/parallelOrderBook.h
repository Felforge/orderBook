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

// Max orders is given per thread
// This design uses a thread to match each symbol
// This would not scale to thousands of symbols
// For that many symbols a different approach shopuld be used
template<size_t maxTickers, size_t maxOrders>
class OrderBook {
    private:
        // Universal order ID
        std::atomic<int> orderID{0};

        // Hold matching threads
        // One thread per ticker
        std::thread matchingThreads[maxTickers];

        // Map to hold all deques
        std::unordered_map<SymbolID, LocklessQueue<Order*>> symbolQueues;
        
        // Symbol registration maps
        std::unordered_map<std::string, SymbolID> symbolNameToID;
        std::unordered_map<SymbolID, std::string> symbolIDToName;
        std::atomic<SymbolID> nextSymbolID{1};

        // Create flag to control worker threads
        std::atomic<bool> running{false};

        // Lockless queues of pending requests
        LocklessQueue<OrderParams> pendingAddOrders; // Type likely has to be changed
        LocklessQueue<int> pendingRemoveOrders;

        // Memory pool for pending order nodes
        // Number of blocks might need to be higher
        MemoryPool<sizeof(Node<OrderParams>), maxOrders> *pendingAddPool;
        MemoryPool<sizeof(Node<int>), maxOrders> *pendingRemovePool;

        // Vector to hold threads
        std::vector<std::thread> threads;

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

            // Add order to symbol queue
            symbolQueues[symbolID].pushRight(order, pools<maxOrders>.nodePool);
        }

        // Intake worker thread function
        void intakeWorker() {
            // WHile running flag is set
            while (running.load()) {
                // Attempt to pop from pendingAddOrders
                std::optional<OrderParams> params = pendingAddOrders.popLeft();

                // If params has a value intake it
                if (params.has_value()) {
                    // Get params out of optional wrapper
                    OrderParams paramsObj = *params;
                    intakeOrder(paramsObj.userID, paramsObj.symbolID, paramsObj.side, paramsObj.quantity, paramsObj.price);
                }

                // Attempt to pop from pendingRemoveOrders;
                std::optional<int> ID = pendingRemoveOrders.popLeft();

                // If ID had a value remove it
                if (ID.has_value()) {
                    // Top be implemented later
                }

                // Yield if nothing to do
                std::this_thread::yield();
            }
        }

    public:
        // Constructor
        OrderBook() {
            // Make sure number of matching and working threads are valid
            static_assert(NUM_THREADS > maxTickers, "Too many tickers for the number of threads");

            // Create pending pools
            pendingAddOrders = new MemoryPool<sizeof(Node<OrderParams>), maxOrders>();
            pendingRemovePool = new MemoryPool<sizeof(Node<int>), maxOrders>;

            // Launch threads
            for (int i = 0; i < NUM_THREADS; i++) {
                threads.emplace_back([&]{
                    intakeWorker();
                })
            }

            // Start threads
            running.store(true);
        }

        // Destructor
        ~OrderBook();

        // Register a symbol and return its fast ID
        SymbolID registerSymbol(const std::string& symbolName) {
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
            
            // Create symbol queue
            symbolQueues[newID] = LocklessQueue<Order*>();
            
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

        // Add order using string interface
        void addOrder(int userID, const std::string& ticker, const std::string& side, int quantity, double price) {
            // Convert to internal types
            auto symbolIt = symbolNameToID.find(ticker);
            if (symbolIt == symbolNameToID.end()) {
                cout << "Order Book Error: Ticker is Invalid - use registerSymbol() first" << endl;
                return;
            }
            
            Side sideEnum;
            try {
                sideEnum = parseSide(side);
            } catch (const std::invalid_argument&) {
                cout << "Order Book Error: Invalid Order Side" << endl;
                return;
            }
            
            // Call direct version
            addOrderFast(userID, symbolIt->second, sideEnum, quantity, price);
        }
        
        // Add order using IDs and enums directly
        void addOrderFast(int userID, SymbolID symbolID, Side side, int quantity, double price) {
            // Basic validation
            if (quantity <= 0 || price <= 0.0) {
                return;
            }
            
            // Push add order
            pendingAddOrders.pushRight(OrderParams(userID, symbolID, side, quantity, price), pendingAddPool);
        }
    };

#endif