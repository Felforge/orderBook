#ifndef PARALLELORDERBOOK_H
#define PARALLELORDERBOOK_H

// Imports
#include <atomic>
#include <thread>
#include <unordered_map>
#include <map>
#include <vector>
#include <utility>
#include <optional>
#include "../lockless/queue/queue.h"

// Define SPIN_PAUSE based on architecture
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    #include <immintrin.h>
    #define SPIN_PAUSE() _mm_pause()
#else
    #define SPIN_PAUSE() std::atomic_signal_fence(std::memory_order_seq_cst)
#endif

// Configuaration Constants
constexpr size_t DEFAULT_RING_SIZE = 1 << 20;  // Roughly 1M slots for hot symbols
constexpr size_t PRICE_TABLE_BUCKETS = 16384; // Roughly 16k Available Price Levels
constexpr uint64_t TICK_PRECISION = 10000;     // 1e-4 precision (0.0001)

// Could be needed later for alignment
// constexpr size_t CACHE_LINE_SIZE = 64;

// Order side enum
enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};

// Order types
enum class OrderType : uint8_t {
    ADD = 0,
    CANCEL = 1
};

// Convert price to integer ticks to avoid failiures do to float precision
// Inline is needed for those granular optimizations
inline uint64_t priceToTicks(double price) {
    return static_cast<uint64_t>(price * TICK_PRECISION);
}

// Convert back to float price
inline double ticksToPrice(uint64_t ticks) {
    return static_cast<double>(ticks) / TICK_PRECISION;
}

// Forward declarations
template<size_t RingSize, size_t NumBuckets>
struct Symbol;

// Order struct
// Alligned to CACHE_LINE_SIZE 
template<size_t RingSize, size_t NumBuckets>
struct Order {
    // Memory block where this order is allocated
    void* memoryBlock;

    // Order ID
    uint64_t orderID;

    // ID of the user that placed the Order
    uint32_t userID;

    // Quantity (remaining) of order
    uint32_t quantity;

    // Order price in integer ticks
    uint64_t priceTicks;

    // Buy/Sell
    Side side;

    // Add/Cancel
    OrderType type;

    // Symbol ID
    uint16_t symbolID;

    // Generic Symbol Pointer
    Symbol<RingSize, NumBuckets>* symbol;

    // Store the node it is kept in
    Node<Order*>* node;

    // Create orderID from symbol and sequence
    static uint64_t createOrderID(uint16_t symbolID, uint64_t localSeq) {
        return (static_cast<uint64_t>(symbolID) << 48) | localSeq;
    }

    // Default constructor
    Order() = default;

    // Regular constructor
    Order(void* memoryBlock, uint64_t orderID, uint32_t userID, Side side, uint16_t symbolID, 
        Symbol<RingSize, NumBuckets>* symbol, uint32_t quantity, uint64_t price, OrderType type)
        : memoryBlock(memoryBlock), orderID(orderID), userID(userID), quantity(quantity), 
        priceTicks(price), side(side), type(type), symbolID(symbolID), symbol(symbol), node(nullptr) {}
};

// Price Level struct
// Alligned to CACHE_LINE_SIZE 
template<size_t RingSize, size_t NumBuckets>
struct PriceLevel {
    // Memory block where this price level is allocated
    void* memoryBlock;

    // Price in ticks
    uint64_t priceTicks;

    // Per price order queue
    LocklessQueue<Order<RingSize, NumBuckets>*>* queue;

    // Current number of orders held
    std::atomic<uint32_t> numOrders;

    // Concstructor
    PriceLevel(void* memoryBlock, uint64_t priceTicks, LocklessQueue<Order<RingSize, NumBuckets>*>* queue)
        : memoryBlock(memoryBlock), priceTicks(priceTicks), queue(queue), numOrders(0) {}
};

// Per-symbol publish ring (lock-free ring buffer)
// A ring is used to allow slots to be more easily reused, to easier track slots, and for backpressure detection
template<size_t RingSize, size_t NumBuckets>
class PublishRing {
    private:
        // Capacity must also be a power of 2 for bitmasking (cheaper than mod)
        static_assert((RingSize & (RingSize - 1)) == 0, "RingSize must be power of 2");

        // Struct for a solt in the ring
        struct Slot {
            std::atomic<Order<RingSize, NumBuckets>*> order{nullptr};
        };

        // Used to track the next slot where producers will write
        std::atomic<uint64_t> publishSeq{0};

