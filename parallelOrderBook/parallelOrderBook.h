#ifndef PARALLELORDERBOOK_H
#define PARALLELORDERBOOK_H

// Imports
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

// Order struct
// Alligned to CACHE_LINE_SIZE 
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

    // Create orderID from symbol and sequence
    static uint64_t createOrderID(uint16_t symbolID, uint64_t localSeq) {
        return (static_cast<uint64_t>(symbolID) << 48) | localSeq;
    }

    // Default constructor
    Order() = default;

    // Regular constructor
    Order(void* memoryBlock, uint64_t orderID, uint32_t userID, Side side, 
        uint16_t symbolID, uint32_t quantity, uint64_t price, OrderType type)
        : memoryBlock(memoryBlock), orderID(orderID), userID(userID), quantity(quantity), 
        priceTicks(price), side(side), type(type), symbolID(symbolID) {}
};

// Price Level struct
// Alligned to CACHE_LINE_SIZE 
struct PriceLevel {
    // Memory block where this price level is allocated
    void* memoryBlock;

    // Price in ticks
    uint64_t priceTicks;

    // Per price order queue
    LocklessQueue<Order*>* queue;

    // Current number of orders held
    std::atomic<uint32_t> numOrders;

    // Concstructor
    PriceLevel(void* memoryBlock, uint64_t priceTicks, LocklessQueue<Order*>* queue)
        : memoryBlock(memoryBlock), priceTicks(priceTicks), queue(queue), numOrders(0) {}
};

// Per-symbol publish ring (lock-free ring buffer)
// A ring is used to allow slots to be more easily reused, to easier track slots, and for backpressure detection
template<size_t RingSize = DEFAULT_RING_SIZE>
class PublishRing {
    private:
        // Capacity must also be a power of 2 for bitmasking (cheaper than mod)
        static_assert((RingSize & (RingSize - 1)) == 0, "RingSize must be power of 2");

        // Struct for a solt in the ring
        struct Slot {
            std::atomic<Order*> order{nullptr};
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
        // Returns updated sequence number
        uint64_t publish(Order* order) {
            // Increment and retrieve sequence
            uint64_t seq = publishSeq.fetch_add(1);

            // Get ring index based on sequence
            size_t index = seq & (RingSize - 1);

            // Loop to update ring with strong CAS
            Order* expected = nullptr;
            while (!ring[index].order.compare_exchange_strong(expected, order)) {
                spinBackoff();
            }

            // Return the sequence number
            return seq;
        }

        // Get current publish sequence
        uint64_t getPublishSeq() const {
            return publishSeq.load();
        }

        // Worker function to grab the next available order
        Order* pullNextOrder() {
            // Get the next sequence
            uint64_t seq = workSeq.fetch_add(1);

            // Check if work is available
            if (seq >= publishSeq.load()) {
                return nullptr;
            }

            // Get ring index based on sequence
            size_t index = seq & (RingSize - 1);

            // Retrieve order at the sequence
            Order* order = ring[index].order.load();

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
template<size_t NumBuckets = PRICE_TABLE_BUCKETS>
class PriceTable {
    private:
        // Capacity must also be a power of 2 for bitmasking (cheaper than mod)
        static_assert((NumBuckets & (NumBuckets - 1)) == 0, "NumBuckets must be power of 2");

        // Struct Object for a Price Bucket
        struct Bucket {
            atomic<PriceLevel*> level{nullptr};
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
        bool installPriceLevel(PriceLevel* level) {
            // Retrieve index from hash function
            size_t index = hash(level->priceTicks);

            // Check all buckets
            // Linear probing is used to check consecutive buckets
            // O(NumBuckets) Worst Case, O(1) average
            for (size_t i = 0; i < numBuckets; i++) {
                // Declare expected value
                PriceLevel* expected = nullptr;

                // Attempt strong CAS to insert level
                // Will succeed if the value matched the expected value
                if (buckets[index].level.compare_exchange_strong(expected, level)) {
                    // Operation succeeded, return true
                    return true;
                }

                // Failed, check if price level already exists, if so return false
                // If failed expected was previously updated to the real value
                if (expected->priceTicks == level->priceTicks) {
                    return false
                }

                // Update the index using linear probing
                index = hash(index + 1);
            }

            // All buckets were full, return false
            return false;
        }

        // Lookup price level
        PriceLevel* lookup(uint64_t priceTicks) {
            // Retrieve index from hash function
            size_t index = hash(priceTicks);

            // Check all buckets
            // Linear probing is used to check consecutive buckets
            // O(NumBuckets) Worst Case, O(1) average
            for (size_t i = 0; i < numBuckets; i++) {
                // Retrieve expected pointer
                PriceLevel* level = buckets[index].level.load();

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
};

// Memory Allocation Struct
template<size_t MaxOrders>
struct ThreadLocalPools {
    MemoryPool<sizeof(Order), MaxOrders> orderPool;
    MemoryPool<sizeof(Node<Order*>), MaxOrders> nodePool;
};

#endif