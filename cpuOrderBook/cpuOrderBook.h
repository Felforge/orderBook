#ifndef CPUORDERBOOK_H
#define CPUORDERBOOK_H

#include <string>
#include <unordered_map>

struct Order {
    int orderID;
    int userID;
    double price;
    int quantity;
    std::string side;
    Order(int orderID, int userID, std::string side, int quantity, double price);
};

struct OrderNode {
    Order* order;
    OrderNode* prev;
    OrderNode* next;
    OrderNode(Order* order);
};

struct PriceLevel {
    OrderNode* head;
    OrderNode* tail;
    PriceLevel(OrderNode* orderNode);
};

// best orders are being left as PriceLevel to access the whole doubly linked list
struct Ticker {
    std::string ticker;
    PriceLevel* buyOrderList[100000];
    PriceLevel* sellOrderList[100000];
    PriceLevel* bestBuyOrder; // pointer to current best buy order
    PriceLevel* bestSellOrder; // pointer to current best sell order
    Ticker(std::string ticker);
};

class OrderBook {
    private:
        int orderID;

    public:
        // Easy access to every order
        // Just make sure to erase from here if the order gets deleted
        std::unordered_map<int, Order*> orderMap;
        std::unordered_map<std::string, Ticker*> tickerMap;
        OrderBook();
        ~OrderBook();
        void addTicker(std::string ticker);
        void addOrder(int userID, std::string ticker, std::string side, int quantity, double price, bool print = true);
    };

#endif