        // Tracks the next slot the worker should pull from
        std::atomic<uint64_t> workSeq{0};

        // The actual ring buffer array
        // Indexed by seq & (RingSize - 1) for wraparound
        Slot ring[RingSize];

        // Function to back off for unsuccessful operations
        // The number of spins could be messed with
        void spinBackoff(int spinCount = 1) {
            for (int i = 0; i < spinCount; ++i) {
                // Run spin pause
                SPIN_PAUSE();
            }
        }

    public:
        // Producer function: acquire sequence and publish order
        void publish(Order<RingSize, NumBuckets>* order) {
            // Increment and retrieve sequence
            uint64_t seq = publishSeq.fetch_add(1);

            // Get ring index based on sequence
            size_t index = seq & (RingSize - 1);

            // Loop to update ring with strong CAS
            Order<RingSize, NumBuckets>* expected = nullptr;
            while (!ring[index].order.compare_exchange_strong(expected, order)) {
                spinBackoff();
            }
        }

        // Worker function to grab the next available order
        Order<RingSize, NumBuckets>* pullNextOrder() {
            // Get the next sequence
            uint64_t seq = workSeq.fetch_add(1);

            // Check if work is available
            if (seq >= publishSeq.load()) {
                return nullptr;
            }

            // Get ring index based on sequence
            size_t index = seq & (RingSize - 1);

            // Retrieve order at the sequence
            Order<RingSize, NumBuckets>* order = ring[index].order.load();

            // if the order exists
            if (order) {
                // Clear the slot
                ring[index].order.store(nullptr);
            }

            // Return the order
            return order;
        }
};

// Fixed-size lockless hash table
// Opened price levels are never cleared
// In a real system they'd be cleared after a long period of inactivity
template<size_t RingSize, size_t NumBuckets>
class PriceTable {
    private:
        // Capacity must also be a power of 2 for bitmasking (cheaper than mod)
        static_assert((NumBuckets & (NumBuckets - 1)) == 0, "NumBuckets must be power of 2");

        // Struct Object for a Price Bucket
        struct Bucket {
            std::atomic<PriceLevel<RingSize, NumBuckets>*> level{nullptr};
        };

        // Fixed size array of buckets
        Bucket buckets[NumBuckets];

        // Simple has function, could be imrpoved later
        size_t hash(uint64_t priceTicks) {
            return priceTicks & (NumBuckets - 1);
        }

    public:
        // Install price level using strong CAS
        // Returns true or false depending on the success of the operation
        bool installPriceLevel(PriceLevel<RingSize, NumBuckets>* level) {
            // Retrieve index from hash function
            size_t index = hash(level->priceTicks);

            // Check all buckets
            // Linear probing is used to check consecutive buckets
            // O(NumBuckets) Worst Case, O(1) average
            for (size_t i = 0; i < NumBuckets; i++) {
                // Declare expected value
                PriceLevel<RingSize, NumBuckets>* expected = nullptr;

                // Attempt strong CAS to insert level
                // Will succeed if the value matched the expected value
                if (buckets[index].level.compare_exchange_strong(expected, level)) {
                    // Operation succeeded, return true
                    return true;
                }

                // Failed, check if price level already exists, if so return false
                // If failed expected was previously updated to the real value
                if (expected->priceTicks == level->priceTicks) {
                    return false;
                }

                // Update the index using linear probing
                index = hash(index + 1);
            }

            // All buckets were full, return false
            return false;
        }

        // Lookup price level
        PriceLevel<RingSize, NumBuckets>* lookup(uint64_t priceTicks) {
            // Retrieve index from hash function
            size_t index = hash(priceTicks);

            // Check all buckets
            // Linear probing is used to check consecutive buckets
            // O(NumBuckets) Worst Case, O(1) average
            for (size_t i = 0; i < NumBuckets; i++) {
                // Retrieve expected pointer
                PriceLevel<RingSize, NumBuckets>* level = buckets[index].level.load();

                // If level is nullptr (not found) or the level is found return it
                if (!level || level->priceTicks == priceTicks) {
                    return level;
                }

                // Update the index using linear probing
                index = hash(index + 1);
            }

            // Price level was not found
            return nullptr;
        }

        // Checks if a price level exists and if it has orders within it
        // Return true if both conditions are true
        bool isActive(uint64_t priceTicks) {
            // Retrieve price level
            PriceLevel<RingSize, NumBuckets>* level = lookup(priceTicks);

            // Price level was not found, return false
            if (!level) {
                return false;
            }

            // Make sure it has orders
            return level->numOrders.load() > 0;
        }
};

