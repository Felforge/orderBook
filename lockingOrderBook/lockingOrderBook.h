#ifndef LOCKINGORDERBOOK_H
#define LOCKINGORDERBOOK_H

// Imports
#include <mutex>
#include <thread>
#include <unordered_map>
#include <map>
#include <vector>
#include <queue>
#include <utility>
#include <optional>
#include <cmath>
#include <atomic>

// Configuration Constants
constexpr size_t DEFAULT_RING_SIZE = 1 << 20;  // ~1M slots
constexpr size_t PRICE_TABLE_BUCKETS = 16384; // Roughly 16k Available Price Levels
constexpr uint64_t TICK_PRECISION = 100;       // 1e-2 precision (0.01 / 1 cent)

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

// Convert price to integer ticks to avoid failures due to float precision
inline uint64_t priceToTicks(double price) {
    return static_cast<uint64_t>(round(price * TICK_PRECISION));
}

// Convert back to float price
inline double ticksToPrice(uint64_t ticks) {
    return static_cast<double>(ticks) / TICK_PRECISION;
}

// Forward declarations
template<size_t RingSize, size_t NumBuckets>
struct Symbol;

template<typename T>
struct Node;

// Generic Memory Pool Type
struct GenericMemoryPool {
    virtual void* allocate() = 0;
    virtual void deallocate(void* ptr) = 0;
    virtual ~GenericMemoryPool() = default;
};

// Simple locking memory pool (replaces lockless version)
// Uses std::mutex for thread-safe allocation/deallocation
template<size_t BlockSize, size_t NumBlocks>
class MemoryPool : public GenericMemoryPool {
    private:
        struct Block {
            char data[BlockSize];
            Block* next;
        };

        std::queue<void*> freeList;
        std::mutex poolMutex;
        Block* allBlocks[NumBlocks];

    public:
        MemoryPool() {
            for (size_t i = 0; i < NumBlocks; ++i) {
                Block* block = new Block();
                allBlocks[i] = block;
                freeList.push(block);
            }
        }

        ~MemoryPool() {
            for (size_t i = 0; i < NumBlocks; ++i) {
                delete allBlocks[i];
            }
        }

        void* allocate() override {
            std::lock_guard<std::mutex> lock(poolMutex);
            if (freeList.empty()) {
                throw std::bad_alloc();
            }
            void* block = freeList.front();
            freeList.pop();
            return block;
        }

        void deallocate(void* ptr) override {
            std::lock_guard<std::mutex> lock(poolMutex);
            freeList.push(ptr);
        }
};

// Node for doubly linked list (used in queue)
template<typename T>
struct Node {
    T data;
    Node* prev;
    Node* next;
    bool isDummy;
    void* memoryBlock;
    GenericMemoryPool* ownerPool;

    Node(GenericMemoryPool* ownerPool, void* memoryBlock)
        : data(), prev(nullptr), next(nullptr), isDummy(true),
          memoryBlock(memoryBlock), ownerPool(ownerPool) {}

    Node(GenericMemoryPool* ownerPool, void* memoryBlock, const T& val)
        : data(val), prev(nullptr), next(nullptr), isDummy(false),
          memoryBlock(memoryBlock), ownerPool(ownerPool) {}
};

// Mutex-protected queue (replaces LocklessQueue)
// Uses mutex to protect doubly-linked list operations
template<typename T>
class LockingQueue {
    private:
        Node<T>* head;
        Node<T>* tail;
        std::mutex queueMutex;

    public:
        LockingQueue() : head(nullptr), tail(nullptr) {}

        ~LockingQueue() {
            if (!head || !tail) return;

            Node<T>* curr = head->next;
            while (curr != tail) {
                Node<T>* next = curr->next;
                curr->~Node<T>();
                curr->ownerPool->deallocate(curr->memoryBlock);
                curr = next;
            }

            head->~Node<T>();
            head->ownerPool->deallocate(head->memoryBlock);
            tail->~Node<T>();
            tail->ownerPool->deallocate(tail->memoryBlock);
        }

