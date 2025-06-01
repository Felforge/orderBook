#pragma once

#include <atomic>
#include <memory>
#include <cstdint>
#include <cassert>
#include <thread>
#include <utility>

// Reference-counted, lock-free, doubly-linked deque per Sundell & Tsigas (OPODIS04)
// Only push_back (PushRight) and pop_front (PopLeft) are exposed here.

namespace lockfree_deque_internal {

template <typename T>
class Node;

// We use pointer tagging for marking deletion, so all pointers must be at least 2-byte aligned.
template <typename T>
class MarkedPtr {
    static constexpr uintptr_t MARK_MASK = 1;
    uintptr_t bits;
public:
    MarkedPtr() : bits(0) {}
    MarkedPtr(Node<T>* ptr, bool mark) : bits((uintptr_t)ptr | (mark ? MARK_MASK : 0)) {}

    Node<T>* get_ptr() const { return (Node<T>*)(bits & ~MARK_MASK); }
    bool get_mark() const { return bits & MARK_MASK; }

    bool operator==(const MarkedPtr& other) const { return bits == other.bits; }
    bool operator!=(const MarkedPtr& other) const { return !(*this == other); }
};

template <typename T>
struct Node {
    std::atomic<MarkedPtr<T>> prev;
    std::atomic<MarkedPtr<T>> next;
    std::atomic<size_t> refcount;
    T value;
    bool is_dummy;

    Node() : prev(MarkedPtr<T>(nullptr, false)), next(MarkedPtr<T>(nullptr, false)), refcount(1), is_dummy(true) {}
    Node(const T& val) : prev(MarkedPtr<T>(nullptr, false)), next(MarkedPtr<T>(nullptr, false)), refcount(1), value(val), is_dummy(false) {}
};

// Reference counting primitives (Valois/Michael-Scott style)
template <typename T>
Node<T>* copy(Node<T>* node) {
    if (node) node->refcount.fetch_add(1, std::memory_order_relaxed);
    return node;
}

template <typename T>
void rel(Node<T>* node);

template <typename T>
void terminate_node(Node<T>* node) {
    if (!node) return;
    rel<T>(node->prev.load().get_ptr());
    rel<T>(node->next.load().get_ptr());
    delete node;
}

template <typename T>
void rel(Node<T>* node) {
    if (!node) return;
    if (node->refcount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        terminate_node<T>(node);
    }
}

// Pointer dereferencing with reference increment, returns nullptr if marked is set
template <typename T>
Node<T>* deref(std::atomic<MarkedPtr<T>>* ptr) {
    MarkedPtr<T> mp = ptr->load(std::memory_order_acquire);
    if (mp.get_mark()) return nullptr;
    Node<T>* node = mp.get_ptr();
    if (node) copy<T>(node);
    return node;
}

// Like deref but always returns pointer (even if marked)
template <typename T>
Node<T>* deref_d(std::atomic<MarkedPtr<T>>* ptr) {
    MarkedPtr<T> mp = ptr->load(std::memory_order_acquire);
    Node<T>* node = mp.get_ptr();
    if (node) copy<T>(node);
    return node;
}

template <typename T>
class LockFreeDeque {
public:
    LockFreeDeque() {
        head = new Node<T>();
        tail = new Node<T>();
        head->next.store(MarkedPtr<T>(tail, false));
        tail->prev.store(MarkedPtr<T>(head, false));
    }
    ~LockFreeDeque() {
        rel<T>(head);
        rel<T>(tail);
    }

    // PushRight == push_back
    void push_back(const T& value) {
        Node<T>* node = new Node<T>(value);
        Node<T>* next = copy(tail);
        Node<T>* prev = deref(&next->prev);
        while (true) {
            MarkedPtr<T> prev_next = prev->next.load();
            if (prev_next != MarkedPtr<T>(next, false)) {
                prev = help_insert(prev, next);
                continue;
            }
            node->prev.store(MarkedPtr<T>(prev, false));
            node->next.store(MarkedPtr<T>(next, false));
            if (prev->next.compare_exchange_weak(prev_next, MarkedPtr<T>(node, false))) {
                copy(node);
                break;
            }
            // back-off: std::this_thread::yield();
        }
        push_common(node, next);
    }

    // PopLeft == pop_front
    bool pop_front(T& out_value) {
        Node<T>* prev = copy(head);
        while (true) {
            Node<T>* node = deref(&prev->next);
            if (node == tail) {
                rel<T>(node);
                rel<T>(prev);
                return false; // empty
            }
            MarkedPtr<T> link1 = node->next.load();
            if (link1.get_mark()) {
                help_delete(node);
                rel<T>(node);
                continue;
            }
            MarkedPtr<T> expected = link1;
            if (node->next.compare_exchange_weak(expected, MarkedPtr<T>(link1.get_ptr(), true))) {
                help_delete(node);
                Node<T>* next = deref_d(&node->next);
                prev = help_insert(prev, next);
                rel<T>(prev);
                rel<T>(next);
                out_value = node->value;
                remove_cross_reference(node);
                rel<T>(node);
                return true;
            }
            // back-off: std::this_thread::yield();
        }
    }

