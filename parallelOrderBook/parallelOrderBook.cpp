#include <iostream>
#include "parallelOrderBook.h"
using namespace std;

// Number of different price levels available
const int NUM_LEVELS = 100000;

// Constructor for Ticker
Ticker::Ticker(void* memoryBlock, string ticker) 
    : memoryBlock(memoryBlock), ticker(ticker), bestBuyOrder(nullptr), bestSellOrder(nullptr) {}

// Constructor
// numTickers is the maximum number of tickers that could be added
OrderBook::OrderBook(int numTickers, int numOrders) 
    // Declare memory pool
    : orderPool(sizeof(Order), numOrders), nodePool(sizeof(OrderNode), numOrders), priceLevelPool(sizeof(OrderList), 2 * numTickers * NUM_LEVELS), 
    tickerPool(sizeof(Ticker), numTickers), orderList(priceLevelPool.allocate(), orderPool, nodePool) {

    // Declare max number of tickers
    maxTickers = numTickers;

    // Declare max number of orders
    maxOrders = numOrders;

    // Declare ID counter
    orderID.store(0);

    // Declare active threads
    activeThreads = 0;
}

// Destructor
// Orders and OrderNodes are deallocated elsewhere
OrderBook::~OrderBook() {
    // Delete every price level pointer and ticker pointer
    for (const auto& pair: tickerMap) {
        for (const auto& buyPair: pair.second->buyOrderMap) {
            priceLevelPool.deallocate(buyPair.second->memoryBlock);
        }
        for (const auto& sellPair: pair.second->sellOrderMap) {
            priceLevelPool.deallocate(sellPair.second->memoryBlock);
        }
        tickerPool.deallocate(pair.second->memoryBlock);
    }
}

void OrderBook::addTicker(string ticker) {
    if (tickerMap.size() == maxTickers) {
        cout << "Order Book Error: Too Many Tickers" << endl;
        return;
    }
    void* memoryBlock = tickerPool.allocate();
    tickerMap[ticker] = new (memoryBlock) Ticker(memoryBlock, ticker);
}

void OrderBook::threadWorkerInsert(OrderList& orderList, OrderNode* node) {
    orderList.insert(node);
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
    } else if (tickerMap.find(ticker) == tickerMap.end()) {
        cout << "Order Book Error: Ticker is Invalid" << endl;
        return;
    } else if (orderID + 1 >= maxOrders) {
        cout << "Order Book Error: Max Order Limit Reached" << endl;
        return;
    }

    // Allocate order and order node
    void* orderMemoryBlock = orderPool.allocate();
    void* nodeMemoryBlock = nodePool.allocate();

    // Creater new order and order node
    Order* newOrder = new (orderMemoryBlock) Order(orderMemoryBlock, orderID, userID, side, ticker, quantity, price);
    OrderNode* orderNode = new (nodeMemoryBlock) OrderNode(nodeMemoryBlock, newOrder);

    // Insert into queue
    buyQueue.push(orderNode);
}