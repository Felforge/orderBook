# Lockless Parallel Order Book

A high-performance, lock-free order book system built from scratch in C++20, designed to investigate whether lockless architectures can outperform traditional mutex-based designs in latency-sensitive financial applications.

---

## Demo

Full GTest suite passing under Normal, ASan, and TSan builds on a 32-thread remote server:

<video src="resources/Tests-Passing.mp4" controls width="100%"></video>

---

## Abstract

Order books underpin all modern markets, matching buy and sell orders to complete transactions. To facilitate high-frequency trading, squeezing maximum performance from these books is critical. Traditional parallel order books rely on mutex locks to protect shared data, but waiting destroys efficiency. This project explores whether a parallel order book built entirely around lock-free data structures can outperform an architecturally identical locking implementation. The lockless design centers on a lock-free deque based on Sundell and Tsigas (2008), extended with hazard pointers, per-thread memory pools, and a lock-free publish ring. Currently in final benchmarking across six contention scenarios on a 32-thread university server.

---

## Architecture

The system follows a **producer-consumer pattern** where a host thread publishes orders into a lock-free ring buffer, and a configurable pool of worker threads pull and process them.

```
  Producer Thread(s)                    Worker Threads (1–32)
       |                                       |
  submitOrder()  ───>  Publish Ring  ───>  processOrder()
  cancelOrder()     (lock-free CAS          /       \
                     ring buffer)     matchOrder()  insertOrder()
                                          |              |
                                     PriceTable     PriceTable
                                   (lock-free        (lock-free
                                    hash table)       hash table)
                                          |              |
                                     PriceLevel     PriceLevel
                                   (lock-free        (lock-free
                                      deque)           deque)
```

### Core Components

**Publish Ring** — A lock-free circular buffer that distributes orders from producers to workers. Uses `fetch_add` for multi-producer sequencing and CAS-based claiming on the consumer side. Cache-line aligned counters with explicit padding prevent false sharing. Built-in backpressure causes producers to spin-yield when the ring is full, naturally absorbing volume spikes.

**Price Table** — A lock-free open-addressing hash table with linear probing. Workers install new price levels atomically via `compare_exchange_strong`. All prices are converted to integer ticks (`price * 100`) to avoid floating-point comparison issues. Price levels are never removed during runtime — only during shutdown.

**Price-Level Deques** — Each price level holds a lock-free doubly-linked deque based on the Sundell and Tsigas algorithm. Supports O(1) push, pop, and arbitrary node removal. A deque is necessary because time-based ordering is critical in an order book, and the doubly-linked structure enables O(1) cancellations via direct pointer access — without traversal.

**Matching Engine** — Embedded into the order processing pipeline. When a new order arrives, workers check the opposing best price level. If prices cross, the engine pops orders from the opposing deque and adjusts quantities until the incoming order is filled or the level is exhausted.

**Best Price Tracking** — Adding a newly improved best price is trivial (CAS update). But when the current best level empties, the system backtracks up to 25 ticks to find the next active level. While seemingly non-optimal, tight spreads in high-frequency markets mean it almost never iterates more than a few times.

---

## Lock-Free Building Blocks

Each component was developed and tested independently before integration.

### Lock-Free Deque (`lockless/queue/`)

Based on [*Lock-Free Deques and Doubly Linked Lists* — Sundell & Tsigas, 2008](resources/Lock-Free-Deque-2008.pdf).

- **Marked pointers** use the lowest bit to flag nodes for deletion, notifying concurrent threads
- **Compare-and-swap (CAS)** operations drive all structural mutations
- **`correctPrev()`** — the most complex operation — walks backward from a node to repair prev-pointers after deletions, skipping logically removed nodes
- ~800 lines of implementation, ~1,000 lines of unit tests, plus multi-hour soak tests under ASan and TSan

### Hazard Pointers (`lockless/hazardPointers/`)

Each thread is given **8 hazard slots**, allowing it to shield up to 8 pointers simultaneously from being freed by other threads. Hazard registration and retirement use RAII guards for exception safety.

I initially tried reference counting for memory reclamation, but it proved vulnerable to race conditions under high contention, making hazard pointers the safer choice.

### Per-Thread Memory Pools (`lockless/memoryPool/`)

A true lock-free global multi-producer multi-consumer memory pool is an unsolved problem in computer science. Instead, each thread owns a private memory pool.

The core challenge is **cross-thread deallocation** — threads freeing memory they didn't allocate. Each pool has a lock-free **MPSC (multi-producer single-consumer) free-list**. When a thread deallocates another thread's pointer, it pushes to the owner's free-list. Before allocating new memory, a thread drains its free-list to recycle returned blocks.

### MPSC Queue (`lockless/MPSCQueue/`)

