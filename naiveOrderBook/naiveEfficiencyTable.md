# Naive Linked List Efficiency Data

## Adding Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 5 | 0.500000 | 2000000.000000 |
| 1000 | 6595 | 0.659500 | 1516300.227445 |
| 100000 | 62137752 | 62.137752 | 16093.276113 |

Clearly the naive linked-list implementation becomes extremely inefficient at adding orders as the order count increases

## Reamoving Head Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 2 | 0.200000 | 5000000.000000 |
| 1000 | 119 | 0.011900 | 84033613.445378 |
| 100000 | 10949 | 0.010949 | 91332541.784638 |

Latency and throughput are fairly consistent when removing from the head

## Reamoving Tail Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 2496 | 249.600000 | 4006.410256 |
| 1000 | 244222 | 24.422200 | 40946.352089 |
| 100000 | 11685634 | 11.685634 | 85575.160064 |

Latency gets continuously lower as the linked-list shortens so the average for 100,000 is much lower
