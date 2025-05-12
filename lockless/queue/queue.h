#ifndef QUEUE_H
#define QUEUE_H

#include <atomic>

// List Node
// memoryBlock is for memory pool allocation
template<typename T>
struct alignas(64) Node {
    T data;
    void* memoryBlock;
    std::atomic<Node*> prev;
    std::atomic<Node*> next;
    Node(T data, void* memoryBlock=nullptr) : data(data), memoryBlock(memoryBlock) {
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
            NodeType* headPtr = head.load();
            NodeType* tailPtr = tail.load();
            while(true) {
                if (!headPtr) {
                    // List is empty, set both head and tail to newNode
                    if (head.compare_exchange_weak(headPtr, nodePtr)) {
                        tail.store(nodePtr);
                        return;
                    }
                } else {
                    // Update the next pointer of the current tail
                    NodeType* nullNode = nullptr;
                    if (tailPtr->next.compare_exchange_weak(nullNode, nodePtr)) {
                        // Successfully updated tail's next, now update tail
                        nodePtr->prev.store(tailPtr);
                        tail.store(nodePtr);
                        return;
                    } else {
                        // Failed to update, reload tailPtr and retry
                        tailPtr = tail.load();
                    }
                }
            }
        }

        // Removes head
        virtual NodeType* remove(NodeType* nodePtr = nullptr) {
            NodeType* headPtr = head.load();
            NodeType* nullNode = nullptr;
            NodeType* headNext;

            while (true) {
                if (!headPtr) {
                    return nullptr;
                } 
                headNext = headPtr->next.load();
                if (headNext) {
                    if (headNext->prev.compare_exchange_weak(headPtr, nullNode)) {
                        head.store(headNext);
                        return headPtr;
                    }
                } else {
                    if (head.compare_exchange_weak(headPtr, nullNode)) {
                        tail.store(nullptr);
                        return headPtr;
                    }
                }
                // Failed, retry
                headPtr = head.load();
            }
        }
};

#endif