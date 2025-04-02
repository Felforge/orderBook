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
    : id(id), price(price), quantity(quantity), type(side) {}
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
        int orderCount;
        OrderList* buyOrderList;
        OrderList* sellOrderList;

        OrderList* searchOrderList(int searchID, OrderList* orderList) {
            if (!orderList->order && !orderList->next) {
                return nullptr;
            }
            // Create reference to current level in linked list
            // Both buy and sell have to be searched
            OrderList* currentLayer = orderList;
            
            // Create reference to previous layer
            OrderList* prevLayer = nullptr;

            // Search for ID
            while (currentLayer->order->id != searchID && currentLayer->next) {
                prevLayer = currentLayer;
                currentLayer = currentLayer->next;
            }

            // Return either located order list or nullptr
            if (currentLayer->order->id != searchID && !currentLayer->next) {
                return nullptr;
            }
            return prevLayer;
        }

    public:
        // Constructor
        OrderBook() {
            // Create initial order lists
            buyOrderList = new OrderList(nullptr);
            sellOrderList = new OrderList(nullptr);

            // Declare orderCount value
            orderCount = 0;
        }

        // Destructor
        ~OrderBook() {
            // Delete order lists and orders allocated using new
            if (buyOrderList->order) {
                delete buyOrderList->order;
            }
            delete buyOrderList;
            if (sellOrderList->order) {
                delete sellOrderList->order;
            }
            delete sellOrderList;
            // Rest will delete recursively 
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

            if (type == "BUY" && !buyOrderList->order) {
                // Add into empty buy order list
                buyOrderList->order = new Order(orderCount, price, quantity, type);
            } else if (type == "SELL" && !sellOrderList->order) {
                // Add into empty sell order list
                sellOrderList->order = new Order(orderCount, price, quantity, type);
            } else {
                // Create reference to current level in linked list
                OrderList* currentLayer = nullptr;
                if (type == "BUY") {
                    OrderList* currentLayer = buyOrderList;
                } else if (type == "SELL") {
                    OrderList* currentLayer = sellOrderList;
                }

                // Find proper place for next order
                while (type == "BUY" && price <= currentLayer->order->price && currentLayer->next ||
                       type == "SELL" && price >= currentLayer->order->price && currentLayer->next) {
                        currentLayer = currentLayer->next;
                       }
                
                // Create order pointer
                Order* newOrder = new Order(orderCount, price, quantity, type);

                cout << "test" << endl;
                // Create next layer and insert order if needed
                if (!currentLayer->next) {
                    currentLayer->next = new OrderList(newOrder);
                } else {
                    currentLayer->next->order = newOrder;
                }
                cout << "test2" << endl;
            }

            // Print Order ID
            cout << "Order of type " << type << " for " << quantity << " units for price " << price << " successfully added. Order ID is " << orderCount << "." << endl;

            // Update counter for order ID
            orderCount += 1;
        }
        
        // Removes order from linked list
        // type should be "BUY" for buy, "SELL" for sell
        void removeOrder(int id) {
            // Return if order type is invalid or quanity is invalid
            if (id > orderCount) {
                cout << "Order Book Error: Invalid Order ID" << endl;
                return;
            }

            // Check buy
            OrderList* currentLayer = buyOrderList;
            OrderList* prevLayer = searchOrderList(id, buyOrderList);

            if (!prevLayer) {
                // Check sell
                OrderList* currentLayer = sellOrderList;
                OrderList* prevLayer = searchOrderList(id, sellOrderList);

                if (!prevLayer) {
                    cout << "Order Book Error: Order ID " << id << " Not Found" << endl;
                    return;
                }
            }

            // Get currentLayer out of prevLayer
            currentLayer = prevLayer->next;

            // Delete dynamically allocated order
            delete currentLayer->order;

            if (!currentLayer->next) {
                // Bottom layer no longer needed
                currentLayer = prevLayer;
            } else if (!prevLayer) {
                // Top layer no longer needed
                currentLayer = currentLayer->next;
            } else {
                // Get rid of current layer order
                prevLayer->next = currentLayer->next;
                currentLayer = prevLayer;
            }

            // Print Status
            cout << "Order " << id << " Successfully Removed." << endl;
        }

        // Match buy and sell orders
        void matchOrders() {
            while (buyOrderList->order && sellOrderList->order && buyOrderList->order->price >= sellOrderList->order->price) {
                double orderPrice = buyOrderList->order->price;
                int orderQuantity = min(buyOrderList->order->quantity, sellOrderList->order->quantity);
                if (buyOrderList->order->quantity == orderQuantity && sellOrderList->order->quantity == orderQuantity) {
                    // Edge case
                    removeOrder(buyOrderList->order->id);
                    removeOrder(sellOrderList->order->id);
                } else if (buyOrderList->order->quantity == orderQuantity) {
                    // Delete one and remove from larger
                    removeOrder(buyOrderList->order->id);
                    sellOrderList->order->quantity -= orderQuantity;
                } else if (sellOrderList->order->quantity == orderQuantity) {
                    // Delete one and remove from larger
                    removeOrder(sellOrderList->order->id);
                    buyOrderList->order->quantity -= orderQuantity;
                }
                cout << orderQuantity << " units sold for $" << orderPrice << "." << endl;
            }
        }
};

int main() {
    OrderBook orderBook = OrderBook();
    orderBook.addOrder(100.0, 10, "BUY");
    orderBook.addOrder(90.0, 5, "SELL");
    orderBook.addOrder(125.0, 15, "BUY");
    // orderBook.removeOrder(2);
    // orderBook.matchOrders();
    return 0;
}