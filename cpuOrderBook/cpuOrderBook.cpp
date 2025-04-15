#include <iostream>
#include "cpuOrderBook.h"
using namespace std;

Order::Order(int id, string side, string type, int quantity, double price)
    : id(id), side(side), type(type), quantity(quantity), price(price) {}

PriceLevel::PriceLevel(Order* order) : order(order), prev(nullptr), next(nullptr) {}

Ticker::Ticker(string ticker) : ticker(ticker), bestBuyOrder(nullptr), bestSellOrder(nullptr) {}

// Constructor
OrderBook::OrderBook() {
    orderCount = 0;
}

void OrderBook::addOrder(string side, string type, int quantity, double price, bool print, bool execute) {
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
    } else if (type != "MARKET" && type != "LIMIT" && type != "STOP") {
        cout << "Order Book Error: Invalid Order Type" << endl;
        return;
    }
}

// int main() {
//     OrderBook orderBook = OrderBook();
//     if (orderBook.buyOrderList[0] == nullptr) {
//         cout << "test" << endl;
//     }
//     return 0;
// }