        void initialize(GenericMemoryPool* nodePool) {
            void* headBlock = nodePool->allocate();
            void* tailBlock = nodePool->allocate();

            head = new (headBlock) Node<T>(nodePool, headBlock);
            tail = new (tailBlock) Node<T>(nodePool, tailBlock);

            head->next = tail;
            tail->prev = head;
        }

        Node<T>* pushRight(const T& value, GenericMemoryPool* nodePool) {
            std::lock_guard<std::mutex> lock(queueMutex);

            void* block = nodePool->allocate();
            if (!block) return nullptr;

            Node<T>* newNode = new (block) Node<T>(nodePool, block, value);

            Node<T>* prev = tail->prev;
            newNode->prev = prev;
            newNode->next = tail;
            prev->next = newNode;
            tail->prev = newNode;

            return newNode;
        }

        void pushLeft(const T& value, GenericMemoryPool* nodePool) {
            std::lock_guard<std::mutex> lock(queueMutex);

            void* block = nodePool->allocate();
            if (!block) return;

            Node<T>* newNode = new (block) Node<T>(nodePool, block, value);

            Node<T>* next = head->next;
            newNode->prev = head;
            newNode->next = next;
            head->next = newNode;
            next->prev = newNode;
        }

        std::optional<T> popLeft() {
            std::lock_guard<std::mutex> lock(queueMutex);

            Node<T>* node = head->next;
            if (node == tail) {
                return std::nullopt;
            }

            T data = node->data;
            head->next = node->next;
            node->next->prev = head;

            node->~Node<T>();
            node->ownerPool->deallocate(node->memoryBlock);

            return data;
        }

        void removeNode(Node<T>* node) {
            if (!node) return;

            std::lock_guard<std::mutex> lock(queueMutex);

            node->prev->next = node->next;
            node->next->prev = node->prev;

            node->~Node<T>();
            node->ownerPool->deallocate(node->memoryBlock);
        }
};

// Order struct
// Uses atomics for quantity and type to match test expectations
template<size_t RingSize, size_t NumBuckets>
struct Order {
    // Memory block where this order is allocated
    void* memoryBlock;
    uint64_t orderID;
    uint32_t userID;
    std::atomic<uint32_t> quantity;
    uint64_t priceTicks;
    Side side;
    std::atomic<OrderType> type;
    uint16_t symbolID;
    Symbol<RingSize, NumBuckets>* symbol;
    Node<Order*>* node;
    GenericMemoryPool* ownerPool;

    static uint64_t createOrderID(uint16_t symbolID, uint64_t localSeq) {
        return (static_cast<uint64_t>(symbolID) << 48) | localSeq;
    }

    Order() = default;

    Order(void* memoryBlock, GenericMemoryPool* ownerPool, uint64_t orderID,
          uint32_t userID, Side side, uint16_t symbolID, Symbol<RingSize, NumBuckets>* symbol,
          uint32_t quantity, uint64_t price, OrderType type)
        : memoryBlock(memoryBlock), orderID(orderID), userID(userID), quantity(quantity),
          priceTicks(price), side(side), type(type), symbolID(symbolID), symbol(symbol),
          node(nullptr), ownerPool(ownerPool) {}
};

// Price Level struct
template<size_t RingSize, size_t NumBuckets>
struct PriceLevel {
    void* memoryBlock;
    uint64_t priceTicks;
    LockingQueue<Order<RingSize, NumBuckets>*>* queue;
    void* queueBlock;
    std::atomic<uint32_t> numOrders;
    GenericMemoryPool* ownerPool;
    GenericMemoryPool* queuePool;

    PriceLevel(void* memoryBlock, uint64_t priceTicks,
               LockingQueue<Order<RingSize, NumBuckets>*>* queue, void* queueBlock,
               GenericMemoryPool* ownerPool, GenericMemoryPool* queuePool)
        : memoryBlock(memoryBlock), priceTicks(priceTicks), queue(queue),
          queueBlock(queueBlock), numOrders(0), ownerPool(ownerPool), queuePool(queuePool) {}

    ~PriceLevel() {
        queue->~LockingQueue();
        queuePool->deallocate(queueBlock);
    }
};