A lock-free ring buffer for the multi-producer single-consumer pattern. Power-of-2 capacity with bitmask indexing. Used internally by the memory pools for cross-thread deallocation.

---

## Locking Control Implementation (`lockingOrderBook/`)

To ensure a fair comparison, I built an **architecturally identical** locking order book that mirrors every structural decision but replaces all lock-free primitives with mutex-based alternatives.

| Component | Lockless | Locking |
|---|---|---|
| Publish Ring | Atomic `fetch_add` + CAS | `std::mutex` + `lock_guard` |
| Price Table | Atomic CAS install, atomic loads | Striped locking (16 mutexes) |
| Price-Level Queue | Sundell-Tsigas deque + hazard pointers | Mutex-protected doubly-linked list |
| Memory Pools | SPSC free-list + MPSC remote queue | `std::mutex` + `std::queue` |

This is not a strawman. The locking version uses striped locks on the hash table, fine-grained per-queue mutexes, and the same algorithmic structure. It is a legitimate, optimized locking design.

---

## Benchmarking

All benchmarks run on a university remote Linux server across **1–32 worker threads**, with **100 iterations** per thread count. Compiled with `-O3 -march=native -DNDEBUG` for maximum optimization. Each case outputs CSV data with columns: `num_threads`, `runtime_microseconds`, `throughput_ops_sec`.

### Benchmark Cases

| Case | Orders | Price Levels | Workload | What It Tests |
|------|--------|-------------|----------|---------------|
| **One** | 100K | 1 (same price) | 100% BUY submission | Maximum contention — all workers touch the same queue |
| **Two** | 100K | 100 | 100% BUY submission | Contention reduction via price-level spreading |
| **Three** | 100K | 1 (same price) | 70% submit / 30% cancel | Deque node removal under high contention |
| **Four** | 100K | 100 | 70% submit / 30% cancel | Combined price spreading + cancellation |
| **Five** | 100K | 1 (same price) | 60% BUY / 40% SELL | Matching engine stress test at a single price |
| **Six** | 100K | Realistic (GBM) | Realistic HFT data | Full pipeline: matching, multi-level, backtracking |

Case Six uses **Geometric Brownian Motion** to generate realistic price distributions and varying order sizes, exercising every code path in the system.

> **Status:** Final data collection in progress across all six cases.

---

## Testing

The test suite totals nearly **2,000 lines** across the lockless and locking order books, plus additional suites for each lock-free building block.

**Test Categories:**
- Symbol management and registration
- Order submission, cancellation, and double-cancel prevention
- Price tick conversion correctness
- Order matching (full match, partial match, no-cross)
- Multi-symbol independence
- Multi-threaded concurrent submission and cancellation (up to 32 threads)
- High-volume stress tests
- Shutdown-while-processing and lifecycle edge cases

**Sanitizer Coverage:**
- **Address Sanitizer (ASan)** — memory corruption, buffer overflows, use-after-free
- **Thread Sanitizer (TSan)** — data races, lock-order inversions

Both order books and all lock-free primitives pass the full suite under Normal, ASan, and TSan builds.

The lock-free deque additionally survived multi-hour soak tests under all three modes.

---

## Building

Requires **C++20** and **Google Test**.

```bash
# Standard build
g++ -std=c++20 -O2 parallelOrderBook/testParallelOrderBook.cpp -lgtest -o testParallel

# Address Sanitizer
g++ -std=c++20 -fsanitize=address -O2 parallelOrderBook/testParallelOrderBook.cpp -lgtest -o testParallel

# Thread Sanitizer
g++ -std=c++20 -fsanitize=thread -O2 parallelOrderBook/testParallelOrderBook.cpp -lgtest -o testParallel
```

Benchmark builds (for timing cases):
```bash
g++ -std=c++20 -O3 -march=native -DNDEBUG timing/caseOne/caseOneLockless.cpp -lgtest -o caseOneLockless
```

---

## Project Structure

```
orderBook/
├── parallelOrderBook/         # Lockless order book (the experiment)
├── lockingOrderBook/          # Locking order book (the control)
├── lockless/
│   ├── queue/                 # Lock-free deque (Sundell & Tsigas)
│   ├── hazardPointers/        # Hazard pointer memory reclamation
│   ├── memoryPool/            # Per-thread lock-free memory pools
│   └── MPSCQueue/             # Multi-producer single-consumer ring buffer
├── timing/
│   ├── prelim/                # Preliminary benchmarks
│   ├── caseOne/ – caseSix/    # Six contention scenarios
│   └── data/                  # CSV benchmark output
└── resources/
    └── Lock-Free-Deque-2008.pdf   # Reference paper
```

---

## References

- H. Sundell and P. Tsigas, *Lock-Free Deques and Doubly Linked Lists*, Journal of Parallel and Distributed Computing, 2008.
