# Lockless Parallel Order Book Research

## Abstract

This project explores whether an order book system built around a lockless deque can outperform traditional locking designs in terms of latency and throughput. Starting from baseline single-threaded CPU implementations, I developed progressively more optimized versions culminating in a parallel lockless order book. The design draws on Sundell and Tsigas’ lock-free deque, extended with hazard pointers, per-thread memory pools, and custom allocation strategies. Through this research, I aim to evaluate whether lockless systems, long considered impractical under heavy contention, may offer unique advantages in latency-sensitive financial applications such as high-frequency trading.

---

## The Idea

The original idea was to investigate whether GPU acceleration on a CUDA system could potentially benefit order book systems. While this approach may not make much sense to experts in the field, I began this project simply out of curiosity.  

As I built baseline implementations to benchmark against, I moved through:  

1. A **naive single-core CPU implementation**  
2. An **optimized single-core CPU implementation**  
3. A **parallel CPU version**  

While working on the parallel version, I discovered the world of **lockless architectures**, and specifically a lockless deque described in research papers by Sundell and Tsigas (2004, revised 2008). This data structure seemed promising but underutilized in real-world systems, sparking the idea for this project.

---

## Why Order Books?

- **Finance + Computer Science:** I wanted to work in an area of finance that directly overlaps with computer science.  
- **Market Impact:** More efficient order books could benefit market makers, who earn fees per trade and constantly seek latency improvements.  
- **Lockless Curiosity:** Lockless systems are theoretically powerful but rarely used in practice. I wanted to explore whether they’ve been overlooked unfairly.  

---

## The Question

**Can a lockless parallel order book system outperform a traditional mutex-based system?**  

Mutex locks are simple and widely used but introduce contention and latency. Lockless algorithms, while much harder to implement and debug, may offer superior performance in the right context.

---

## Approaches

### 1. The Naive Approach

- **Structure:** Singly linked list sorted by price, then by age.  
- **Drawbacks:** Insertions become exponentially more expensive with volume. Removal requires parent pointer traversal. Uses slow dynamic memory allocation.  
- **Latency Results (microseconds):**  
  - 10 orders: 2.3  
  - 100 orders: 5.2  
  - 1,000 orders: 43.7  
  - 10,000 orders: 196.5  
  - 100,000 orders: 1987.6  

---

### 2. The Optimized Approach

- **Structure:** Double-ended queue (deque) per price level, stored in a hash map.  
- **Improvements:**  
  - Eliminated sorting by pushing to deque ends.  
  - Faster cancellations via direct pointer connections.  
  - Memory pools for faster allocation/deallocation.  
- **Latency Results (microseconds):**  
  - 10 orders: 3.2  
  - 100 orders: 2.1  
  - 1,000 orders: 1.7  
  - 10,000 orders: 1.7  
  - 100,000 orders: 1.7  

---

### 3. The Lockless Deque

Based on Sundell & Tsigas (2008).  

**Key Features:**  

- Operations: push left, push right, pop left, pop right, remove node.  
- **Marked pointers:** notify threads of nodes marked for deletion.  
- **Compare-and-exchange (CAS):** lockless atomic updates.  
- **Correct-prev function:** ensures both pointers of a removed node are cleaned up properly.  
- **Hazard slots:** protect pointers from unsafe deletion.  
- **Memory pools:** thread-local pools with a lockless “free list” for cross-thread deallocation.  

**Testing:** ~1000 lines of unit tests and multiple long-running test cases.

---

### 4. The Parallel Order Book

**Status:** ~99% complete  

**Design Highlights:**  

- Lockless publish ring (CAS-based, circular buffer).  
- Lockless hash tables for price levels.  
- Each price level uses a lockless deque.  
- Thread-local memory pools to avoid global contention.  
- Matching engine logic embedded into insertion process.  
- Best bid/ask tracked locklessly, with naive fallback search when a price level empties.  

---

## Challenges

### 1. Lockless Memory Pools

- **Problem:** A true lockless global memory pool is unsolved in computer science.  
- **Solution:** Thread-local pools with per-thread “free lists” for cross-thread deallocation.  

### 2. Best Price Level Tracking

- **Problem:** Lockless priority queues are too slow for this workload.  
- **Solution:** Naive loop-back search for next valid level, which is efficient in active markets since price levels are typically very close together.  

---

## Future Work & Implications

- Complete debugging and finalize implementation.  
- Benchmark against a traditional locking parallel order book (control system).  
- Publish results as a proof-of-concept, regardless of whether the hypothesis holds.  
- Potentially collaborate with professors to prepare the research for academic publication.  

---

## Conclusion

I believe that an order book built around a lockless deque has the potential to outperform modern locking systems in both latency and throughput. Even if this specific design fails, the exploration may spark renewed interest in applying lockless architectures to real-world, high-performance systems.  