// Mutex-protected publish ring (replaces lock-free ring buffer)
// A ring is used to allow slots to be more easily reused and for backpressure detection
template<size_t RingSize, size_t NumBuckets>
class PublishRing {
    private:
        static_assert((RingSize & (RingSize - 1)) == 0, "RingSize must be power of 2");

        struct Slot {
            Order<RingSize, NumBuckets>* order;
        };

        uint64_t publishSeq;
        uint64_t workSeq;
        Slot ring[RingSize];
        std::mutex ringMutex;

    public:
        PublishRing() : publishSeq(0), workSeq(0) {
            for (size_t i = 0; i < RingSize; ++i) {
                ring[i].order = nullptr;
            }
        }

        void publish(Order<RingSize, NumBuckets>* order) {
            std::lock_guard<std::mutex> lock(ringMutex);
            size_t index = publishSeq & (RingSize - 1);
            ring[index].order = order;
            publishSeq++;
        }

        Order<RingSize, NumBuckets>* pullNextOrder() {
            std::lock_guard<std::mutex> lock(ringMutex);

            if (workSeq >= publishSeq) {
                return nullptr;
            }

            size_t index = workSeq & (RingSize - 1);
            Order<RingSize, NumBuckets>* order = ring[index].order;
            ring[index].order = nullptr;
            workSeq++;

            return order;
        }

        // Check if ring is empty (workers caught up)
        bool isEmpty() const {
            std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(ringMutex));
            return workSeq >= publishSeq;
        }
};

// Mutex-protected hash table for price levels
// Uses linear probing for collision resolution
template<size_t RingSize, size_t NumBuckets>
class PriceTable {
    private:
        static_assert((NumBuckets & (NumBuckets - 1)) == 0, "NumBuckets must be power of 2");

        struct Bucket {
            PriceLevel<RingSize, NumBuckets>* level;
        };

        Bucket buckets[NumBuckets];
        mutable std::mutex tableMutex;

        size_t hash(uint64_t priceTicks) {
            return priceTicks & (NumBuckets - 1);
        }

    public:
        PriceTable() {
            for (size_t i = 0; i < NumBuckets; ++i) {
                buckets[i].level = nullptr;
            }
        }

        ~PriceTable() {
            cleanup();
        }

        bool installPriceLevel(PriceLevel<RingSize, NumBuckets>* level) {
            std::lock_guard<std::mutex> lock(tableMutex);

            size_t index = hash(level->priceTicks);

            for (size_t i = 0; i < NumBuckets; i++) {
                if (buckets[index].level == nullptr) {
                    buckets[index].level = level;
                    return true;
                }

                if (buckets[index].level->priceTicks == level->priceTicks) {
                    return false;
                }

                index = (index + 1) & (NumBuckets - 1);
            }

            return false;
        }

        PriceLevel<RingSize, NumBuckets>* lookup(uint64_t priceTicks) {
            std::lock_guard<std::mutex> lock(tableMutex);

            size_t index = hash(priceTicks);

            for (size_t i = 0; i < NumBuckets; i++) {
                PriceLevel<RingSize, NumBuckets>* level = buckets[index].level;

                if (!level || level->priceTicks == priceTicks) {
                    return level;
                }

                index = (index + 1) & (NumBuckets - 1);
            }

            return nullptr;
        }

        bool isActive(uint64_t priceTicks) {
            PriceLevel<RingSize, NumBuckets>* level = lookup(priceTicks);

            if (!level) {
                return false;
            }

            return level->numOrders.load() > 0;
        }

        void cleanup() {
            std::lock_guard<std::mutex> lock(tableMutex);

            for (size_t i = 0; i < NumBuckets; i++) {
                PriceLevel<RingSize, NumBuckets>* level = buckets[i].level;
                if (level) {
                    GenericMemoryPool* ownerPool = level->ownerPool;
                    void* memoryBlock = level->memoryBlock;

                    level->~PriceLevel();
                    ownerPool->deallocate(memoryBlock);
                }
            }
        }
};

