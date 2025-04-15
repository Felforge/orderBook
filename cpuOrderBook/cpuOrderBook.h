#ifndef CPUORDERBOOK_H
#define CPUORDERBOOK_H

#include <string>
#include <unordered_map>

struct Order {
    int id;
    double price;
    int quantity;
    std::string type;
    std::string side;
    Order(int id, std::string side, std::string type, int quantity, double price);
};

struct PriceLevel {
    Order* order;
    PriceLevel* prev;
    PriceLevel* next;
    PriceLevel(Order* order);
};

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
        int orderCount;

    public:
        // Easy access to every order
        // Just make sure to erase from here if the order gets deleted
        std::unordered_map<int, PriceLevel*> orderMap;
        OrderBook();
        ~OrderBook();
        void addOrder(std::string side, std::string type, int quantity, double price = 0.0, bool print = true, bool execute = true);
    };

#endif