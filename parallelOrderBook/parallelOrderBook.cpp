#include <iostream>
#include <unistd.h>
#include "parallelOrderBook.h"
using namespace std;

// Constructor for Buy Node
// This is a node of the doubly linked list
BuyRequest::BuyRequest(void* memoryBlock, OrderNode* orderNode, void* levelBlock, void* queueBlock) 
    : memoryBlock(memoryBlock), orderNode(orderNode), levelBlock(levelBlock), queueBlock(queueBlock) {
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
    : memoryBlock(memoryBlock), ticker(ticker), priorityBuyPrices(), prioritySellPrices(true) {
        bestBuyIdx.store(-1);
        bestSellIdx.store(-1);
        for (int i = 0; i < numLevels; i++) {
            buyOrderList.push_back(nullptr);
        }
        for (int i = 0; i < numLevels; i++) {
            sellOrderList.push_back(nullptr);
        }
        for (int i = 0; i < numLevels; i++) {
            activeBuyLevels.push_back(false);
        }
        for (int i = 0; i < numLevels; i++) {
            activeSellLevels.push_back(false);
        }
    }

// Constructor
// numTickers is the maximum number of tickers that could be added
OrderBook::OrderBook(int numTickers, int numOrders, double inpMinPrice, double inpMaxPrice) 
    // Declare memory pool
    : orderPool(sizeof(Order), numOrders), nodePool(sizeof(OrderNode), numOrders), priceLevelPool(sizeof(OrderList), 2 * numTickers * int((inpMaxPrice - inpMinPrice) * 100.0)), 
      tickerPool(sizeof(Ticker), numTickers), buyRequestPool(sizeof(BuyRequest), 10 * numOrders), removeRequestPool(sizeof(RemoveRequest), numOrders), 
      priceQueuePool(sizeof(Node<int>), 2 * numTickers * int((inpMaxPrice - inpMinPrice) * 100.0)) {

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

    // Count number of processed orders
    ordersProcessed.store(0);

    // Start threads
    running = true;
    startup();
}

// Destructor
// Orders and OrderNodes are deallocated elsewhere
OrderBook::~OrderBook() {
    // Stop threads
    shutdown();

    // DEALLOCATE QUEUES

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

        Node<int>* current = pair.second->priorityBuyPrices.tail.load();
        if (current) {
            while (current != pair.second->priorityBuyPrices.head.load()) {
                Node<int>* prev = current->prev.load();
                priceQueuePool.deallocate(current->memoryBlock);
                current = prev;
            }
            priceQueuePool.deallocate(current->memoryBlock);
        }

        current = pair.second->prioritySellPrices.tail.load();
        if (current) {
            while (current != pair.second->prioritySellPrices.head.load()) {
                Node<int>* prev = current->prev.load();
                priceQueuePool.deallocate(current->memoryBlock);
                current = prev;
            }
            priceQueuePool.deallocate(current->memoryBlock);
        }

        tickerPool.deallocate(pair.second->memoryBlock);
    }


}

