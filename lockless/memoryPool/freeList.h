#ifndef SPSCFREELIST_H
#define SPSCFREELIST_H

// SPSC Free List
// Used for managing reusable objects
// Only safe for a single thread
template<typename T>
class FreeList {
    private:
        // Internal linked list node structure
        struct Node {
            Node* next;
        };

        // Points to the top of the free list
        Node* head;
    
    public:
        // Constrcutor
        // Initializes head to nullptr
        FreeList() : head(nullptr) {}

        // Push a pointer to a T type object into the free list
        // NOTE: This implementation stores pointers, so you must manage memory outside the queue
        void push(T* item) {
            // Reuse object memory for node
            Node* newNode = reinterpret_cast<Node*>(item);

            // Push back current head
            newNode->next = head;

            // Set head to newNode
            head = newNode;
        }

        // Pop an object from the free list
        // Returns a pointer to a T, or nullptr if the list is empty
        T* pop() {
            // Retrieve head
            Node* node = head;

            // If node is nullptr list must be empty
            if (!node) {
                return nullptr;
            }

            // Bring node->next up
            head = node->next;

            // Convert node back into a pointer to a T and return it
            return reinterpret_cast<T*>(node);
        }

        // Return true if the free list is empty
        bool isEmpty() {
            return head == nullptr;
        }
};

#endif