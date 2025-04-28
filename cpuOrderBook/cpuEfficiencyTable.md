# Naive Linked List Efficiency Data

## Adding Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 26 | 2.600000 | 384615.384615 |
| 100 | 174 | 1.740000 | 574712.643678 |
| 1000 | 1320 | 1.320000 | 757575.757576 |
| 10000 | 12735 | 1.273500 | 785237.534354 |
| 100000 | 126412 | 1.264120 | 791064.139480 |

Latency and throughout expectedly stay consistent as orders are added

## Reamoving Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 6 | 0.600000 | 1666666.666667 |
| 100 | 47 | 0.470000 | 2127659.574468 |
| 1000 | 438 | 0.438000 | 2283105.022831 |
| 10000 | 4406 | 0.440600 | 2269632.319564 |
| 100000 | 70802 | 0.708020 | 1412389.480523 |

Latency and throughout also stays consistent here

## Order Match Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 18 | 1.800000 | 555555.555556 |
| 100 | 166 | 1.660000 | 602409.638554 |
| 1000 | 1649 | 1.649000 | 606428.138266 |
| 10000 | 16398 | 1.639800 | 609830.467130 |
| 100000 | 164235 | 1.642350 | 608883.611898 |

Latency and throughput are once again fairly consistent here.
Although, it is worth noting that the best orders are reassigned every time here
