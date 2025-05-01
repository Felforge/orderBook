#include <iostream>
#include <thread>
#include "parallelOrderBook.h"
using namespace std;

// Get place in list based on price
int getListIndex(double price) {
    return int(price * 100.0) - 1;
}

// Constructor for Order
Order::Order(void* memoryBlock, int orderID, int userID, string side, string ticker, int quantity, double price)
    : memoryBlock(memoryBlock), orderID(orderID), userID(userID), side(side), ticker(ticker), quantity(quantity), price(price) {}

// Constructor for Order Node
// This is a node of the doubly linked list
OrderNode::OrderNode(void* memoryBlock, Order* order) 
    : memoryBlock(memoryBlock), order(order), prev(nullptr), next(nullptr) {}

// Constructor for PriceLevel
// Wrapper for the doubly linked list that makes head and tail accessible 
PriceLevel::PriceLevel(void* memoryBlock, OrderNode* orderNode) 
    : memoryBlock(memoryBlock), head(orderNode), tail(orderNode) {} 

// Constructor for Ticker
Ticker::Ticker(void* memoryBlock, string ticker) 
    : memoryBlock(memoryBlock), ticker(ticker) {}

// OrderBook constructor
OrderBook::OrderBook(int numTickers, int numOrders)
    // Allocate memory to pool for worker threads
    : threadPool(sizeof(WorkerThread), MAX_THREADS - 1) {

    // Initialize Order ID
    orderID = 0;

    // Create worker threads
    for (int i = 0; i < MAX_THREADS - 1; i++) {
        void* newMemoryBlock = threadPool.allocate();
        instances[i] = nullptr; // new (newMemoryBlock) WorkerThread(newMemoryBlock, numTickers, numOrders);
    }
}

// OrderBook destructor
OrderBook::~OrderBook() {
    // Deallocate worker threads
    for (WorkerThread* inst: instances) {
        threadPool.deallocate(inst->memoryBlock);
    }
}

void OrderBook::addTicker(string ticker) {
    // Add ticker to all worker threads
    for (WorkerThread* inst: instances) {
        inst->requestAddTicker(ticker);
    }
}

void OrderBook::addOrder(int userID, string ticker, string side, int quantity, double price, bool print) {
    // Check for all local errors
    if (side != "BUY" && side != "SELL") {
        cout << "Order Book Error: Invalid Order Side" << endl;
        return;
    } else if (quantity <= 0) { 
        cout << "Order Book Error: Quantity Must Be An Integer Greater Than 0" << endl;
        return;
    } else if (price <= 0.0) {
        cout << "Order Book Error: Price Must Be A Number Greater Than 0" << endl;
        return;
    } else if (price > double(MAX_PRICE_IDX / 100)) {
        cout << "Order Book Error: Maximum Available Price is " << MAX_PRICE_IDX / 100 << endl;
        return;
    }

    // Call worker thread
    int threadNum = orderID % (MAX_THREADS - 1);
    instances[threadNum]->requestAddOrder(userID, orderID, ticker, side, quantity, price, print);

    // Update queues
    int listIdx = getListIndex(price);
    pair<int, int> data(orderID, threadNum);
    if (side == "BUY") {
        if (buyQueue[listIdx].size() == 0) {
            activeBuyPrices.push(listIdx);
        }
        buyQueue[listIdx].push(data);
    } else { // side == "SELL"
        if (sellQueue[listIdx].size() == 0) {
            activeSellPrices.push(listIdx);
        }
        sellQueue[listIdx].push(data);
    }

    // Update order ID
    orderID++;
}

int main() {
    OrderBook orderBook = OrderBook(1, 10);
    cout << "Order Book successfully created" << endl;
    // orderBook.addTicker("AAPL");
    // cout << "Ticker successfully added" << endl;
    // orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0);
}