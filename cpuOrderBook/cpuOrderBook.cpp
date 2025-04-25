#include <iostream>
#include "cpuOrderBook.h"
using namespace std;

const int NUM_TICKERS = 5;

// Constructor for Order
Order::Order(int orderID, int userID, string side, string ticker, int quantity, double price)
    : orderID(orderID), userID(userID), side(side), ticker(ticker), quantity(quantity), price(price) {}

// Constructor for Order Node
// This is a node of the doubly linked list
OrderNode::OrderNode(Order* order) : order(order), prev(nullptr), next(nullptr) {}

// Constructor for PriceLevel
// Wrapper for the doubly linked list that makes head and tail accessible 
PriceLevel::PriceLevel(OrderNode* orderNode) : head(orderNode), tail(orderNode) {} 

// Constructor for Ticker
Ticker::Ticker(string ticker) : ticker(ticker), bestBuyOrder(nullptr), bestSellOrder(nullptr) {}

// Constructor
OrderBook::OrderBook() {
    // Declare memore pool
    // : orderPool(sizeof(Order), MAX_ORDERS), nodePool(sizeof(OrderNode), MAX_ORDERS), 
    //   priceLevelPool(sizeof(PriceLevel), NUM_TICKERS * MAX_PRICE_IDX), tickerPool(sizeof(Ticker), NUM_TICKERS) 
    orderPool = MemoryPool(sizeof(Order), MAX_PRICE_IDX);
    nodePool = MemoryPool(sizeof(OrderNode), MAX_PRICE_IDX);
    priceLevelPool = MemoryPool(sizeof(PriceLevel), NUM_TICKERS * MAX_PRICE_IDX);
    tickerPool = MemoryPool(sizeof(Ticker), NUM_TICKERS);

    // Declare ID counter
    orderID = 0;

    // Add Tickers
    addTicker("AAPL");
    addTicker("MSFT");
    addTicker("NVDA");
    addTicker("AMZN");
    addTicker("GOOG");
}

// Destructor
OrderBook::~OrderBook() {
    // Delete every order pointer
    for (const auto& pair: orderMap) {
        orderPool.deallocate(pair.second->order);
    }

    // Delete every price level pointer and ticker pointer
    for (const auto& pair: tickerMap) {
        for (int i = 0; i < MAX_PRICE_IDX; i++) {
            while (pair.second->buyOrderList[i]->tail != pair.second->buyOrderList[i]->head) {
                OrderNode* temp = pair.second->buyOrderList[i]->tail;
                pair.second->buyOrderList[i]->tail = temp->prev;
                nodePool.deallocate(temp);
            }
            nodePool.deallocate(pair.second->buyOrderList[i]->head);
            priceLevelPool.deallocate(pair.second->buyOrderList[i]);
            while (pair.second->sellOrderList[i]->tail != pair.second->sellOrderList[i]->head) {
                OrderNode* temp = pair.second->sellOrderList[i]->tail;
                pair.second->sellOrderList[i]->tail = temp->prev;
                nodePool.deallocate(temp);
            }
            nodePool.deallocate(pair.second->sellOrderList[i]->head);
            priceLevelPool.deallocate(pair.second->sellOrderList[i]);
        }
        tickerPool.deallocate(pair.second);
    }
}

int OrderBook::getListIndex(double price) {
    return int(price * 100.0) - 1;
}

