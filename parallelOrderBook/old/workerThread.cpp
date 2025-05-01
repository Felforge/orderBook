#include <thread>
#include "parallelOrderBook.h"
using namespace std;

// Get place in list based on price
// int getListIndex(double price) {
//     return int(price * 100.0) - 1;
// }

// Constructor for AddOrderRequest
AddOrderRequest::AddOrderRequest(Order* order, bool print)
    : order(order), print(print) {}

// Constructor for RemoveOrderRequest
RemoveOrderRequest::RemoveOrderRequest(int orderID, bool print)
    : orderID(orderID), print(print) {}

// Worker Thread Constructor
WorkerThread::WorkerThread(void* inpMemoryBlock, int numTickers, int numOrders)
    // Declare memory pool
    : orderPool(sizeof(Order), numOrders), nodePool(sizeof(OrderNode), numOrders),
      priceLevelPool(sizeof(PriceLevel), 2 * numTickers * MAX_PRICE_IDX), tickerPool(sizeof(Ticker), numTickers) {

    // Save memory location
    memoryBlock = inpMemoryBlock;

    // Initiate maxTickers
    maxTickers = numTickers;

    // Initiate maxOrders
    maxOrders = numOrders;
    
    // Start worker thread
    stop = false;
    worker = thread(&WorkerThread::processRequests, this);
}

// Worker Thread Destructor
WorkerThread::~WorkerThread() {
    // Delete every order pointer
    for (const auto& pair: orderMap) {
        orderPool.deallocate(pair.second->order->memoryBlock);
    }

    // Delete every price level pointer and ticker pointer
    for (const auto& pair: tickerMap) {
        for (int i = 0; i < MAX_PRICE_IDX; i++) {
            if (pair.second->buyOrderList[i] != nullptr) {
                while (pair.second->buyOrderList[i]->tail != pair.second->buyOrderList[i]->head) {
                    OrderNode* temp = pair.second->buyOrderList[i]->tail;
                    pair.second->buyOrderList[i]->tail = temp->prev;
                    nodePool.deallocate(temp->memoryBlock);
                }
                nodePool.deallocate(pair.second->buyOrderList[i]->head->memoryBlock);
                priceLevelPool.deallocate(pair.second->buyOrderList[i]->memoryBlock);
            }
            if (pair.second->sellOrderList[i] != nullptr) {
                while (pair.second->sellOrderList[i]->tail != pair.second->sellOrderList[i]->head) {
                    OrderNode* temp = pair.second->sellOrderList[i]->tail;
                    pair.second->sellOrderList[i]->tail = temp->prev;
                    nodePool.deallocate(temp->memoryBlock);
                }
                nodePool.deallocate(pair.second->sellOrderList[i]->head->memoryBlock);
                priceLevelPool.deallocate(pair.second->sellOrderList[i]->memoryBlock);
            }
        }
        tickerPool.deallocate(pair.second->memoryBlock);
    }

    // Stop the worker thread and join it
    stop = true;
    if (worker.joinable()) {
        worker.join();
    }
}

// Create Add Ticker Request to be Processed
void WorkerThread::requestAddTicker(string ticker) {
    // Return if too many tickers
    if (tickerMap.size() == maxTickers) {
        cout << "Order Book Error: Too Many Tickers" << endl;
        return;
    }
    tickerQueue.push(ticker);
}

// Allocate space and add ticker
void WorkerThread::addTicker(string ticker) {
    void* memoryBlock = tickerPool.allocate();
    tickerMap[ticker] = new (memoryBlock) Ticker(memoryBlock, ticker);
}

// Create Add Order Request to be Processed
void WorkerThread::requestAddOrder(int userID, int orderID, std::string ticker, std::string side, int quantity, double price, bool print) {
    // Check for valid ticker
    if (tickerMap.find(ticker) == tickerMap.end()) {
        cout << "Order Book Error: Ticker is Invalid" << endl;
        return;
    }

    void* memoryBlock = orderPool.allocate();
    Order* newOrder = new (orderPool.allocate()) Order(memoryBlock, orderID, userID, side, ticker, quantity, price);
    addQueue.push(AddOrderRequest(newOrder, print));
}

// Add New Order to Data
void WorkerThread::addOrder(Order* order, bool print) {

    // Create order variables
    string ticker = order->ticker;
    string side = order->side;
    int price = order->price;
    int quantity = order->quantity;
    int orderID = order->orderID;

    // Get index for price level
    int listIdx = getListIndex(price);

    // Allocate order node and create object
    void* nodeMemoryBlock = nodePool.allocate();
    OrderNode* orderNode = new (nodeMemoryBlock) OrderNode(nodeMemoryBlock, order);

    // Insert order into order map
    orderMap[orderID] = orderNode;

    if (side == "BUY") {
        if (tickerMap[ticker]->buyOrderList[listIdx] == nullptr) {
            void* priceLevelMemoryBlock = priceLevelPool.allocate();
            tickerMap[ticker]->buyOrderList[listIdx] = new (priceLevelMemoryBlock) PriceLevel(priceLevelMemoryBlock, orderNode);
        } else {
            orderNode->prev = tickerMap[ticker]->buyOrderList[listIdx]->tail;
            tickerMap[ticker]->buyOrderList[listIdx]->tail->next = orderNode;
            tickerMap[ticker]->buyOrderList[listIdx]->tail = orderNode;
        }
    } else { // side == "SELL"
        if (tickerMap[ticker]->sellOrderList[listIdx] == nullptr) {
            void* priceLevelMemoryBlock = priceLevelPool.allocate();
            tickerMap[ticker]->sellOrderList[listIdx] = new (priceLevelMemoryBlock) PriceLevel(priceLevelMemoryBlock, orderNode);
        } else {
            orderNode->prev = tickerMap[ticker]->sellOrderList[listIdx]->tail;
            tickerMap[ticker]->sellOrderList[listIdx]->tail->next = orderNode;
            tickerMap[ticker]->sellOrderList[listIdx]->tail = orderNode;
        }
    }

    // Print Order Data
    if (print) {
        cout << "Order successfully added:" << endl
             << "  Type: " << side << endl
             << "  Quantity: " << quantity << endl
             << "  Ticker: " << ticker << endl
             << "  Price: " << price << endl
             << "  Order ID: " << orderID << endl;
    }

    // Update counter for order ID
    orderID++;
}

// matchOrders will run a function like below that will either run till a given count
// or till no matches remain. It will be called on a thread like this one.
// Function to process requests
void WorkerThread::processRequests() {
    while (!stop) {
        while (!addQueue.empty()) {
            AddOrderRequest request = addQueue.front();
            addQueue.pop();
            addOrder(request.order, request.print);
        }
        while (!removeQueue.empty()) {
            RemoveOrderRequest request = removeQueue.front();
            removeQueue.pop();
            // removeOrder(request.orderID, request.print);
        }
        while(!tickerQueue.empty()) {
            string ticker = tickerQueue.front();
            tickerQueue.pop();
            addTicker(ticker);
        }
    }
}