    LockFreeDeque(const LockFreeDeque&) = delete;
    LockFreeDeque& operator=(const LockFreeDeque&) = delete;

private:
    Node<T>* head;
    Node<T>* tail;

    // MarkPrev: set deletion mark on prev pointer
    void mark_prev(Node<T>* node) {
        while (true) {
            MarkedPtr<T> link1 = node->prev.load();
            if (link1.get_mark() ||
                node->prev.compare_exchange_weak(link1, MarkedPtr<T>(link1.get_ptr(), true)))
                break;
        }
    }

    // PushCommon: update prev pointer of next node
    void push_common(Node<T>* node, Node<T>* next) {
        while (true) {
            MarkedPtr<T> link1 = next->prev.load();
            if (link1.get_mark() || node->next.load() != MarkedPtr<T>(next, false))
                break;
            if (next->prev.compare_exchange_weak(link1, MarkedPtr<T>(node, false))) {
                copy(node);
                rel<T>(link1.get_ptr());
                if (node->prev.load().get_mark()) {
                    Node<T>* prev2 = copy(node);
                    prev2 = help_insert(prev2, next);
                    rel<T>(prev2);
                }
                break;
            }
            // back-off: std::this_thread::yield();
        }
        rel<T>(next);
        rel<T>(node);
    }

    // HelpInsert: repair prev pointers
    Node<T>* help_insert(Node<T>* prev, Node<T>* node) {
        Node<T>* last = nullptr;
        while (true) {
            Node<T>* prev2 = deref(&prev->next);
            if (!prev2) {
                if (last) {
                    mark_prev(prev);
                    Node<T>* next2 = deref_d(&prev->next);
                    MarkedPtr<T> expected(prev, false);
                    prev->next.compare_exchange_weak(expected, MarkedPtr<T>(next2, false));
                    rel<T>(prev);
                }
                rel<T>(prev2);
                rel<T>(prev);
                prev = last;
                last = nullptr;
            } else {
                Node<T>* prev3 = deref_d(&prev->prev);
                rel<T>(prev);
                prev = prev3;
            }
            MarkedPtr<T> link1 = node->prev.load();
            if (link1.get_mark()) {
                rel<T>(prev2);
                break;
            }
            if (prev2 != node) {
                if (last) rel<T>(last);
                last = prev;
                prev = prev2;
                continue;
            }
            rel<T>(prev2);
            if (link1.get_ptr() == prev) break;
            if (prev->next.load() == MarkedPtr<T>(node, false) &&
                node->prev.compare_exchange_weak(link1, MarkedPtr<T>(prev, false))) {
                copy(prev);
                rel<T>(link1.get_ptr());
                if (!prev->prev.load().get_mark())
                    break;
            }
            // back-off: std::this_thread::yield();
        }
        if (last) rel<T>(last);
        return prev;
    }

    // HelpDelete: delete a logically-marked node from the list
    void help_delete(Node<T>* node) {
        mark_prev(node);
        Node<T>* last = nullptr;
        Node<T>* prev = deref_d(&node->prev);
        Node<T>* next = deref_d(&node->next);
        while (true) {
            if (prev == next) break;
            if (next->next.load().get_mark()) {
                mark_prev(next);
                Node<T>* next2 = deref_d(&next->next);
                rel<T>(next);
                next = next2;
                continue;
            }
            Node<T>* prev2 = deref(&prev->next);
            if (!prev2) {
                if (last) {
                    mark_prev(prev);
                    Node<T>* next2 = deref_d(&prev->next);
                    MarkedPtr<T> expected(prev, false);
                    prev->next.compare_exchange_weak(expected, MarkedPtr<T>(next2, false));
                    rel<T>(prev);
                }
                rel<T>(next2);
                rel<T>(prev);
                prev = last;
                last = nullptr;
            } else {
                Node<T>* prev3 = deref_d(&prev->prev);
                rel<T>(prev);
                prev = prev3;
            }
            if (prev2 != node) {
                if (last) rel<T>(last);
                last = prev;
                prev = prev2;
                continue;
            }
            rel<T>(prev2);
            MarkedPtr<T> expected(node, false);
            if (prev->next.compare_exchange_weak(expected, MarkedPtr<T>(next, false))) {
                copy(next);
                rel<T>(node);
                break;
            }
            // back-off: std::this_thread::yield();
        }
        if (last) rel<T>(last);
        rel<T>(prev);
        rel<T>(next);
    }

    // RemoveCrossReference: break cyclic garbage
    void remove_cross_reference(Node<T>* node) {
        while (true) {
            Node<T>* prev = node->prev.load().get_ptr();
            if (prev->prev.load().get_mark()) {
                Node<T>* prev2 = deref_d(&prev->prev);
                node->prev.store(MarkedPtr<T>(prev2, true));
                rel<T>(prev);
                continue;
            }
            Node<T>* next = node->next.load().get_ptr();
            if (next->prev.load().get_mark()) {
                Node<T>* next2 = deref_d(&next->next);
                node->next.store(MarkedPtr<T>(next2, true));
                rel<T>(next);
                continue;
            }
            break;
        }
    }
};

} // namespace lockfree_deque_internal

// Alias for external usage
template <typename T>
using LockFreeDeque = lockfree_deque_internal::LockFreeDeque<T>;