#ifndef QUEUE_H
#define QUEUE_H

#include <atomic>

// List Node
template<typename T>
struct alignas(64) Node {
    T data;
    std::atomic<Node*> prev;
    std::atomic<Node*> next;
    Node(T data) : data(data) {
        prev.store(nullptr);
        next.store(nullptr);
    }
};

template<typename NodeType>
class Queue {
    public:

        // Head and tail pointers
        std::atomic<NodeType*> head;
        std::atomic<NodeType*> tail;

        Queue() {
            // Initialize head and tail
            head.store(nullptr);
            tail.store(nullptr);
        }

        virtual void insert(NodeType* nodePtr) {
            NodeType* tailPtr = tail.load();
            while(true) {
                if (!head) {
                    // List is empty, set both head and tail to newNode
                    if (head.compare_exchange_weak(tailPtr, nodePtr)) {
                        tail.store(nodePtr);
                        return;
                    }
                } else {
                    // Update the next pointer of the current tail
                    NodeType* nullNode = nullptr;
                    if (tailPtr->next.compare_exchange_weak(nullptr, nodePtr)) {
                        // Successfully updated tail's next, now update tail
                        nodePtr->prev.store(tailPtr);
                        tail.store(nodePtr);
                        return;
                    } else {
                        // Failed to update, reload tailPtr and retry
                        tailPtr = tail.load();
                        return;
                    }
                }
            }
        }

        // Removes head
        virtual void remove(NodeType* nodePtr = nullptr) {
            NodeType* headPtr = head.load();
            NodeType* headNext = headPtr->next.load();
            NodeType* nullNode = nullptr;
            while (true) {
                if (headNext) {
                    if (headNext->prev.compare_exchange_weak(headPtr, nullNode)) {
                        head.store(headNext);
                        return;
                    }
                } else {
                    if (head.compare_exchange_strong(headPtr, nullNode)) {
                        tail.store(nullptr);
                        return;
                    }
                }
            }
        }
};

#endif