# Debugging Guide: Sundell-Tsigas Lock-Free Queue

## Critical Bug Found

### 1. Hazard Pointer Bug (CRITICAL)
**Location**: `lockless/hazardPointers/hazardPointers.h` lines 76-77

**Problem**: In `removeHazardPointer()`, when clearing `ptr3`, the code incorrectly clears `ptr4` instead:
```cpp
} else if (globalHazardPointers[hazardSlot].ptr3.load() == ptr) {
    // Matches ptr2, clear it
    globalHazardPointers[hazardSlot].ptr4.store(nullptr);  // BUG: should be ptr3
```

**Fix**: Should be:
```cpp
} else if (globalHazardPointers[hazardSlot].ptr3.load() == ptr) {
    // Matches ptr3, clear it
    globalHazardPointers[hazardSlot].ptr3.store(nullptr);
```

**Impact**: This causes memory leaks and potential use-after-free bugs since hazard pointers won't be properly removed.

## Other Potential Issues

### 2. Incomplete `isHazard()` Function
**Location**: `lockless/hazardPointers/hazardPointers.h` lines 87-94

**Problem**: Only checks `ptr1` and `ptr2`, but not `ptr3` and `ptr4`:
```cpp
if (globalHazardPointers[i].ptr1 == ptr || globalHazardPointers[i].ptr2 == ptr) {
    return true;
}
```

**Fix**: Should check all four pointers.

### 3. Memory Ordering Concerns
**Location**: Throughout `queue.h`

**Problem**: Some operations use `memory_order_acquire` but corresponding stores might need `memory_order_release` for proper synchronization.

### 4. Reference Counting Issues
**Location**: `copy()` and `releaseNode()` functions

**Problem**: The queue appears to implement a reference counting mechanism but there are potential race conditions in the counting logic.

## Debugging Strategies

### 1. Immediate Fixes
1. Fix the hazard pointer bug above
2. Update `isHazard()` to check all four pointer slots
3. Add memory ordering consistency checks

### 2. Testing Strategy
1. **Start with single-threaded tests** - ensure basic correctness
2. **Use sanitizers**:
   - AddressSanitizer: `g++ -fsanitize=address`
   - ThreadSanitizer: `g++ -fsanitize=thread`
3. **Add logging/assertions** for debugging

### 3. Debugging Tools

#### Add Debug Logging
```cpp
#ifdef DEBUG_QUEUE
#define QUEUE_LOG(msg) std::cout << "[Thread " << std::this_thread::get_id() << "] " << msg << std::endl
#else
#define QUEUE_LOG(msg)
#endif
```

#### Add Hazard Pointer Validation
```cpp
void validateHazardPointers() {
    for (size_t i = 0; i < MAX_HAZARD_POINTERS; i++) {
        auto ptr1 = globalHazardPointers[i].ptr1.load();
        auto ptr2 = globalHazardPointers[i].ptr2.load();
        auto ptr3 = globalHazardPointers[i].ptr3.load();
        auto ptr4 = globalHazardPointers[i].ptr4.load();
        
        if (ptr1 || ptr2 || ptr3 || ptr4) {
            QUEUE_LOG("Thread slot " << i << " has active hazard pointers");
        }
    }
}
```

### 4. Common Issues to Check

#### ABA Problem Prevention
- Verify that the memory pool + hazard pointer combination prevents ABA
- Check that nodes aren't reused while still accessible

#### Linearization Point Verification
- Each operation should have a clear linearization point
- Use assertions to verify queue state consistency

#### Memory Leak Detection
- Monitor retire list growth
- Ensure hazard pointers are properly cleared
- Check memory pool usage patterns

### 5. Testing Recommendations

1. **Stress test with varying thread counts** (2, 4, 8, 16 threads)
2. **Mixed operation testing** (push/pop from both ends + removeNode)
3. **Memory pressure testing** (limited memory pool size)
4. **Long-duration soak testing**

### 6. Specific Test Cases to Add

```cpp
// Test hazard pointer cleanup
TEST(LocklessQueueTest, HazardPointerCleanup) {
    // Verify all hazard pointers are cleared after operations
}

// Test ABA prevention
TEST(LocklessQueueTest, ABAPreventionTest) {
    // Rapid allocation/deallocation with node reuse
}

// Test memory pressure
TEST(LocklessQueueTest, MemoryPressureTest) {
    // Limited memory pool, force retry scenarios
}
```

## Next Steps

1. **Fix the critical hazard pointer bug immediately**
2. **Run tests with ThreadSanitizer** to detect race conditions
3. **Add debug logging** to trace operation sequences
4. **Implement the suggested test cases**
5. **Profile memory usage** to detect leaks

The concurrent tests being commented out suggests they were failing - likely due to the hazard pointer bug. Fix that first and then gradually re-enable concurrent testing.