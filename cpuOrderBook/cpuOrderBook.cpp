#include <iostream>
#include "cpuOrderBook.h"
using namespace std;

// Constructor
OrderBook::OrderBook() {

}

int main() {
    OrderBook orderBook = OrderBook();
    if (orderBook.buyOrderList[0] == nullptr) {
        cout << "test" << endl;
    }
    return 0;
}