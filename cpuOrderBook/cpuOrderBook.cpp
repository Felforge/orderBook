#include <iostream>
#include "cpuOrderBook.h"
using namespace std;

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
OrderBook::OrderBook(int numTickers) 
    // Declare memory pool
    : orderPool(sizeof(Order), 2 * MAX_PRICE_IDX), nodePool(sizeof(OrderNode), 2 * MAX_PRICE_IDX),
      priceLevelPool(sizeof(PriceLevel), 2 * numTickers * MAX_PRICE_IDX), tickerPool(sizeof(Ticker), numTickers) {

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
}

int OrderBook::getListIndex(double price) {
    return int(price * 100.0) - 1;
}

void OrderBook::addTicker(string ticker) {
    void* memoryBlock = tickerPool.allocate();
    tickerMap[ticker] = new (memoryBlock) Ticker(memoryBlock, ticker);
}

// Update best buy order
void OrderBook::updateBestBuyOrder(string ticker) {
    auto& priorities = tickerMap[ticker]->priorityBuyPrices;
    auto& active = tickerMap[ticker]->activeBuyPrices;

    while (!priorities.empty()) {
        int bestBuyIdx = priorities.top();
        if (active[bestBuyIdx]) {
            tickerMap[ticker]->bestBuyOrder = tickerMap[ticker]->buyOrderList[bestBuyIdx];
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
    auto& active = tickerMap[ticker]->activeSellPrices;

    while (!priorities.empty()) {
        int bestSellIdx = priorities.top();
        if (active[bestSellIdx]) {
            tickerMap[ticker]->bestSellOrder = tickerMap[ticker]->sellOrderList[bestSellIdx];
            return;
        }
        // Remove inactive price lvel
        priorities.pop();
    }
    // No active buy orders
    tickerMap[ticker]->bestBuyOrder = nullptr;
}

// Handle removing price level
void OrderBook::removePriceLevel(std::string side, std::string ticker, int listIdx, PriceLevel* levelPtr) {
    if (side == "BUY") {
            tickerMap[ticker]->activeBuyPrices[listIdx] = false;
            tickerMap[ticker]->buyOrderList[listIdx] = nullptr;
            if (levelPtr == tickerMap[ticker]->bestBuyOrder) {
                updateBestBuyOrder(ticker);
            }
        } else { // side == "SELL"
            tickerMap[ticker]->activeSellPrices[listIdx] = false;
            tickerMap[ticker]->sellOrderList[listIdx] = nullptr;
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
    } else if (price > double(MAX_PRICE_IDX / 100)) {
        cout << "Order Book Error: Maximum Available Price is " << MAX_PRICE_IDX / 100 << endl;
        return;
    } else if (tickerMap.find(ticker) == tickerMap.end()) {
        cout << "Order Book Error: Ticker is Invalid" << endl;
        return;
    }

    // Get index for price level
    int listIdx = getListIndex(price);

    // Allocate memory for order, node and priceLevel
    void* orderMemoryBlock = orderPool.allocate();
    void* nodeMemoryBlock = nodePool.allocate();
    void* priceLevelMemoryBlock = priceLevelPool.allocate();

    // Create new order and node
    Order* newOrder = new (orderMemoryBlock) Order(orderMemoryBlock, orderID, userID, side, ticker, quantity, price);
    OrderNode* orderNode = new (nodeMemoryBlock) OrderNode(nodeMemoryBlock, newOrder);

    // Insert order into order map
    orderMap[orderID] = orderNode;

    if (side == "BUY") {
        if (tickerMap[ticker]->buyOrderList[listIdx] == nullptr) {
            tickerMap[ticker]->buyOrderList[listIdx] = new (priceLevelMemoryBlock) PriceLevel(priceLevelMemoryBlock, orderNode);
            if (tickerMap[ticker]->bestBuyOrder == nullptr || price > tickerMap[ticker]->bestBuyOrder->head->order->price) {
                tickerMap[ticker]->bestBuyOrder = tickerMap[ticker]->buyOrderList[listIdx];
            }
        } else {
            orderNode->prev = tickerMap[ticker]->buyOrderList[listIdx]->tail;
            tickerMap[ticker]->buyOrderList[listIdx]->tail->next = orderNode;
            tickerMap[ticker]->buyOrderList[listIdx]->tail = orderNode;
        }
        // Mark price level as active
        tickerMap[ticker]->priorityBuyPrices.push(listIdx);
        tickerMap[ticker]->activeBuyPrices[listIdx] = true;
    } else { // side == "SELL"
        if (tickerMap[ticker]->sellOrderList[listIdx] == nullptr) {
            tickerMap[ticker]->sellOrderList[listIdx] = new (priceLevelMemoryBlock) PriceLevel(priceLevelMemoryBlock, orderNode);
            if (tickerMap[ticker]->bestSellOrder == nullptr || price < tickerMap[ticker]->bestSellOrder->head->order->price) {
                tickerMap[ticker]->bestSellOrder = tickerMap[ticker]->sellOrderList[listIdx];
            }
        } else {
            orderNode->prev = tickerMap[ticker]->sellOrderList[listIdx]->tail;
            tickerMap[ticker]->sellOrderList[listIdx]->tail->next = orderNode;
            tickerMap[ticker]->sellOrderList[listIdx]->tail = orderNode;
        }
        // Mark price level as active
        tickerMap[ticker]->prioritySellPrices.push(listIdx);
        tickerMap[ticker]->activeSellPrices[listIdx] = true;
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

    // Get list index
    int listIdx = getListIndex(nodePtr->order->price);

    // Flag for deleteing level
    bool deleteLevel = false;
    PriceLevel* levelPtr = (side == "BUY") ? tickerMap[ticker]->buyOrderList[listIdx] : tickerMap[ticker]->sellOrderList[listIdx];

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
        removePriceLevel(side, ticker, listIdx, levelPtr);
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
    if (!tickerMap[ticker]->bestBuyOrder || !tickerMap[ticker]->bestSellOrder) {
        cout << "Order Book Error: No Orders To Be Matched" << endl;
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
            removeOrder(bestBuy->orderID);
            removeOrder(bestSell->orderID);
        } else if (bestBuy->quantity > bestSell->quantity) {
            removeOrder(bestSell->orderID);
            bestBuy->quantity -= orderQuantity;
        } else { // bestSell->quantity > bestBuy->quantity
            removeOrder(bestBuy->orderID);
            bestSell->quantity -= orderQuantity;
        }

        // Print completed order
        if (print) {
            cout << "Order successfully completed:" << endl
                 << "  Ticker: " << ticker << endl
                 << "  Price: " << orderPrice << endl
                 << "  Quantity: " << orderQuantity << endl;
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