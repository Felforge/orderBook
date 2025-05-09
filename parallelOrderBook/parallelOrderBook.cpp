#include <iostream>
#include "parallelOrderBook.h"
using namespace std;

// Constructor for Buy Node
// This is a node of the doubly linked list
BuyRequest::BuyRequest(void* memoryBlock, OrderNode* orderNode, void* levelBlock) 
    : memoryBlock(memoryBlock), orderNode(orderNode), levelBlock(levelBlock) {
        prev.store(nullptr);
        next.store(nullptr);
    }

// Constructor for ID Node
// This is a node of the doubly linked list
RemoveRequest::RemoveRequest(void* memoryBlock, int ID) 
    : memoryBlock(memoryBlock), ID(ID) {
        prev.store(nullptr);
        next.store(nullptr);
    }

// Constructor for Ticker
Ticker::Ticker(void* memoryBlock, string ticker, int numLevels) 
    : memoryBlock(memoryBlock), ticker(ticker), bestBuyOrder(nullptr), bestSellOrder(nullptr) {
        for (int i = 0; i < numLevels; i++) {
            buyOrderList.push_back(nullptr);
        }
        for (int i = 0; i < numLevels; i++) {
            sellOrderList.push_back(nullptr);
        }
        for (int i = 0; i < numLevels; i++) {
            activeLevels.push_back(false);
        }
    }

// Constructor
// numTickers is the maximum number of tickers that could be added
OrderBook::OrderBook(int numTickers, int numOrders, double inpMinPrice, double inpMaxPrice) 
    // Declare memory pool
    : orderPool(sizeof(Order), numOrders), nodePool(sizeof(OrderNode), numOrders), priceLevelPool(sizeof(OrderList), 2 * numTickers * int((inpMaxPrice - minPrice) * 100.0)), 
    tickerPool(sizeof(Ticker), numTickers), buyRequestPool(sizeof(BuyRequest), numOrders), removeRequestPool(sizeof(RemoveRequest), numOrders) {

    // Declare max number of tickers
    maxTickers = numTickers;

    // Declare max number of orders
    maxOrders = numOrders;

    // Declare prices and numLevels
    minPrice = inpMinPrice;
    maxPrice = inpMaxPrice;
    numLevels = int((maxPrice - minPrice) * 100.0);

    // Create elements in orders
    for (int i = 0; i < numOrders; i++) {
        orders.push_back(nullptr);
    }

    // Declare ID counter
    orderID = 0;

    // Start threads
    running = true;
    startup();
}

// Destructor
// Orders and OrderNodes are deallocated elsewhere
OrderBook::~OrderBook() {
    // Stop threads
    shutdown();

    // Delete every price level pointer and ticker pointer
    for (const auto& pair: tickerMap) {
        for (OrderList* buyPtr: pair.second->buyOrderList) {
            if (buyPtr) {
                priceLevelPool.deallocate(buyPtr->memoryBlock);
            }
        }
        for (OrderList* sellPtr: pair.second->sellOrderList) {
            if (sellPtr) {
                priceLevelPool.deallocate(sellPtr->memoryBlock);
            }
        }
        tickerPool.deallocate(pair.second->memoryBlock);
    }
}

// Start threads
void OrderBook::startup() {
    for (int i = 0; i < MAX_THREADS; i++) {
        threads.emplace_back(receiveRequests);
    }
}

// Stop threads
void OrderBook::shutdown() {
    running = false;
    for (auto& thread : threads) {
        thread.join();
    }
}

// Recieve requests
void OrderBook::receiveRequests() {
    while (running) {
        BuyRequest* buyRequest = buyQueue.remove();
        if (buyRequest) {
            processBuy(buyRequest);
            continue;  
        }
        RemoveRequest* sellRequest = sellQueue.remove();
        if (sellRequest) {
            // Process sell
            continue;
        }
        // No task available, yield to other threads
        this_thread::yield();
    }
}