// Struct of required memory pools
template<size_t MaxOrders, size_t RingSize, size_t NumBuckets>
struct Pools {
    MemoryPool<sizeof(Order<RingSize, NumBuckets>), MaxOrders> orderPool;
    MemoryPool<sizeof(Node<Order<RingSize, NumBuckets>*>), MaxOrders> nodePool;
    MemoryPool<sizeof(PriceLevel<RingSize, NumBuckets>), NumBuckets> priceLevelPool;
    MemoryPool<sizeof(LockingQueue<Order<RingSize, NumBuckets>*>), NumBuckets> queuePool;
};

// Symbol Struct
template<size_t RingSize, size_t NumBuckets>
struct Symbol {
    void* memoryBlock;
    uint16_t symbolID;
    std::string symbolName;
    PriceTable<RingSize, NumBuckets> buyPrices;
    PriceTable<RingSize, NumBuckets> sellPrices;
    std::atomic<uint64_t> bestBidTicks;
    std::atomic<uint64_t> bestAskTicks;

    Symbol(void* memoryBlock, uint16_t symbolID, std::string symbolName)
        : memoryBlock(memoryBlock), symbolID(symbolID), symbolName(symbolName),
          bestBidTicks(0), bestAskTicks(UINT64_MAX) {}
};

// Worker thread class
// Each worker has its own memory pools to reduce contention
template<size_t MaxOrders, size_t RingSize, size_t NumBuckets>
class Worker {
    private:
        uint16_t workerID;
        std::atomic<bool>* running;
        Pools<MaxOrders, RingSize, NumBuckets> pools;

        void processOrder(Order<RingSize, NumBuckets>* order) {
            switch (order->type.load()) {
                case OrderType::ADD:
                    insertOrder(order);
                    break;
                case OrderType::CANCEL:
                    cancelOrder(order);
                    break;
            }
        }

        bool canMatch(uint64_t oppTicks, Order<RingSize, NumBuckets>* order) {
            if (oppTicks == UINT64_MAX || oppTicks == 0) {
                return false;
            }

            if (order->side == Side::BUY) {
                return order->priceTicks >= oppTicks;
            }

            return order->priceTicks <= oppTicks;
        }

        void matchAtPriceLevel(Order<RingSize, NumBuckets>* order, PriceLevel<RingSize, NumBuckets>* level) {
            while (order->quantity.load() > 0 && level->numOrders.load() > 0) {
                auto optMatch = level->queue->popLeft();

                if (!optMatch.has_value()) {
                    break;
                }

                Order<RingSize, NumBuckets>* match = optMatch.value();

                if (order->quantity.load() >= match->quantity.load()) {
                    uint32_t matchQuant = match->quantity.load();
                    order->quantity.fetch_sub(matchQuant);
                    level->numOrders.fetch_sub(1);
                    match->ownerPool->deallocate(match->memoryBlock);
                } else {
                    uint32_t matchQuant = order->quantity.load();
                    match->quantity.fetch_sub(matchQuant);
                    order->quantity.store(0);
                    level->queue->pushLeft(match, &pools.nodePool);
                }
            }
        }

        void matchOrder(Order<RingSize, NumBuckets>* order) {
            Symbol<RingSize, NumBuckets>* symbol = order->symbol;
            Side opp = (order->side == Side::BUY) ? Side::SELL : Side::BUY;
            PriceTable<RingSize, NumBuckets>& oppTable = (opp == Side::BUY) ?
                symbol->buyPrices : symbol->sellPrices;

            while (order->quantity.load() > 0) {
                uint64_t bestMatch = (opp == Side::BUY) ?
                    symbol->bestBidTicks.load() : symbol->bestAskTicks.load();

                if (!canMatch(bestMatch, order)) {
                    return;
                }

                PriceLevel<RingSize, NumBuckets>* level = oppTable.lookup(bestMatch);

                if (!oppTable.isActive(bestMatch)) {
                    backtrackPriceLevel(symbol, opp, bestMatch);
                    continue;
                }

                matchAtPriceLevel(order, level);

                if (!oppTable.isActive(bestMatch)) {
                    backtrackPriceLevel(symbol, opp, bestMatch);
                }
            }
        }