// Start threads
void OrderBook::startup() {
    for (int i = 0; i < MAX_THREADS; i++) {
        threads.emplace_back([this]() { receiveRequests(); });
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
            processSell(sellRequest);
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
    } else if (orderID >= maxOrders) {
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
    void* queueBlock = nullptr;
    if (side == "BUY" && !tickerMap[ticker]->activeBuyLevels[getListIdx(price)] || side == "SELL" && !tickerMap[ticker]->activeSellLevels[getListIdx(price)]) {
        levelBlock = priceLevelPool.allocate();
        queueBlock = priceQueuePool.allocate();
        if (side == "BUY") {
            tickerMap[ticker]->activeBuyLevels[getListIdx(price)] = true;
        } else { // side == "SELL"
            tickerMap[ticker]->activeSellLevels[getListIdx(price)] = true;
        }
    }

    // Insert into queue
    void* memoryBlock = buyRequestPool.allocate();
    BuyRequest* buyRequest = new (memoryBlock) BuyRequest(memoryBlock, orderNode, levelBlock, queueBlock);
    buyQueue.insert(buyRequest);

    // Store order
    orders[orderID] = orderNode;

    // Print Order Data
    if (print) {
        cout << "Order successfully added:" << endl
             << "  Type: " << side << endl
             << "  Quantity: " << quantity << endl
             << "  Ticker: " << ticker << endl
             << "  Price: " << price << endl
             << "  Order ID: " << orderID << endl;
    }

    cout << ordersProcessed.load() << " " << buyQueue.head.load() << endl;

    // Increment orderID
    orderID++;
}

// Worker thread function to process buy requests
void OrderBook::processBuy(BuyRequest* nodePtr) {
    Order* orderPtr = nodePtr->orderNode->order;
    string side = orderPtr->side;
    int listIdx = getListIdx(orderPtr->price);
    Ticker* tickerPtr = tickerMap[orderPtr->ticker];
    void* levelBlock = nodePtr->levelBlock;
    void* queueBlock = nodePtr->queueBlock;
    bool addToQueue = true;
    
    if (orderPtr->side == "BUY") {

        // Create price level if needed
        if (levelBlock) {
            tickerPtr->buyOrderList[listIdx] = new (levelBlock) OrderList(levelBlock, orderPool, nodePool);

            // Change best buy if needed
            int expectedIdx = tickerPtr->bestBuyIdx.load();
            if (listIdx > expectedIdx) {
                while (listIdx > expectedIdx) {
                    if (tickerPtr->bestBuyIdx.compare_exchange_weak(expectedIdx, listIdx)) {
                        addToQueue = false;
                        break;
                    }
                    expectedIdx = tickerPtr->bestBuyIdx.load();
                }
            }
            Node<int>* priceNode = new (queueBlock) Node(listIdx, queueBlock);
            tickerPtr->priorityBuyPrices.insert(priceNode);
        }

        // Insert order
        tickerPtr->buyOrderList[listIdx]->insert(nodePtr->orderNode);

    } else { // orderPtr->side == "SELL"
        // Create price level if needed
        if (levelBlock) {
            tickerPtr->sellOrderList[listIdx] = new (levelBlock) OrderList(levelBlock, orderPool, nodePool);

            // Change best buy if needed
            int expectedIdx = tickerPtr->bestSellIdx.load();
            if (expectedIdx == 1 || listIdx < expectedIdx) {
                while (expectedIdx == 1 || listIdx < expectedIdx) {
                    if (tickerPtr->bestSellIdx.compare_exchange_weak(expectedIdx, listIdx)) {
                        addToQueue = false;
                        break;
                    }
                    expectedIdx = tickerPtr->bestSellIdx.load();
                }
            } else { // listIdx >= expectedIdx
                Node<int>* priceNode = new (queueBlock) Node(listIdx, queueBlock);
                tickerPtr->prioritySellPrices.insert(priceNode);
            }
        }

        // Insert order
        tickerPtr->sellOrderList[listIdx]->insert(nodePtr->orderNode);

    }
    buyRequestPool.deallocate(nodePtr->memoryBlock);

    // Count buy orders as they are processed
    ordersProcessed.fetch_add(1, std::memory_order_relaxed);
}

void OrderBook::removeOrder(int id, bool print) {
    // Return if order ID is invalid
    if (id >= orderID) {
        cout << "Order Book Error: Invalid Order ID" << endl;
        return;
    } else if (!orders[id]) {
        cout << "Order Book Error: Order Does Not Exist" << endl;
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
void OrderBook::processSell(RemoveRequest* nodePtr) {
    OrderNode* orderNodePtr = orders[nodePtr->ID];
    int listIdx = getListIdx(orderNodePtr->order->price);
    string side = orderNodePtr->order->side;
    string ticker = orderNodePtr->order->ticker;
    Ticker* tickerPtr = tickerMap[ticker];

    cout << "here" << endl;

    bool deleteLevel = !orderNodePtr->prev.load() && !orderNodePtr->next.load();

    // Remove node
    if (side == "BUY") {
        tickerMap[ticker]->buyOrderList[listIdx]->remove(orderNodePtr);
    } else { // side == "SELL"
        tickerMap[ticker]->sellOrderList[listIdx]->remove(orderNodePtr);
    }
    orders[nodePtr->ID] = nullptr;

    // Clear memory
    if (deleteLevel) {
        if (side == "BUY") {
            priceLevelPool.deallocate(tickerMap[ticker]->buyOrderList[listIdx]);
            tickerMap[ticker]->buyOrderList[listIdx] = nullptr;
            tickerPtr->activeBuyLevels[listIdx] = false;
            if (listIdx == tickerMap[ticker]->bestBuyIdx) {
                updateBestIdx(tickerPtr->priorityBuyPrices, tickerPtr->activeBuyLevels, tickerPtr->bestBuyIdx);
            }
        } else if (side == "SELL") {
            priceLevelPool.deallocate(tickerMap[ticker]->sellOrderList[listIdx]);
            tickerMap[ticker]->sellOrderList[listIdx] = nullptr;
            tickerPtr->activeSellLevels[listIdx] = false;
            if (listIdx == tickerMap[ticker]->bestSellIdx) {
                updateBestIdx(tickerPtr->prioritySellPrices, tickerPtr->activeSellLevels, tickerPtr->bestSellIdx);
            }
        }
    }

    // Delete request
    removeRequestPool.deallocate(nodePtr->memoryBlock);
}

int OrderBook::getListIdx(double price) {
    return int((price - minPrice) * 100);
}

void OrderBook::updateBestIdx(PriorityQueue &queue, vector<bool> &activeLevels, atomic<int> &bestIdx) {
    // Remove first
    queue.remove();

    Node<int>* current = queue.remove();
    while (current && !activeLevels[current->data]) {
        priceQueuePool.deallocate(current->memoryBlock);
        current = queue.remove();
    }
    if (current) {
        bestIdx.store(current->data);
        priceQueuePool.deallocate(current->memoryBlock);
    } else {
        bestIdx.store(-1);
    }
}

// MAKE SURE DESTRUCTOR IS BEING EXPLICITELY CALLED WHERE NEEDED BEFORE DEALLOCATION
// IF NOT THERE COULD BE BIG MEMORY LEAKS

// To-do:
// Do test cases
// Get rid of OrderNode and just use template node?
// Add a way to reuse order IDs or at least spaces in orders

// int main() {
//     OrderBook orderBook(1, 10, 50.0, 150.0);
//     orderBook.addTicker("AAPL");
//     orderBook.addOrder(1, "AAPL", "BUY", 10, 100.0);
//     usleep(2000000);
//     orderBook.removeOrder(0);
//     return 0;
// }