#include <iostream>
#include <string>
#include <algorithm>
#include "naiveOrderBook.h"
using namespace std;

// Everything here was already declared in the header


Order::Order(int id, double price, int quantity, string side)
    : id(id), price(price), quantity(quantity), type(side) {}

OrderList::OrderList(Order* order) : order(order), next(nullptr) {}

// Order Book Class member
OrderList* OrderBook::searchOrderList(int searchID, OrderList* orderList) {
    if (!orderList->order && !orderList->next) {
        return nullptr;
    }

    // Create reference to current level in linked list
    // Both buy and sell have to be searched
    OrderList* currentLayer = orderList;
    
    // Create reference to previous layer
    OrderList* prevLayer = currentLayer;

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


// Constructor
// Order Book Class member
OrderBook::OrderBook() {
    // Create initial order lists
    buyOrderList = new OrderList(nullptr);
    sellOrderList = new OrderList(nullptr);

    // Declare orderCount value
    orderCount = 0;
}

// Destructor
// Order Book Class member
OrderBook::~OrderBook() {
    // Delete order lists and orders allocated using new
    OrderList* currentLayer = buyOrderList;
    while (currentLayer && currentLayer->next) {
        currentLayer = currentLayer->next;
        if (currentLayer->order) {
            delete currentLayer->order;
        }
        delete currentLayer;
    }

    currentLayer = sellOrderList;
    while (currentLayer && currentLayer->next) {
        currentLayer = currentLayer->next;
        if (currentLayer->order) {
            delete currentLayer->order;
        }
        delete currentLayer;
    }

    if (buyOrderList) {
        if (buyOrderList->order) {
            delete buyOrderList->order;
        }
        delete buyOrderList;
    }

    if (sellOrderList) {
        if (sellOrderList->order) {
            delete sellOrderList->order;
        }
        delete sellOrderList;
    }
}

// Creates order in linked list
// type should be "BUY" for buy, "SELL" for sell
// Order Book Class member
void OrderBook::addOrder(double price, int quantity, string type, bool print) {
    // Return if order type is invalid or quanity is invalid
    if (type != "BUY" && type != "SELL") {
        cout << "Order Book Error: Invalid Order Type" << endl;
        return;
    } else if (quantity <= 0) { 
        cout << "Order Book Error: Quantity Must Be An Integer Greater Than 0" << endl;
        return;
    } else if (price <= 0.0) {
        cout << "Order Book Error: Price Must Be A Number Greater Than 0" << endl;
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
        OrderList* currentLayer;
        if (type == "BUY") {
            currentLayer = buyOrderList;
        } else if (type == "SELL") {
            currentLayer = sellOrderList;
        }

        // Find proper place for next order
        while (type == "BUY" && price <= currentLayer->order->price && currentLayer->next ||
               type == "SELL" && price >= currentLayer->order->price && currentLayer->next) {
            currentLayer = currentLayer->next;
        }

        // Create order pointer
        Order* newOrder = new Order(orderCount, price, quantity, type);

        // Create next layer and insert order if needed
        if (!currentLayer->next && 
            (type == "BUY" && price <= currentLayer->order->price ||
             type == "SELL" && price >= currentLayer->order->price )) {
            currentLayer->next = new OrderList(newOrder);
        } else {
            OrderList* pushedDown = new OrderList(currentLayer->order);
            pushedDown->next = currentLayer->next;
            currentLayer->next = pushedDown;
            currentLayer->order = newOrder;
        }
    }

    // Print Order ID
    if (print) {
        cout << "Order of type " << type << " for " << quantity << " units for price " << price << " successfully added. Order ID is " << orderCount << "." << endl;
    }

    // Update counter for order ID
    orderCount++;
}

void OrderBook::removeOrder(int id, string type, bool print) {
    // Return if order ID is invalid
    if (id >= orderCount) {
        cout << "Order Book Error: Invalid Order ID" << endl;
        return;
    }

    // Determine the order list to search
    OrderList* orderList = nullptr;
    if (type == "BUY") {
        orderList = buyOrderList;
    } else if (type == "SELL") {
        orderList = sellOrderList;
    } else {
        cout << "Order Book Error: Invalid Order Type" << endl;
        return;
    }

    // Search for the order in the specified list
    OrderList* prevLayer = searchOrderList(id, orderList);
    if (!prevLayer) {
        cout << "Order Book Error: Order ID " << id << " Not Found" << endl;
        return;
    }

    // Get the current layer
    OrderList* currentLayer = (prevLayer->order && prevLayer->order->id == id) ? prevLayer : prevLayer->next;

    // Update the linked list
    if (currentLayer && currentLayer->order && currentLayer->order->id == id) {
        if (currentLayer->order) {
            delete currentLayer->order;
        }

        if (currentLayer == orderList) {
            if (type == "BUY") {
                buyOrderList = buyOrderList->next;
            } else {
                sellOrderList = sellOrderList->next;
            }
            delete currentLayer;
        } else {
            prevLayer->next = currentLayer->next;
            delete currentLayer;
        }

        // Print status
        if (print) {
            cout << "Order ID " << id << " Successfully Removed." << endl;
        }
    } else {
        cout << "Order Book Error: Order ID " << id << " Not Found" << endl;
    }
}

// Match buy and sell orders
// Order Book Class member
void OrderBook::matchOrders(bool print) {
    while (buyOrderList->order && sellOrderList->order && buyOrderList->order->price >= sellOrderList->order->price) {
        double orderPrice = buyOrderList->order->price;
        int orderQuantity = min(buyOrderList->order->quantity, sellOrderList->order->quantity);

        if (buyOrderList->order->quantity == sellOrderList->order->quantity) {
            // Edge case: both quantities are the same
            removeOrder(buyOrderList->order->id, "BUY", false);
            removeOrder(sellOrderList->order->id, "SELL", false);
        } else if (buyOrderList->order->quantity == orderQuantity) {
            // Delete one and remove from larger
            removeOrder(buyOrderList->order->id, "BUY", false);
            sellOrderList->order->quantity -= orderQuantity;
        } else if (sellOrderList->order->quantity == orderQuantity) {
            // Delete one and remove from larger
            removeOrder(sellOrderList->order->id, "SELL", false);
            buyOrderList->order->quantity -= orderQuantity;
        }
        // Print completed order
        if (print) {
            cout << orderQuantity << " units sold for $" << orderPrice << "." << endl;
        }
    }
}

// int main() {
//     OrderBook orderBook = OrderBook();
//     orderBook.addOrder(100.0, 10, "BUY");
//     orderBook.addOrder(90.0, 5, "SELL");
//     orderBook.addOrder(125.0, 15, "BUY");
//     orderBook.removeOrder(2);
//     orderBook.matchOrders();
//     return 0;
// }