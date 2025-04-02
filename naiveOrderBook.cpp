#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

struct Order {
    int id;         // Unique order ID
    double price;   // Order Price
    int quantity;   // Order size
    string type;    // "BUY" for buy, "SELL" for sell

    // Constructor
    Order(int id, double price, int quantity, string side)
    : id(id), price(price), quantity(quantity), side(side) {}
};

struct OrderList {
    Order* order;       // Order on current level of linked list
    OrderList* next;    // Reference to next order

    // Constructor
    OrderList(Order* order) : order(order), next(nullptr) {}
};

class OrderBook {
    private:
        // Needed for order ID
        int orderCount = 0;
        OrderList* buyOrderList;
        OrderList* sellOrderList;

    public:
        // Constructor
        OrderBook() {
            // Create initial order lists
            OrderList* buyOrderList = new OrderList(nullptr);
            OrderList* sellOrderList = new OrderList(nullptr);
        }

        // Destructor
        ~OrderBook() {
            // Delete order lists allocated using new
            delete buyOrderList;
            delete sellOrderList;
        }

        // Creates order in linked list
        // type should be "BUY" for buy, "SELL" for sell
        void addOrder(double price, int quantity, string type) {
            // Return if order type is invalid or quanity is invalid
            if (type != "BUY" && type != "SELL") {
                cout << "Order Book Error: Invalid Order Type" << endl;
                return;
            } else if (quantity <= 0) { 
                cout << "Order Book Error: Quantity Must Be Integer Greater Than 0" << endl;
                return;
            }

            if (type == "BUY" && buyOrderList.order == nullptr) {
                // Add into empty buy order list
                buyOrderList.order = new Order(orderCount, price, quantity, type);
            } else if (type == "SELL" && sellOrderList.order == nullptr) {
                // Add into empty sell order list
                sellOrderList.order = new Order(orderCount, price, quantity, type);
            } else {
                // Create reference to current level in linked list
                if (type == "BUY") {
                    OrderList* currentLayer = buyOrderList;
                } else (type == "SELL") {
                    OrderList* currentLayer = sellOrderList;
                }

                // Find proper place for next order
                while (type == "BUY" && price <= currentLayer.order->price && currentLayer.next != nullptr ||
                       type == "SELL" && price >= currentLayer.order->price && currentLayer.next != nullptr) {
                        currentLayer = currentLayer.next;
                       }
                
                // Create order pointer
                Order* newOrder = new Order(orderCount, price, quantity, type)
                // Create next layer and insert order if needed
                if (currentLayer.next == nullptr) {
                    currentLayer.next = new OrderList(orderCount, price, quantity, type);
                }
            }

            // Print Order ID
            cout << "Order Successfully Added. Order ID is " << orderCount << "." << endl;

            // Update counter for order ID
            orderCount += 1;
        }
        
        // Removes order from linked list
        // type should be "BUY" for buy, "SELL" for sell
        void removeOrder(int id, string type) {
            // Return if order type is invalid or quanity is invalid
            if (type != "BUY" && type != "SELL") {
                cout << "Order Book Error: Invalid Order Type" << endl;
                return;
            }

            // Create reference to current level in linked list
            if (type == "BUY") {
                OrderList* currentLayer = buyOrderList;
            } else (type == "SELL") {
                OrderList* currentLayer = sellOrderList;
            }
            
            // Create reference to previous layer
            OrderList* prevLayer = nullptr;

            // Search for ID
            while (currentLayer.order.id != id && currentLayer.next != nullptr) {
                prevLayer = currentLayer;
                currentLayer = currentLayer.next;
            }

            // Catch error of order not being found
            if (currentLayer.order.id != id && currentLayer.next == nullptr) {
                cout << "Order Book Error: Order ID Not Found" << endl;
                return;
            }

            if (currentLayer.next == nullptr) {
                // Bottom layer no longer needed
                currentLayer = prevLayer;
            } else if (prevLayer == nullptr) {
                // Top layer no longer needed
                currentLayer = currentLayer.next;
            } else {
                // Get rid of current layer order
                prevLayer.next = currentLayer.next;
                currentLayer = prevLayer;
            }

            // Update order lists
            if (type == "BUY") {
                buyOrderList = currentLayer;
            } else (type == "SELL") {
                sellOrderList = currentLayer;
            }

            // Print Status
            cout << "Order " << id << " Successfully Removed." << endl;
        }

        // Match buy and sell orders
        void matchOrders() {
            while (buyOrderList->order->price > sellOrderList->order->price) {
                double orderPrice = buyOrderList->order->price;
                int orderQuantity = min(buyOrderList->order->quantity, sellOrderList->order->quantity);
                if (buyOrderList->order->quantity == orderQuantity && sellOrderList->order->quantity == orderQuantity) {
                    // Edge case
                    removeOrder(buyOrderList->order->id, "BUY");
                    removeOrder(sellOrderList->order->id, "SELL");
                } else if (buyOrderList->order->quantity == orderQuantity) {
                    // Delete one and remove from larger
                    removeOrder(buyOrderList->order->id, "BUY");
                    sellOrderList->order->quantity -= orderQuantity;
                } else if (sellOrderList->order->quantity == orderQuantity) {
                    // Delete one and remove from larger
                    removeOrder(sellOrderList->order->id, "SELL");
                    buyOrderList->order->quantity -= orderQuantity;
                }
                cout << orderQuantity << " units sold for $" << orderPrice << "." << endl;
            }
        }
}
