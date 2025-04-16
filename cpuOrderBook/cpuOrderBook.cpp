#include <iostream>
#include "cpuOrderBook.h"
using namespace std;

// Constructor for Order
Order::Order(int orderID, int userID, string side, int quantity, double price)
    : orderID(orderID), userID(userID), side(side), quantity(quantity), price(price) {}

// Constructor for Order Node
// This is a node of the doubly linked list
OrderNode::OrderNode(Order* order) : order(order), prev(nullptr), next(nullptr) {}

// Constructor for PriceLevel
// Wrapper for the doubly linked list that makes head and tail accessible 
PriceLevel::PriceLevel(OrderNode* orderNode) : head(orderNode), tail(orderNode) {} 

// Constructor for Ticker
Ticker::Ticker(string ticker) : ticker(ticker), bestBuyOrder(nullptr), bestSellOrder(nullptr) {}

// Constructor
OrderBook::OrderBook() {
    orderID = 0;
    addTicker("AAPL");
    addTicker("MSFT");
    addTicker("NVDA");
    addTicker("AMZN");
    addTicker("GOOG");
}

// Destructor
OrderBook::~OrderBook() {
    // Delete every order pointer
    for (const auto& pair: orderMap) {
        delete pair.second;
    }

    // Delete every price level pointer and ticker pointer
    for (const auto& pair: tickerMap) {
        for (int i = 0; i < 100000; i++) {
            while (pair.second->buyOrderList[i]->tail != pair.second->buyOrderList[i]->head) {
                OrderNode* temp = pair.second->buyOrderList[i]->tail;
                pair.second->buyOrderList[i]->tail = temp->prev;
                delete temp;
            }
            delete pair.second->buyOrderList[i]->head;
            delete pair.second->buyOrderList[i];
            while (pair.second->sellOrderList[i]->tail != pair.second->sellOrderList[i]->head) {
                OrderNode* temp = pair.second->sellOrderList[i]->tail;
                pair.second->sellOrderList[i]->tail = temp->prev;
                delete temp;
            }
            delete pair.second->sellOrderList[i]->head;
            delete pair.second->sellOrderList[i];
        }
        delete pair.second;
    }
}

void OrderBook::addTicker(string ticker) {
    tickerMap[ticker] = new Ticker(ticker);
}

void OrderBook::addOrder(int userID, string ticker, string side, int quantity, double price, bool print) {
    // Check for all errors
    if (side != "BUY" && side != "SELL") {
        cout << "Order Book Error: Invalid Order Side" << endl;
        return;
    } else if (quantity <= 0) { 
        cout << "Order Book Error: Quantity Must Be An Integer Greater Than 0" << endl;
        return;
    } else if (price <= 0.0) {
        cout << "Order Book Error: Price Must Be A Number Greater Than 0" << endl;
        return;
    } else if (price > 1000.00) {
        cout << "Order Book Error: Maximum Available Price is 1000" << endl;
        return;
    } else if (tickerMap.find(ticker) == tickerMap.end()) {
        cout << "Order Book Error: Ticker is Invalid" << endl;
        return;
    }

    // Get index for price level
    int listIdx = int(price * 100.0) - 1;

    // Create new order and orderNode objects
    Order* newOrder = new Order(userID, orderID, side, quantity, price);
    OrderNode* orderNode = new OrderNode(newOrder);

    // Insert order into order map
    orderMap[orderID] = newOrder;

    if (side == "BUY") {
        if (tickerMap[ticker]->buyOrderList[listIdx] == nullptr) {
            tickerMap[ticker]->buyOrderList[listIdx] = new PriceLevel(orderNode);
            if (tickerMap[ticker]->bestBuyOrder == nullptr || price > tickerMap[ticker]->bestBuyOrder->head->order->price) {
                tickerMap[ticker]->bestBuyOrder = tickerMap[ticker]->buyOrderList[listIdx];
            }
        } else {
            orderNode->prev = tickerMap[ticker]->buyOrderList[listIdx]->tail;
            tickerMap[ticker]->buyOrderList[listIdx]->tail->next = orderNode;
            tickerMap[ticker]->buyOrderList[listIdx]->tail = orderNode;
        }
    } else { // side == "SELL"
        if (tickerMap[ticker]->sellOrderList[listIdx] == nullptr) {
            tickerMap[ticker]->sellOrderList[listIdx] = new PriceLevel(orderNode);
            if (tickerMap[ticker]->bestSellOrder == nullptr || price < tickerMap[ticker]->bestSellOrder->head->order->price) {
                tickerMap[ticker]->bestSellOrder = tickerMap[ticker]->sellOrderList[listIdx];
            }
        } else {
            orderNode->prev = tickerMap[ticker]->sellOrderList[listIdx]->tail;
            tickerMap[ticker]->sellOrderList[listIdx]->tail->next = orderNode;
            tickerMap[ticker]->sellOrderList[listIdx]->tail = orderNode;
        }
    }

    // Print Order ID
    if (print) {
        if (print) {
        std::cout << "Order successfully added:\n"
                  << "  Type: " << side
                  << "\n  Quantity: " << quantity
                  << "\n  Ticker: " << ticker
                  << "\n  Price: " << price
                  << "\n  Order ID: " << orderID
                  << std::endl;
        }
    }

    // Update counter for order ID
    orderID++;
}

// int main() {
//     OrderBook orderBook = OrderBook();
//     if (orderBook.buyOrderList[0] == nullptr) {
//         cout << "test" << endl;
//     }
//     return 0;
// }