        void insertOrder(Order<RingSize, NumBuckets>* order) {
            Symbol<RingSize, NumBuckets>* symbol = order->symbol;

            matchOrder(order);

            if (order->quantity.load() > 0) {
                PriceLevel<RingSize, NumBuckets>* level = getOrCreatePriceLevel(symbol, order->priceTicks, order->side);

                if (!level) {
                    order->ownerPool->deallocate(order->memoryBlock);
                    return;
                }

                Node<Order<RingSize, NumBuckets>*>* node = level->queue->pushRight(order, &pools.nodePool);
                order->node = node;
                order->type.store(OrderType::CANCEL);

                if (node) {
                    level->numOrders.fetch_add(1);
                }

                updateBestPrices(symbol, order->priceTicks, order->side);
            } else {
                order->ownerPool->deallocate(order->memoryBlock);
            }
        }

        void cancelOrder(Order<RingSize, NumBuckets>* order) {
            Symbol<RingSize, NumBuckets>* symbol = order->symbol;
            Node<Order<RingSize, NumBuckets>*>* node = order->node;

            PriceLevel<RingSize, NumBuckets>* level = getOrCreatePriceLevel(symbol, order->priceTicks, order->side);

            if (!level) {
                order->ownerPool->deallocate(order->memoryBlock);
                return;
            }

            level->numOrders.fetch_sub(1);
            level->queue->removeNode(node);
            order->ownerPool->deallocate(order->memoryBlock);
        }

        PriceLevel<RingSize, NumBuckets>* getOrCreatePriceLevel(Symbol<RingSize, NumBuckets>* symbol, uint64_t priceTicks, Side side) {
            PriceTable<RingSize, NumBuckets>& table = (side == Side::BUY) ?
                symbol->buyPrices : symbol->sellPrices;

            PriceLevel<RingSize, NumBuckets>* level = table.lookup(priceTicks);

            if (level) {
                return level;
            }

            void* levelBlock = pools.priceLevelPool.allocate();
            if (!levelBlock) {
                return nullptr;
            }

            void* queueBlock = pools.queuePool.allocate();
            if (!queueBlock) {
                pools.priceLevelPool.deallocate(levelBlock);
                return nullptr;
            }

            LockingQueue<Order<RingSize, NumBuckets>*>* queue =
                new (queueBlock) LockingQueue<Order<RingSize, NumBuckets>*>();
            queue->initialize(&pools.nodePool);

            level = new (levelBlock) PriceLevel<RingSize, NumBuckets>(
                levelBlock, priceTicks, queue, queueBlock, &pools.priceLevelPool, &pools.queuePool);

            if (!table.installPriceLevel(level)) {
                level->~PriceLevel();
                pools.priceLevelPool.deallocate(levelBlock);
                return table.lookup(priceTicks);
            }

            return level;
        }

        void backtrackPriceLevel(Symbol<RingSize, NumBuckets>* symbol, Side side, uint64_t prev) {
            if (side == Side::BUY) {
                for (uint64_t i = prev - 1; i >= prev - 25 && i < prev; i--) {
                    uint64_t current = symbol->bestBidTicks.load();
                    if (current != prev) {
                        // Someone else updated - check if new value is in range we already searched or is valid
                        if (current == 0 || current > i || symbol->buyPrices.isActive(current)) {
                            return;
                        }
                        // New value is below our search position, will be handled by our CAS or subsequent iterations
                    }
                    if (symbol->buyPrices.isActive(prev)) {
                        return;
                    }

                    if (symbol->buyPrices.isActive(i)) {
                        symbol->bestBidTicks.compare_exchange_strong(prev, i);
                        return;
                    }
                }

                symbol->bestBidTicks.compare_exchange_strong(prev, 0);
            } else {
                for (uint64_t i = prev + 1; i <= prev + 25; i++) {
                    uint64_t current = symbol->bestAskTicks.load();
                    if (current != prev) {
                        // Someone else updated - check if new value is in range we already searched or is valid
                        if (current == UINT64_MAX || current < i || symbol->sellPrices.isActive(current)) {
                            return;
                        }
                        // New value is above our search position, will be handled by our CAS or subsequent iterations
                    }
                    if (symbol->sellPrices.isActive(prev)) {
                        return;
                    }

                    if (symbol->sellPrices.isActive(i)) {
                        symbol->bestAskTicks.compare_exchange_strong(prev, i);
                        return;
                    }
                }

                symbol->bestAskTicks.compare_exchange_strong(prev, UINT64_MAX);
            }
        }