// Struct of required memory pools
template<size_t MaxOrders, size_t RingSize, size_t NumBuckets>
struct Pools {
    MemoryPool<sizeof(Order<RingSize, NumBuckets>), MaxOrders> orderPool;
    MemoryPool<sizeof(Node<Order<RingSize, NumBuckets>*>), MaxOrders> nodePool;
    MemoryPool<sizeof(PriceLevel<RingSize, NumBuckets>), NumBuckets> priceLevelPool;
    MemoryPool<sizeof(LocklessQueue<Order<RingSize, NumBuckets>*>), NumBuckets> queuePool;
};

// Thread local declaration of Pools
template<size_t MaxOrders, size_t RingSize, size_t NumBuckets>
thread_local Pools<MaxOrders, RingSize, NumBuckets> myPools;

// Symbol Struct
template<size_t RingSize, size_t NumBuckets>
struct Symbol {
    // Memory on which this struct is allocated
    void* memoryBlock;

    // ID of this symbol
    uint16_t symbolID;

    // Name of this symbol
    std::string symbolName;

    // Symbol price tables
    PriceTable<RingSize, NumBuckets> buyPrices;
    PriceTable<RingSize, NumBuckets> sellPrices;

    // Best bid/ask tracking
    std::atomic<uint64_t> bestBidTicks{0};
    std::atomic<uint64_t> bestAskTicks{UINT64_MAX};

    // Constructor
    Symbol(void* memoryBlock, uint16_t symbolID, std::string symbolName) :
        memoryBlock(memoryBlock), symbolID(symbolID), symbolName(symbolName) {}
};

// Struct for worker thread
template<size_t MaxOrders, size_t RingSize, size_t NumBuckets>
class Worker {
    private:
        // ID of current thread
        uint16_t workerID;

        // Bool to control the thread
        std::atomic<bool>* running;

        // Function to process order
        void processOrder(Order<RingSize, NumBuckets>* order) {
            switch (order->type) {
                case OrderType::ADD:
                    insertOrder(order);
                    break;
                case OrderType::CANCEL:
                    cancelOrder(order);
                    break;
            }
        }

        // Helper function to check if an order can be matched
        bool canMatch(uint64_t oppTicks, Order<RingSize, NumBuckets>* order) {
            // Base case, nothing to be matched
            if (oppTicks == UINT64_MAX || oppTicks == 0) {
                return false;
            }

            // Buy case
            if (order->side == Side::BUY) {
                return order->priceTicks >= oppTicks;
            }

            // Else it is the sell case
            return order->priceTicks <= oppTicks;
        }

        // New best price level can be found in O(N) going backwards
        // This can be done because the shfits should be so small
        void matchOrder(Order<RingSize, NumBuckets>* order) {
            
        }

        // Function to insert order
        void insertOrder(Order<RingSize, NumBuckets>* order) {
            // Retrieve symbol pointer
            Symbol<RingSize, NumBuckets>* symbol = order->symbol; 

            // Try to match the order first
            matchOrder(order);

            // If order still has quantity, insert into book
            if (order->quantity > 0) {
                // Get or create price level
                PriceLevel<RingSize, NumBuckets>* level = getOrCreatePriceLevel(symbol, order->priceTicks, order->side);

                // Price level could not be created, delete the order
                if (!level) {
                    myPools<MaxOrders, RingSize, NumBuckets>.orderPool.deallocate(order->memoryBlock);
                    return;
                }

                // Insert into price level queue
                Node<Order<RingSize, NumBuckets>*>* node = level->queue->pushRight(order, &myPools<MaxOrders, RingSize, NumBuckets>.nodePool);

                // Store node pointer in order
                order->node = node;

                // Ammend order type to cancel so the user is able to cancel it
                order->type = OrderType::CANCEL;

                // If node was successfully created increment numOrders
                if (node) {
                    level->numOrders.fetch_add(1);
                }

                // Update best prices
                updateBestPrices(symbol, order->priceTicks, order->side);
            } else {
                // Order fully matched, deallocate
                myPools<MaxOrders, RingSize, NumBuckets>.orderPool.deallocate(order->memoryBlock);
            }
        }

