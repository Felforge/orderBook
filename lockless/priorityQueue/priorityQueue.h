#include "../queue/queue.h"

// Derived class of lockless queue
// Values are assumed to be unique
class PriorityQueue: public Queue<Node<int>> {
    private:
        // Track if queue must be reversed
        bool reverse;

    public:
        PriorityQueue(bool reverse=false);

        // Override virtual function
        // Bool function should be passed in
        // This function needs to see if node1 is greater than node2
        void insert(Node<int>* nodePtr) override;
};