        void updateBestPrices(Symbol<RingSize, NumBuckets>* symbol, uint64_t priceTicks, Side side) {
            if (side == Side::BUY) {
                while (running->load()) {
                    uint64_t current = symbol->bestBidTicks.load();

                    if (priceTicks <= current || symbol->bestBidTicks.compare_exchange_strong(current, priceTicks)) {
                        return;
                    }
                }
            } else {
                while (running->load()) {
                    uint64_t current = symbol->bestAskTicks.load();

                    if (priceTicks >= current || symbol->bestAskTicks.compare_exchange_strong(current, priceTicks)) {
                        return;
                    }
                }
            }
        }

    public:
        void* memoryBlock;

        Worker() : memoryBlock(nullptr), workerID(0), running(nullptr) {}

        Worker(void* block, uint16_t workerID, std::atomic<bool>* running)
            : memoryBlock(block), workerID(workerID), running(running) {}

        ~Worker() {}

        void run(PublishRing<RingSize, NumBuckets>* publishRing) {
            while (running->load()) {
                Order<RingSize, NumBuckets>* order = publishRing->pullNextOrder();

                if (order) {
                    processOrder(order);
                } else {
                    std::this_thread::yield();
                }
            }
        }
};

// Worker Pool Management
// Manages lifecycle of worker threads
template<size_t NumWorkers, size_t MaxOrders, size_t RingSize, size_t NumBuckets>
class WorkerPool {
    private:
        MemoryPool<sizeof(Worker<MaxOrders, RingSize, NumBuckets>), NumWorkers>* allocPool;
        std::vector<Worker<MaxOrders, RingSize, NumBuckets>*> workers;
        std::vector<std::thread> workerThreads;
        std::atomic<bool> running{false};
        PublishRing<RingSize, NumBuckets>* publishRing;

    public:
        WorkerPool() = default;

        WorkerPool(MemoryPool<sizeof(Worker<MaxOrders, RingSize, NumBuckets>), NumWorkers>* pool,
                   PublishRing<RingSize, NumBuckets>* publishRingPtr)
            : allocPool(pool), publishRing(publishRingPtr) {}

        ~WorkerPool() {
            stopWorkers();
        }

        void startWorkers() {
            running.store(true);

            for (uint16_t i = 0; i < NumWorkers; i++) {
                void* block = allocPool->allocate();
                Worker<MaxOrders, RingSize, NumBuckets>* worker =
                    new (block) Worker<MaxOrders, RingSize, NumBuckets>(block, i, &running);
                workers.push_back(worker);
            }

            for (uint16_t i = 0; i < NumWorkers; i++) {
                workerThreads.emplace_back([this, i]() {
                    workers[i]->run(publishRing);
                });
            }
        }

        void stopWorkerThreads() {
            running.store(false);

            for (auto& thread : workerThreads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }

            workerThreads.clear();
        }

        void destroyWorkers() {
            for (auto* worker : workers) {
                worker->~Worker<MaxOrders, RingSize, NumBuckets>();
                allocPool->deallocate(worker->memoryBlock);
            }
            workers.clear();
        }

        void stopWorkers() {
            stopWorkerThreads();
            destroyWorkers();
        }
};

// Main Order Book
// Locking version for performance comparison with lockless implementation
template<size_t NumWorkers, size_t MaxSymbols, size_t MaxOrders,
         size_t RingSize = DEFAULT_RING_SIZE, size_t NumBuckets = PRICE_TABLE_BUCKETS>
class OrderBook {
    private:
        std::unordered_map<std::string, uint16_t> symbolNameToID;
        std::unordered_map<uint16_t, Symbol<RingSize, NumBuckets>*> symbols;
        std::atomic<uint16_t> nextSymbolID{0};

        PublishRing<RingSize, NumBuckets> publishRing;

