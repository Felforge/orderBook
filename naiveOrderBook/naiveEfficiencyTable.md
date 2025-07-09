# Naive Linked List Efficiency Data

## Adding Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 23 | 2.300000 | 434782.608696 |
| 100 | 523 | 5.230000 | 191204.588910 |
| 1000 | 43682 | 43.682000 | 22892.724692 |
| 10000 | 1965136 | 196.513600 | 5088.706329 |
| 100000 | 198758234 | 1987.582340 | 503.123810 |

Clearly the naive linked-list implementation becomes extremely inefficient at adding orders as the order count increases

## Reamoving Head Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 2 | 0.200000 | 5000000.000000 |
| 100 | 14 | 0.140000 | 7142857.142857 |
| 1000 | 170 | 0.170000 | 5882352.941176 |
| 10000 | 1961 | 0.196100 | 5099439.061703 |
| 100000 | 14875 | 0.148750 | 6722689.075630 |

Latency and throughput are fairly consistent when removing from the head

## Reamoving Tail Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 1839 | 183.900000 | 5437.737901 |
| 100 | 17988 | 179.880000 | 5559.261730 |
| 1000 | 192130 | 192.130000 | 5204.809244 |
| 10000 | 1729813 | 172.981300 | 5780.971700 |
| 100000 | 9101685 | 91.016850 | 10986.976587 |

Latency gets continuously lower as the linked-list shortens so the average for 100,000 is much lower

## Order Match Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 4 | 0.400000 | 2500000.000000 |
| 100 | 31 | 0.310000 | 3225806.451613 |
| 1000 | 348 | 0.348000 | 2873563.218391 |
| 10000 | 3348 | 0.334800 | 2986857.825568 |
| 100000 | 31475 | 0.314750 | 3177124.702145 |

Latency and throughput are fairly consistent despite the number of orders as the top orders are easily accessible for matching
