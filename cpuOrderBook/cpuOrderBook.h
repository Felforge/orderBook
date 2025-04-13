#ifndef CPUORDERBOOK_H
#define CPUORDERBOOK_H

#include <string>

struct Order {
    int id;
    double price;
    int quantity;
    std::string type;
    Order(int id, double price, int quantity, std::string side);
};

struct PriceLevel {
    Order order;
    PriceLevel* prev;
    PriceLevel* next;
};

class OrderBook {
    private:
        int orderCount;

    public:
        PriceLevel* buyOrderList[100000];
        PriceLevel* sellOrderList[100000];
        OrderBook();
};

#endif