        MemoryPool<sizeof(Symbol<RingSize, NumBuckets>), MaxSymbols> symbolPool;
        MemoryPool<sizeof(Order<RingSize, NumBuckets>), MaxOrders> orderPool;
        MemoryPool<sizeof(Worker<MaxOrders, RingSize, NumBuckets>), NumWorkers> workerMemPool;

        WorkerPool<NumWorkers, MaxOrders, RingSize, NumBuckets> workerPool;

        static thread_local std::atomic<uint64_t> threadLocalSeq;

    public:
        OrderBook() : workerPool(&workerMemPool, &publishRing) {
            static_assert(MaxSymbols <= UINT16_MAX, "MaxSymbols exceeds uint16_t range");
            threadLocalSeq.store(0);
        }

        ~OrderBook() {
            shutdown();
            nextSymbolID.store(0);
        }

        void start() {
            workerPool.startWorkers();
        }

        void shutdown() {
            workerPool.stopWorkerThreads();

            for (auto& [symbolID, symbol] : symbols) {
                symbol->~Symbol<RingSize, NumBuckets>();
                symbolPool.deallocate(symbol->memoryBlock);
            }
            symbols.clear();
            symbolNameToID.clear();

            workerPool.destroyWorkers();
        }

        uint16_t registerSymbol(std::string& symbolName) {
            if (symbols.size() >= MaxSymbols) {
                throw std::runtime_error("Maximum symbols exceeded");
            }

            auto it = symbolNameToID.find(symbolName);
            if (it != symbolNameToID.end()) {
                return it->second;
            }

            uint16_t symbolID = nextSymbolID.fetch_add(1);

            void* block = symbolPool.allocate();
            if (!block) {
                throw std::runtime_error("Failed to allocate symbol memory");
            }

            Symbol<RingSize, NumBuckets>* symbol =
                new (block) Symbol<RingSize, NumBuckets>(block, symbolID, symbolName);

            symbolNameToID[symbolName] = symbolID;
            symbols[symbolID] = symbol;

            return symbolID;
        }

        std::optional<std::pair<uint64_t, Order<RingSize, NumBuckets>*>>
        submitOrder(uint32_t userID, uint16_t symbolID, Side side, uint32_t quantity, double price) {
            auto symbolIt = symbols.find(symbolID);

            if (symbolIt == symbols.end() || price <= 0.0 || static_cast<int>(quantity) <= 0) {
                return std::nullopt;
            }

            Symbol<RingSize, NumBuckets>* symbol = symbolIt->second;
            uint64_t priceTicks = priceToTicks(price);

            uint64_t localSeq = threadLocalSeq.fetch_add(1);
            uint64_t orderID = Order<RingSize, NumBuckets>::createOrderID(symbolID, localSeq);

            void* orderBlock = orderPool.allocate();
            if (!orderBlock) {
                return std::nullopt;
            }

            Order<RingSize, NumBuckets>* order = new (orderBlock) Order<RingSize, NumBuckets>(
                orderBlock, &orderPool, orderID, userID, side, symbolID, symbol,
                quantity, priceTicks, OrderType::ADD);

            publishRing.publish(order);

            return std::make_pair(orderID, order);
        }

        bool cancelOrder(Order<RingSize, NumBuckets>* order) {
            if (!order || order->type.load() != OrderType::CANCEL) {
                return false;
            }

            uint16_t symbolID = order->symbolID;
            auto symbolIt = symbols.find(symbolID);

            if (symbolIt == symbols.end()) {
                return false;
            }

            publishRing.publish(order);
            return true;
        }

        // Check if all workers have caught up (ring is empty)
        // Small discrepancy: order may be pulled but not fully processed yet
        // Negligible with large batch sizes
        bool isIdle() const {
            return publishRing.isEmpty();
        }
};

// Define the static thread_local member
template<size_t NumWorkers, size_t MaxSymbols, size_t MaxOrders, size_t RingSize, size_t NumBuckets>
thread_local std::atomic<uint64_t> OrderBook<NumWorkers, MaxSymbols, MaxOrders, RingSize, NumBuckets>::threadLocalSeq{0};

#endif
