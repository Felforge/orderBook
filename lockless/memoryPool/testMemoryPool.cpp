#include <gtest/gtest.h>
#include <thread>
#include "memoryPool.h"
using namespace std;

string captureOutput(function<void()> func) {
    stringstream buffer;
    streambuf* old = cout.rdbuf(buffer.rdbuf());
    func();  // Run function that prints output
    cout.rdbuf(old);
    return buffer.str();
}

// Test Memory Pool Allocation
TEST(LocklessMemoryPoolTest, HandlesMemoryAllocation) {
}


// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}