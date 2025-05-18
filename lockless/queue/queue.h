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
            while(true) {
                NodeType* headPtr = head.load();
                NodeType* tailPtr = tail.load();
                if (!headPtr) {
                    // List is empty, set both head and tail to newNode
                    if (head.compare_exchange_weak(headPtr, nodePtr)) {
                        tail.store(nodePtr);
                        return;
                    }
                } else if (tailPtr) {
                    // Update the next pointer of the current tail
                    NodeType* nullNode = nullptr;
                    if (tailPtr->next.compare_exchange_weak(nullNode, nodePtr)) {
                        // Successfully updated tail's next, now update tail
                        nodePtr->prev.store(tailPtr);
                        tail.store(nodePtr);
                        return;
                    }
                }
                 // Retry
            }
        }

        // Removes head
        virtual NodeType* remove(NodeType* nodePtr = nullptr) {
            NodeType* nullNode = nullptr;
            NodeType* headNext;

            while (true) {
                NodeType* headPtr = head.load();
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
            }
        }
};

#endif