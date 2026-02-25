#include <iostream>
#include "cpuOrderBook.h"
using namespace std;

const int NUM_LEVELS = 100000;

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
    : memoryBlock(memoryBlock), ticker(ticker), bestBuyOrder(nullptr), bestSellOrder(nullptr) {}

// Constructor
// numTickers is the maximum number of tickers that could be added
OrderBook::OrderBook(int numTickers, int numOrders) 
    // Declare memory pool
    : orderPool(sizeof(Order), numOrders), nodePool(sizeof(OrderNode), numOrders),
      priceLevelPool(sizeof(PriceLevel), 2 * numTickers * NUM_LEVELS), tickerPool(sizeof(Ticker), numTickers) {

    // Declare max number of tickers
    maxTickers = numTickers;

    // Declare max number of orders
    maxOrders = numOrders;

    // Declare ID counter
    orderID = 0;
}

// Destructor
OrderBook::~OrderBook() {
    // Delete every order pointer
    for (const auto& pair: orderMap) {
        orderPool.deallocate(pair.second->order->memoryBlock);
    }

    // Delete every price level pointer and ticker pointer
    for (const auto& pair: tickerMap) {
        for (const auto& buyPair: pair.second->buyOrderMap) {
            while (buyPair.second->tail != buyPair.second->head) {
                OrderNode* temp = buyPair.second->tail;
                buyPair.second->tail = temp->prev;
                nodePool.deallocate(temp->memoryBlock);
            }
            nodePool.deallocate(buyPair.second->head->memoryBlock);
            priceLevelPool.deallocate(buyPair.second->memoryBlock);
        }
        for (const auto& sellPair: pair.second->sellOrderMap) {
            while (sellPair.second->tail != sellPair.second->head) {
                OrderNode* temp = sellPair.second->tail;
                sellPair.second->tail = temp->prev;
                nodePool.deallocate(temp->memoryBlock);
            }
            nodePool.deallocate(sellPair.second->head->memoryBlock);
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

// Update best buy order
void OrderBook::updateBestBuyOrder(string ticker) {
    auto& priorities = tickerMap[ticker]->priorityBuyPrices;
    auto& buyMap = tickerMap[ticker]->buyOrderMap;

    // Pop old best price
    priorities.pop();

    while (!priorities.empty()) {
        double bestBuyPrice = priorities.top();
        if (buyMap.find(bestBuyPrice) != buyMap.end()) {
            tickerMap[ticker]->bestBuyOrder = tickerMap[ticker]->buyOrderMap[bestBuyPrice];
            return;
        }
        // Remove inactive price lvel
        priorities.pop();
    }

    // No active buy orders
    tickerMap[ticker]->bestBuyOrder = nullptr;
}

// Updated best sell order
void OrderBook::updateBestSellOrder(string ticker) {
    auto& priorities = tickerMap[ticker]->prioritySellPrices;
    auto& sellMap = tickerMap[ticker]->sellOrderMap;

    // Pop old best price
    priorities.pop();

    while (!priorities.empty()) {
        double bestSellPrice = priorities.top();
        if (sellMap.find(bestSellPrice) != sellMap.end()) {
            tickerMap[ticker]->bestSellOrder = tickerMap[ticker]->sellOrderMap[bestSellPrice];
            return;
        }
        // Remove inactive price lvel
        priorities.pop();
    }
    // No active buy orders
    tickerMap[ticker]->bestSellOrder = nullptr;
}

// Handle removing price level
void OrderBook::removePriceLevel(std::string side, std::string ticker, double price, PriceLevel* levelPtr) {
    if (side == "BUY") {
        tickerMap[ticker]->buyOrderMap.erase(price);
        if (levelPtr == tickerMap[ticker]->bestBuyOrder) {
            updateBestBuyOrder(ticker);
        }
    } else { // side == "SELL"
        tickerMap[ticker]->sellOrderMap.erase(price);
        if (levelPtr == tickerMap[ticker]->bestSellOrder) {
            updateBestSellOrder(ticker);
        }
    }
    priceLevelPool.deallocate(levelPtr->memoryBlock);
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

    // Allocate memory for order, node and priceLevel
    void* orderMemoryBlock = orderPool.allocate();
    void* nodeMemoryBlock = nodePool.allocate();

    // Create new order and node
    Order* newOrder = new (orderMemoryBlock) Order(orderMemoryBlock, orderID, userID, side, ticker, quantity, price);
    OrderNode* orderNode = new (nodeMemoryBlock) OrderNode(nodeMemoryBlock, newOrder);

    // Insert order into order map
    orderMap[orderID] = orderNode;

    if (side == "BUY") {
        if (tickerMap[ticker]->buyOrderMap.find(price) == tickerMap[ticker]->buyOrderMap.end()) {
            void* priceLevelMemoryBlock = priceLevelPool.allocate();
            tickerMap[ticker]->buyOrderMap[price] = new (priceLevelMemoryBlock) PriceLevel(priceLevelMemoryBlock, orderNode);
            if (tickerMap[ticker]->bestBuyOrder == nullptr || price > tickerMap[ticker]->bestBuyOrder->head->order->price) {
                tickerMap[ticker]->bestBuyOrder = tickerMap[ticker]->buyOrderMap[price];
            }
        } else {
            orderNode->prev = tickerMap[ticker]->buyOrderMap[price]->tail;
            tickerMap[ticker]->buyOrderMap[price]->tail->next = orderNode;
            tickerMap[ticker]->buyOrderMap[price]->tail = orderNode;
        }
        // Mark price level as active
        tickerMap[ticker]->priorityBuyPrices.push(price);
    } else { // side == "SELL"
        if (tickerMap[ticker]->sellOrderMap.find(price) == tickerMap[ticker]->sellOrderMap.end()) {
            void* priceLevelMemoryBlock = priceLevelPool.allocate();
            tickerMap[ticker]->sellOrderMap[price] = new (priceLevelMemoryBlock) PriceLevel(priceLevelMemoryBlock, orderNode);
            if (tickerMap[ticker]->bestSellOrder == nullptr || price < tickerMap[ticker]->bestSellOrder->head->order->price) {
                tickerMap[ticker]->bestSellOrder = tickerMap[ticker]->sellOrderMap[price];
            }
        } else {
            orderNode->prev = tickerMap[ticker]->sellOrderMap[price]->tail;
            tickerMap[ticker]->sellOrderMap[price]->tail->next = orderNode;
            tickerMap[ticker]->sellOrderMap[price]->tail = orderNode;
        }
        // Mark price level as active
        tickerMap[ticker]->prioritySellPrices.push(price);
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

void OrderBook::removeOrder(int id, bool print) {
    // Return if order ID is invalid
    if (id >= orderID || orderMap.find(id) == orderMap.end()) {
        cout << "Order Book Error: Invalid Order ID" << endl;
        return;
    }


    // Create pointer to node of order
    OrderNode* nodePtr = orderMap[id];

    // Get ticker
    string ticker = nodePtr->order->ticker;

    // Get side of order
    string side = nodePtr->order->side;

    // Get order price
    double price = nodePtr->order->price;

    // Flag for deleteing level
    bool deleteLevel = false;
    PriceLevel* levelPtr = (side == "BUY") ? tickerMap[ticker]->buyOrderMap[price] : tickerMap[ticker]->sellOrderMap[price];

    if (nodePtr->prev == nullptr && nodePtr->next == nullptr) {
        deleteLevel = true; // Last order in price level is being removed
    } else if (nodePtr->prev == nullptr) {
        levelPtr->head = nodePtr->next; // Head order is being removed
        nodePtr->next->prev = nullptr;
    } else if (nodePtr->next == nullptr) {
        levelPtr->tail = nodePtr->prev; // Tail order is being removed
        nodePtr->prev->next = nullptr;
    } else {
        nodePtr->prev->next = nodePtr->next;
        nodePtr->next->prev = nodePtr->prev;
    }

    // Always has to get deleted so it is outside the if statement
    // Scary memory leak if done wrong

    // Delete node and order
    orderPool.deallocate(nodePtr->order->memoryBlock);
    nodePool.deallocate(nodePtr->memoryBlock);

    // Delete price level object and reassign best order if applicable
    if (deleteLevel) {
        removePriceLevel(side, ticker, price, levelPtr);
    }

    // Erase ID from orderMap
    orderMap.erase(id);

    // Print Success Message
    if (print) {
        cout << "Order successfully removed:" << endl
             << "  Order ID: " << id << endl;
    }
}

void OrderBook::matchOrders(string ticker, bool print, int count) {
    if (tickerMap.find(ticker) == tickerMap.end()) {
        cout << "Order Book Error: Ticker is Invalid" << endl;
        return;
    } else if (!tickerMap[ticker]->bestBuyOrder || !tickerMap[ticker]->bestSellOrder) {
        cout << "Order Book Error: No Orders To Be Matched" << endl;
        return;
    }
    int numCount = 0;
    Order* bestBuy = tickerMap[ticker]->bestBuyOrder->head->order;
    Order* bestSell = tickerMap[ticker]->bestSellOrder->head->order;
    while (bestBuy->price >= bestSell->price) {

        // For testing purposes
        numCount++; 
        if (count != 0 && numCount >= count) {
            break;
        }
        // Get matched order price and quantity
        double orderPrice = tickerMap[ticker]->bestSellOrder->head->order->price;
        int orderQuantity = min(bestBuy->quantity, bestSell->quantity);
        if (bestBuy->quantity == bestSell->quantity) {
            removeOrder(bestBuy->orderID, false);
            removeOrder(bestSell->orderID, false);
        } else if (bestBuy->quantity > bestSell->quantity) {
            removeOrder(bestSell->orderID, false);
            bestBuy->quantity -= orderQuantity;
        } else { // bestSell->quantity > bestBuy->quantity
            removeOrder(bestBuy->orderID, false);
            bestSell->quantity -= orderQuantity;
        }

        // Print completed order
        if (print) {
            cout << "Order successfully completed:" << endl
                 << "  Ticker: " << ticker << endl
                 << "  Price: " << orderPrice << endl
                 << "  Quantity: " << orderQuantity << endl;
        }

        if (!tickerMap[ticker]->bestBuyOrder || !tickerMap[ticker]->bestSellOrder) {
            return;
        } else {
            bestBuy = tickerMap[ticker]->bestBuyOrder->head->order;
            bestSell = tickerMap[ticker]->bestSellOrder->head->order;
        }
    }
}

// int main() {
//     OrderBook orderBook = OrderBook();
//     if (orderBook.buyOrderList[0] == nullptr) {
//         cout << "test" << endl;
//     }
//     return 0;
// }