void OrderBook::addTicker(string ticker) {
    tickerMap[ticker] = static_cast<Ticker*>(tickerPool.allocate());
    new (tickerMap[ticker]) Ticker(ticker);
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

    cout << "test" << endl;

    // Allocate memory and create new order and node 
    Order* newOrder = new (orderPool.allocate()) Order(userID, orderID, side, ticker, quantity, price);
    cout << "test2" << endl;
    OrderNode* orderNode = new (nodePool.allocate()) OrderNode(newOrder);

    cout << "test3" << endl;

    // Insert order into order map
    orderMap[orderID] = orderNode;

    if (side == "BUY") {
        if (tickerMap[ticker]->buyOrderList[listIdx] == nullptr) {
            tickerMap[ticker]->buyOrderList[listIdx] = static_cast<PriceLevel*>(priceLevelPool.allocate());
            new (tickerMap[ticker]->buyOrderList[listIdx]) PriceLevel(orderNode);
            if (tickerMap[ticker]->bestBuyOrder == nullptr || price > tickerMap[ticker]->bestBuyOrder->head->order->price) {
                tickerMap[ticker]->bestBuyOrder = tickerMap[ticker]->buyOrderList[listIdx];
            }
        } else {
            orderNode->prev = tickerMap[ticker]->buyOrderList[listIdx]->tail;
            tickerMap[ticker]->buyOrderList[listIdx]->tail->next = orderNode;
            tickerMap[ticker]->buyOrderList[listIdx]->tail = orderNode;
        }
        // Mark price level as active
        tickerMap[ticker]->activeBuyPrices.push(listIdx);
    } else { // side == "SELL"
        if (tickerMap[ticker]->sellOrderList[listIdx] == nullptr) {
            tickerMap[ticker]->sellOrderList[listIdx] = static_cast<PriceLevel*>(priceLevelPool.allocate());
            new (tickerMap[ticker]->sellOrderList[listIdx]) PriceLevel(orderNode);
            if (tickerMap[ticker]->bestSellOrder == nullptr || price < tickerMap[ticker]->bestSellOrder->head->order->price) {
                tickerMap[ticker]->bestSellOrder = tickerMap[ticker]->sellOrderList[listIdx];
            }
        } else {
            orderNode->prev = tickerMap[ticker]->sellOrderList[listIdx]->tail;
            tickerMap[ticker]->sellOrderList[listIdx]->tail->next = orderNode;
            tickerMap[ticker]->sellOrderList[listIdx]->tail = orderNode;
        }
        // Mark price level as active
        tickerMap[ticker]->activeSellPrices.push(listIdx);
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

    // Flag for deleteing level
    bool deleteLevel = false;
    PriceLevel* levelPtr;

    // Get list index
    int listIdx = getListIndex(nodePtr->order->price);

    if (nodePtr->prev == nullptr || nodePtr->next == nullptr) {
        if (nodePtr->order->side == "BUY") {
            levelPtr = tickerMap[ticker]->buyOrderList[listIdx];
        } else {
            levelPtr = tickerMap[ticker]->sellOrderList[listIdx];
        }
        if (nodePtr->prev == nullptr && nodePtr->next == nullptr) {
            deleteLevel = true;
        } else if (nodePtr->prev == nullptr) {
            levelPtr->head = nodePtr->next;
        } else { // nodePtr->next == nullptr
            levelPtr->tail = nodePtr->prev;
        }
    } else {
        nodePtr->prev->next = nodePtr->next;
    }

    // Always has to get deleted so it is outside the if statement
    // Scary memory leak if done wrong
    orderPool.deallocate(nodePtr->order);
    nodePool.deallocate(nodePtr);
    if (deleteLevel) {
        // Reassign bestBuy or bestSell
        if (levelPtr == tickerMap[ticker]->bestBuyOrder) {
            // Mark price level as no longer active
            tickerMap[ticker]->activeBuyPrices.pop();

            // Update bestBuyOrder
            if (!tickerMap[ticker]->activeBuyPrices.empty()) {
                int bestBuyIdx = tickerMap[ticker]->activeBuyPrices.top();
                tickerMap[ticker]->bestBuyOrder = tickerMap[ticker]->buyOrderList[bestBuyIdx];
            } else {
                // No active buy orders
                tickerMap[ticker]->bestBuyOrder = nullptr;
            }
        } else if (levelPtr == tickerMap[ticker]->bestSellOrder) {
            // Mark price level as no longer active
            tickerMap[ticker]->activeSellPrices.pop();
            
            // Update bestSellOrder
            if (!tickerMap[ticker]->activeSellPrices.empty()) {
                int bestSellIdx = tickerMap[ticker]->activeSellPrices.top();
                tickerMap[ticker]->bestSellOrder = tickerMap[ticker]->sellOrderList[bestSellIdx];
            } else {
                // No active sell orders
                tickerMap[ticker]->bestSellOrder = nullptr;
            }
        }
        priceLevelPool.deallocate(levelPtr);
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