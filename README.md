# Parallel Lockless Order Book

## Overview

This project is a research-oriented implementation of a **parallel order book** built in **C++**. The order book is constructed around a **lockless deque**, inspired by the design of Sundell and Tsigas. The goal is to explore whether lock-free techniques can improve the speed and efficiency of order book operations compared to traditional approaches.

An order book is a data structure used in financial markets to match buy and sell orders. Since performance and low latency are critical in this domain, this project investigates how concurrency and lock-free algorithms can be applied to optimize these systems.

## Key Features

- **Lockless Deque Core**: A custom implementation of a lock-free deque.
- **Custom Memory Management**: Per-thread memory pools with safe handling of cross-thread deallocations.
- **Parallel Order Book Design**: Separate deques for each price level on the bid and ask sides.
- **Testing**: Includes unit tests with Google Test (GTest) and long-duration stress tests.

## Purpose

The project is designed as both a **proof of concept** and a **research platform**. The final goal is to benchmark the lockless order book against standard single-threaded and parallelized implementations, and evaluate its potential for use in high-performance, latency-sensitive applications such as high-frequency trading.
