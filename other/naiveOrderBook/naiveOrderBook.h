#ifndef NAIVEORDERBOOK_H
#define NAIVEORDERBOOK_H

#include <iostream>
#include <string>

struct Order {
    int id;
    double price;
    int quantity;
    std::string type;
    Order(int id, double price, int quantity, std::string side);
};

struct OrderList {
    Order* order;
    OrderList* next;
    OrderList(Order* order);
};

class OrderBook {
    private:
        int orderCount;
        OrderList* searchOrderList(int searchID, OrderList* orderList);

    public:
        OrderList* buyOrderList;
        OrderList* sellOrderList;
        OrderBook();
        ~OrderBook();
        void addOrder(double price, int quantity, std::string type, bool print=true);
        void removeOrder(int id, std::string type, bool print=true);
        void matchOrders(bool print=true, int count=0);
};

#endif