        // Function to cancel a given order node
        void cancelOrder(Order<RingSize, NumBuckets>* order) {
            // Retrieve symbol pointer
            Symbol<RingSize, NumBuckets>* symbol = order->symbol; 

            // Retrieve node from order
            Node<Order<RingSize, NumBuckets>*>* node = order->node;

            // Get price level
            PriceLevel<RingSize, NumBuckets>* level = getOrCreatePriceLevel(symbol, order->priceTicks, order->side);

            // Price level should exist, if not delete order and return
            if (!level) {
                myPools<MaxOrders, RingSize, NumBuckets>.orderPool.deallocate(order->memoryBlock);
                return;
            }

            // Remove order from level
            level->queue->removeNode(node);

            // Decrease order count in level
            level->numOrders.fetch_sub(order->quantity);

            // Delete order
            myPools<MaxOrders, RingSize, NumBuckets>.orderPool.deallocate(order->memoryBlock);
        }

        PriceLevel<RingSize, NumBuckets>* getOrCreatePriceLevel(Symbol<RingSize, NumBuckets>* symbol, uint64_t priceTicks, Side side) {
            // Retrieve table based on side
            PriceTable<RingSize, NumBuckets>& table = (side == Side::BUY) ? 
                symbol->buyPrices : symbol->sellPrices;

            // Attempt to retrieve price level
            PriceLevel<RingSize, NumBuckets>* level = table.lookup(priceTicks);

            // If the price level exists return it
            if (level) {
                return level;
            }

            // Else, create new price level

            // Allocater for price level
            void* levelBlock = myPools<MaxOrders, RingSize, NumBuckets>.priceLevelPool.allocate();

            // Could not allocate, return
            if (!levelBlock) {
                return nullptr;
            }

            // Allocate for queue
            void* queueBlock = myPools<MaxOrders, RingSize, NumBuckets>.queuePool.allocate();

            // Could not allocate, return
            if (!queueBlock) {
                return nullptr;
            }

            // Create queue and price level
            LocklessQueue<Order<RingSize, NumBuckets>*>* queue = new (queueBlock) LocklessQueue<Order<RingSize, NumBuckets>*>();
            level = new (levelBlock) PriceLevel<RingSize, NumBuckets>(levelBlock, priceTicks, queue);

            // Price level has been concurrently created elsewhere
            if (!table.installPriceLevel(level)) {
                myPools<MaxOrders, RingSize, NumBuckets>.queuePool.deallocate(queueBlock);
                myPools<MaxOrders, RingSize, NumBuckets>.priceLevelPool.deallocate(levelBlock);
                return table.lookup(priceTicks);
            }

            // Return completed price level
            return level;
        }

        // Backtrack price level till a new best active one is found
        // If the price level no longer equal prev we can assume it was already udpated by someone else and exit
        void backtrackPriceLevel(Symbol<RingSize, NumBuckets>* symbol, Side side, uint64_t prev) {
            // Direction will be different depending on the side so that must be seperated
            // It can deifnietly be combined but the code would be impossible to read
            if (side == Side::BUY) {
                // Loop through possibel levels
                for (uint64_t i = prev - 1; i > 0; i--) {
                    // If price is no longer equal to prev or prev is again active we can return
                    if (symbol->bestBidTicks.load() != prev || symbol->buyPrices.isActive(prev)) {
                        return;
                    }

                    // Else, attempt CAS if price level is active
                    if (symbol->buyPrices.isActive(i)) {
                        // We can return after attempting the CAS once as it will only fail if bestBidTicks is no longer prev
                        symbol->bestBidTicks.compare_exchange_strong(prev, i);
                        return;
                    }
                }

                // Nothing is found, attempt to CAS reset the price level
                symbol->bestBidTicks.compare_exchange_strong(prev, 0);
            } else { // side == Side::SELL
                // Loop through possibel levels
                for (uint64_t i = prev + 1; i < UINT64_MAX; i++) {
                    // If price is no longer equal to prev or prev is again active we can return
                    if (symbol->bestAskTicks.load() != prev || symbol->sellPrices.isActive(prev)) {
                        return;
                    }

                    // Else, attempt CAS if price level is active
                    if (symbol->sellPrices.isActive(i)) {
                        // We can return after attempting the CAS once as it will only fail if bestAskTicks is no longer prev
                        symbol->bestAskTicks.compare_exchange_strong(prev, i);
                        return;
                    }
                }

                // Nothing is found, attempt to CAS reset the price level
                symbol->bestAskTicks.compare_exchange_strong(prev, UINT64_MAX);
            }
        } 

