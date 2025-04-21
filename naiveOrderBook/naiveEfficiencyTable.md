# Naive Linked List Efficiency Data

## Adding Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 11 | 1.100000 | 909090.909091 |
| 100 | 88 | 0.880000 | 1136363.636364 |
| 1000 | 6554 | 6.554000 | 152578.577968 |
| 10000 | 611038 | 61.103800 | 16365.594284 |
| 100000 | 61548228 | 615.482280 | 1624.742145 |

Clearly the naive linked-list implementation becomes extremely inefficient at adding orders as the order count increases

## Reamoving Head Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 3 | 0.300000 | 3333333.333333 |
| 100 | 9 | 0.090000 | 11111111.111111 |
| 1000 | 76 | 0.076000 | 13157894.736842 |
| 10000 | 739 | 0.073900 | 13531799.729364 |
| 100000 | 7780 | 0.077800 | 12853470.437018 |

Latency and throughput are fairly consistent when removing from the head

## Reamoving Tail Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 2783 | 278.300000 | 3593.244700 |
| 100 | 25410 | 254.100000 | 3935.458481 |
| 1000 | 254841 | 254.841000 | 3924.015366 |
| 10000 | 2389253 | 238.925300 | 4185.408577 |
| 100000 | 12017179 | 120.171790 | 8321.420526 |

Latency gets continuously lower as the linked-list shortens so the average for 100,000 is much lower

## Order Match Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 4 | 0.400000 | 2500000.000000 |
| 100 | 17 | 0.170000 | 5882352.941176 |
| 1000 | 154 | 0.154000 | 6493506.493506 |
| 10000 | 1526 | 0.152600 | 6553079.947575 |
| 100000 | 16379 | 0.163790 | 6105378.838757 |

Latency and throughput are fairly consistent despite the number of orders as the top orders are easily accessible for matching
