#include <thread>
#include <atomic>
#include "parallelOrderBook.h"
using namespace std;

// Order ID Counter
atomic<int> orderID = 0;

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

// Constructor for AddOrderRequest
AddOrderRequest::AddOrderRequest(Order* order, bool print)
    : order(order), print(print) {}

// Constructor for RemoveOrderRequest
RemoveOrderRequest::RemoveOrderRequest(int orderID, bool print)
    : orderID(orderID), print(print) {}

// Worker Thread Constructor
WorkerThread::WorkerThread(int numTickers, int numOrders)
    // Declare memory pool
    : orderPool(sizeof(Order), numOrders), nodePool(sizeof(OrderNode), numOrders),
      priceLevelPool(sizeof(PriceLevel), 2 * numTickers * MAX_PRICE_IDX), tickerPool(sizeof(Ticker), numTickers) {

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
    // Stop the worker thread and join it
    stop = true;
    if (worker.joinable()) {
        worker.join();
    }
}

int WorkerThread::getListIndex(double price) {
    return int(price * 100.0) - 1;
}

// Create Add Order Request to be Processed
void WorkerThread::requestAddOrder(int userID, std::string ticker, std::string side, int quantity, double price, bool print) {
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
    } else if (price > double(MAX_PRICE_IDX / 100)) {
        cout << "Order Book Error: Maximum Available Price is " << MAX_PRICE_IDX / 100 << endl;
        return;
    } else if (tickerMap.find(ticker) == tickerMap.end()) {
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
            removeOrder(request.orderID, request.print);
        }
        while(!tickerQueue.empty()) {
            string ticker = tickerQueue.front();
            tickerQueue.pop();
            addTicker(ticker);
        }
    }
}