        // Function to update best bid and ask prices
        void updateBestPrices(Symbol<RingSize, NumBuckets>* symbol, uint64_t priceTicks, Side side) {
            if (side == Side::BUY) {
                // Start retry loop
                while (running->load()) {
                    // Retrieve current best price level
                    uint64_t current = symbol->bestBidTicks.load();

                    // If priceTicks is less than or equal to current or CAS succeeds return
                    if (priceTicks <= current || symbol->bestBidTicks.compare_exchange_strong(current, priceTicks)) {
                        return;
                    }
                }
            } else {
                // Start retry loop
                while (running->load()) {
                    // Retrieve current best price level
                    uint64_t current = symbol->bestAskTicks.load();

                    // If priceTicks is greater than or equal to current or CAS succeeds return
                    if (priceTicks >= current || symbol->bestBidTicks.compare_exchange_strong(current, priceTicks)) {
                        return;
                    }
                }
            }
        }

    public:
        // Hold memory block on which this item is allocated
        void* memoryBlock;

        // Constructor
        Worker(void* block, uint16_t workerID, std::atomic<bool>* running)
            : memoryBlock(block), workerID(workerID), running(running) {}

        // Function to run the worker
        void run(PublishRing<RingSize, NumBuckets>* publishRing) {
            while (running->load()) {
                // Pull next available order
                Order<RingSize, NumBuckets>* order = publishRing->pullNextOrder();

                // If order is valid process it, else yield
                if (order) {
                    processOrder(order);
                } else {
                    std::this_thread::yield();
                }
            }
        }
};

// Worker Pool Management Struct
template<size_t NumWorkers, size_t MaxOrders, size_t RingSize, size_t NumBuckets>
class WorkerPool {
    private:
        // Create pointer to hold memory pool for worker allocation
        // This is done on the host
        MemoryPool<sizeof(Worker<MaxOrders, RingSize, NumBuckets>), NumWorkers>* alocPool;

        // Hold all workers with capacity numWorkers
        std::vector<Worker<MaxOrders, RingSize, NumBuckets>> workers;

        // Hold all threads
        std::vector<std::thread> workerThreads;

        // To control threads
        std::atomic<bool> running{false};

        // Pointer to publish ring
        PublishRing<RingSize, NumBuckets>* publishRing;

    public:
        // Default constructor
        WorkerPool() = default;

        // Pass generic pool so that capacity does not need to be specified
        WorkerPool(MemoryPool<sizeof(Worker<MaxOrders, RingSize, NumBuckets>), NumWorkers>* pool, PublishRing<RingSize, NumBuckets>* publishRingPtr) {
            alocPool = pool;
            publishRing = publishRingPtr;
        }

        // Make destructor call stop
        ~WorkerPool() {
            stopWorkers();
        }

        // Function to start all worker threads
        void startWorkers() {
            // Toggle running on
            running.store(true);

            // Create and start worker threads
            for (uint16_t i = 0; i < NumWorkers; i++) {
                // Allocate from pool
                void* block = alocPool->allocate();

                // Create worker
                workers.emplace_back(block, i, &running);

                // Launch thread
                workerThreads.emplace_back([this, i]() {
                    workers[i].run(publishRing);
                });
            }
        }

        // Function to stop all worker threads
        void stopWorkers() {
            // Set running flag to false
            running.store(false);

            // Wait for all worker threads to finish
            for (auto& thread : workerThreads) {
                thread.join();
            }

            // Clear worker thread vector
            workerThreads.clear();

            // Deallocate all workers
            for (auto& worker : workers) {
                alocPool->deallocate(worker.memoryBlock);
            }
            workers.clear();
        }
};

// Main Order Book Struct
template<size_t NumWorkers, size_t MaxSymbols, size_t MaxOrders, size_t RingSize = DEFAULT_RING_SIZE, size_t NumBuckets = PRICE_TABLE_BUCKETS>
class OrderBook {
    private:
        // Symbol Management
        std::unordered_map<std::string, uint16_t> symbolNameToID;
        std::unordered_map<uint16_t, Symbol<RingSize, NumBuckets>*> symbols;
        std::atomic<uint16_t> nextSymbolID{0};

        // Universal Publish Ring
        PublishRing<RingSize, NumBuckets> publishRing;

        // Global memory pool for symbol data
        MemoryPool<sizeof(Symbol<RingSize, NumBuckets>), MaxSymbols> symbolPool;

        // Global Worker Memory Pool
        MemoryPool<sizeof(Worker<MaxOrders, RingSize, NumBuckets>), NumWorkers> workerMemPool;

        // Worker Thread Pool
        WorkerPool<NumWorkers, MaxOrders, RingSize, NumBuckets> workerPool;

