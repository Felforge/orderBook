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
    : orderPool(sizeof(Order), numOrders), nodePool(sizeof(OrderNode), numOrders),
      priceLevelPool(sizeof(OrderList), 2 * numTickers * NUM_LEVELS), tickerPool(sizeof(Ticker), numTickers) {

    // Declare max number of tickers
    maxTickers = numTickers;

    // Declare max number of orders
    maxOrders = numOrders;

    // Declare ID counter
    orderID.store(0);
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

    // // Allocate memory for order, node and priceLevel
    // void* orderMemoryBlock = orderPool.allocate();
    // void* nodeMemoryBlock = nodePool.allocate();

    // // Create new order and node
    // Order* newOrder = new (orderMemoryBlock) Order(orderMemoryBlock, orderID, userID, side, ticker, quantity, price);
    // OrderNode* orderNode = new (nodeMemoryBlock) OrderNode(nodeMemoryBlock, newOrder);

    // // Insert order into order map
    // orderMap[orderID] = orderNode;

    // if (side == "BUY") {
    //     if (tickerMap[ticker]->buyOrderMap.find(price) == tickerMap[ticker]->buyOrderMap.end()) {
    //         void* priceLevelMemoryBlock = priceLevelPool.allocate();
    //         tickerMap[ticker]->buyOrderMap[price] = new (priceLevelMemoryBlock) PriceLevel(priceLevelMemoryBlock, orderNode);
    //         if (tickerMap[ticker]->bestBuyOrder == nullptr || price > tickerMap[ticker]->bestBuyOrder->head->order->price) {
    //             tickerMap[ticker]->bestBuyOrder = tickerMap[ticker]->buyOrderMap[price];
    //         }
    //     } else {
    //         orderNode->prev = tickerMap[ticker]->buyOrderMap[price]->tail;
    //         tickerMap[ticker]->buyOrderMap[price]->tail->next = orderNode;
    //         tickerMap[ticker]->buyOrderMap[price]->tail = orderNode;
    //     }
    //     // Mark price level as active
    //     tickerMap[ticker]->priorityBuyPrices.push(price);
    // } else { // side == "SELL"
    //     if (tickerMap[ticker]->sellOrderMap.find(price) == tickerMap[ticker]->sellOrderMap.end()) {
    //         void* priceLevelMemoryBlock = priceLevelPool.allocate();
    //         tickerMap[ticker]->sellOrderMap[price] = new (priceLevelMemoryBlock) PriceLevel(priceLevelMemoryBlock, orderNode);
    //         if (tickerMap[ticker]->bestSellOrder == nullptr || price < tickerMap[ticker]->bestSellOrder->head->order->price) {
    //             tickerMap[ticker]->bestSellOrder = tickerMap[ticker]->sellOrderMap[price];
    //         }
    //     } else {
    //         orderNode->prev = tickerMap[ticker]->sellOrderMap[price]->tail;
    //         tickerMap[ticker]->sellOrderMap[price]->tail->next = orderNode;
    //         tickerMap[ticker]->sellOrderMap[price]->tail = orderNode;
    //     }
    //     // Mark price level as active
    //     tickerMap[ticker]->prioritySellPrices.push(price);
    // }

    // // Print Order Data
    // if (print) {
    //     cout << "Order successfully added:" << endl
    //          << "  Type: " << side << endl
    //          << "  Quantity: " << quantity << endl
    //          << "  Ticker: " << ticker << endl
    //          << "  Price: " << price << endl
    //          << "  Order ID: " << orderID << endl;
    // }

    // // Update counter for order ID
    // orderID++;
}