void OrderBook::addTicker(string ticker) {
    if (tickerMap.size() == maxTickers) {
        cout << "Order Book Error: Too Many Tickers" << endl;
        return;
    }
    void* memoryBlock = tickerPool.allocate();
    tickerMap[ticker] = new (memoryBlock) Ticker(memoryBlock, ticker, numLevels);
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
    } else if (price > maxPrice) {
        cout << "Order Book Error: Requested Price is Above Maximum" << endl;
        return;
    } else if (price < minPrice) {
        cout << "Order Book Error: Requested Price is Below Minimum" << endl;
        return;
    }

    // Allocate order and order node
    void* orderMemoryBlock = orderPool.allocate();
    void* nodeMemoryBlock = nodePool.allocate();

    // Creater new order and order node
    Order* newOrder = new (orderMemoryBlock) Order(orderMemoryBlock, orderID, userID, side, ticker, quantity, price);
    OrderNode* orderNode = new (nodeMemoryBlock) OrderNode(nodeMemoryBlock, newOrder);

    void* levelBlock = nullptr;
    if (!tickerMap[ticker]->activeLevels[getListIdx(price)]) {
        void* levelBlock = priceLevelPool.allocate();
        tickerMap[ticker]->activeLevels[getListIdx(price)] = true;
    }

    // Insert into queue
    void* memoryBlock = buyRequestPool.allocate();
    BuyRequest* buyRequest = new (memoryBlock) BuyRequest(memoryBlock, orderNode, levelBlock);
    buyQueue.insert(buyRequest);

    // Increment orderID
    orderID++;

    // Print Order Data
    if (print) {
        cout << "Order successfully added:" << endl
             << "  Type: " << side << endl
             << "  Quantity: " << quantity << endl
             << "  Ticker: " << ticker << endl
             << "  Price: " << price << endl
             << "  Order ID: " << orderID << endl;
    }
}

// Worker thread function to process buy requests
void OrderBook::processBuy(BuyRequest* nodePtr) {
    Order* orderPtr = nodePtr->orderNode->order;
    string side = orderPtr->side;
    int listIdx = getListIdx(orderPtr->price);
    Ticker* tickerPtr = tickerMap[orderPtr->ticker];
    void* levelBlock = nodePtr->levelBlock;
    
    if (orderPtr->side == "BUY") {
        // Create price level if needed
        if (levelBlock) {
            tickerPtr->buyOrderList[listIdx] = new (levelBlock) OrderList(levelBlock, orderPool, nodePool);
        }
        tickerPtr->buyOrderList[listIdx]->insert(nodePtr->orderNode);
    } else { // orderPtr->side == "SELL"
        // Create price level if needed
        if (levelBlock) {
            tickerPtr->sellOrderList[listIdx] = new (levelBlock) OrderList(levelBlock, orderPool, nodePool);
        }
        tickerPtr->sellOrderList[listIdx]->insert(nodePtr->orderNode);
    }
    buyRequestPool.deallocate(nodePtr->memoryBlock);
}

void OrderBook::removeOrder(int id, bool print) {
    // Return if order ID is invalid
    if (id >= orderID) {
        cout << "Order Book Error: Invalid Order ID" << endl;
        return;
    }

    // Insert into queue
    void* memoryBlock = removeRequestPool.allocate();
    RemoveRequest* request = new (memoryBlock) RemoveRequest(memoryBlock, id);
    sellQueue.insert(request);

    // Print Success Message
    if (print) {
        cout << "Order successfully removed:" << endl
             << "  Order ID: " << id << endl;
    }
}

// Worker thread function to process sell requests
// void OrderBook::processSell(RemoveRequest* nodePtr) {
//     // Remove node
//     OrderNode* orderNode = orderMap[nodePtr->ID];
//     Order* orderPtr = orderNode->order;
//     if (orderPtr->side == "BUY") {
//         tickerMap[orderPtr->ticker]->buyOrderMap[orderPtr->price]->remove(orderNode);
//     } else { // orderPtr->side == "SELL"
//         tickerMap[orderPtr->ticker]->sellOrderMap[orderPtr->price]->remove(orderNode);
//     }

//     // Delete request
//     removeRequestPool.deallocate(nodePtr->memoryBlock);
// }

int OrderBook::getListIdx(double price) {
    return int((price - minPrice) * 100);
}

// Find a way to eliminate the order map
// It can cause a bottleneck as it is not lock free