        // Thread local sequence (counter)
        // Used for order ID generation
        // static required because thread_local cannot be a non-static class member
        static thread_local std::atomic<uint64_t> threadLocalSeq;

    public:
        // Constructor
        OrderBook() {
            // Make sure max symbols is valid
            static_assert(MaxSymbols <= UINT16_MAX, "MaxSymbols exceeds uint16_t range");

            // Create worker pool
            workerPool = WorkerPool<NumWorkers, MaxOrders, RingSize, NumBuckets>(&workerMemPool, &publishRing);
        }

        // Order Book Destructor
        ~OrderBook() {
            shutdown();
        }

        // Start the Order Book's system
        void start() {
            workerPool.startWorkers();
        }

        // Shut down the Order Book's system
        void shutdown() {
            workerPool.stopWorkers();
        }

        // Function to register symbol
        uint16_t registerSymbol(std::string& symbolName) {
            // Max number of symbols exceeded, throw errror
            if (symbols.size() >= MaxSymbols) {
                throw std::runtime_error("Maximum symbols exceeded");
            }

            // Check if already registered
            auto it = symbolNameToID.find(symbolName);

            // If symbol exists return it
            if (it != symbolNameToID.end()) {
                return it->second;
            }

            // Increment symbol ID
            uint16_t symbolID = nextSymbolID.fetch_add(1);

            // Allocate memory for symbol
            void* block = symbolPool.allocate();
            
            // Make sure mmemory is valid
            if (!block) {
                throw std::runtime_error("Failed to allocate symbol memory");
            }
            
            // Create symbol pointer
            Symbol<RingSize, NumBuckets>* symbol = new (block) Symbol<RingSize, NumBuckets>(block, symbolID, symbolName);

            // Store in maps
            symbolNameToID[symbolName] = symbolID;
            symbols[symbolID] = symbol;

            // Return the symbol ID
            return symbolID;
        }

        // Submit a new order
        // Returns optional of pair of order ID and order pointer
        // Optional will have no value if order could not be submitted
        std::optional<std::pair<uint64_t, Order<RingSize, NumBuckets>*>> submitOrder(uint32_t userID, uint16_t symbolID, Side side, uint32_t quantity, double price) {
            // Try to retrieve symbol
            auto symbolIt = symbols.find(symbolID);

            // If symbol could not be found return
            if (symbolIt == symbols.end()) {
                return std::nullopt;
            }

            // Retrieve symbol out of iterator
            Symbol<RingSize, NumBuckets>* symbol = symbolIt->second;

            // Convert price to ticks
            uint64_t priceTicks = priceToTicks(price);

            // Generate unique order ID
            uint64_t localSeq = threadLocalSeq.fetch_add(1);
            uint64_t orderID = Order<RingSize, NumBuckets>::createOrderID(symbolID, localSeq);

            // Allocate memory for order
            void* orderBlock = myPools<MaxOrders, RingSize, NumBuckets>.orderPool.allocate();

            // Make sure memory is valid, if not return
            if (!orderBlock) {
                return std::nullopt;
            }

            // Create order
            Order<RingSize, NumBuckets>* order = new (orderBlock) Order<RingSize, NumBuckets>(orderBlock, orderID, userID, side, symbolID, symbol, quantity, priceTicks, OrderType::ADD);

            // Publish to universal ring
            publishRing.publish(order);

            // Return to the user the Order ID and Pointer
            // The Order Type is ammended to Cancel once added so the user can cancel it
            return std::make_pair(orderID, order);
        }

        // Cancel an order
        // Needs to take that order's pointer
        // Return true if the operation succeeds, otherwise return false
        // Type should have already been switched to Cancel, if not false will be returned
        bool cancelOrder(Order<RingSize, NumBuckets>* order) {
            // Make sure type is cancel, if not return false
            if (order->type != OrderType::CANCEL) {
                return false;
            }

            // Retrieve symbol ID
            uint16_t symbolID = order->symbolID;

            // Try to retrieve symbol
            auto symbolIt = symbols.find(symbolID);

            // If symbol could not be found return
            if (symbolIt == symbols.end()) {
                return false;
            }

            // Retrieve symbol out of iterator
            Symbol<RingSize, NumBuckets>* symbol = symbolIt->second;

            // Publish to universal ring
            publishRing.publish(order);

            // Return true
            return true;
        }
};

// To-Do:
// Implement matching engine
// Add client side handling for order pointers

#endif