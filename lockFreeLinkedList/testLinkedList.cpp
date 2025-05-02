#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include "lockFreeLinkedList.h"

std::string captureOutput(std::function<void()> func) {
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    func();  // Run function that prints output
    std::cout.rdbuf(old);
    return buffer.str();
}

// Check if two order pointers are equal in values
void isOrderEqual(Order* expected, Order* actual) {
    EXPECT_EQ(expected->orderID, actual->orderID);
    EXPECT_EQ(expected->userID, actual->userID);
    EXPECT_EQ(expected->price, actual->price);
    EXPECT_EQ(expected->quantity, actual->quantity);
    EXPECT_EQ(expected->side, actual->side);
    EXPECT_EQ(expected->ticker, actual->ticker);
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}