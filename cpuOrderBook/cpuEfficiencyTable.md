# Naive Linked List Efficiency Data

## Adding Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 32 | 3.200000 | 312500.000000 |
| 100 | 214 | 2.140000 | 467289.719626 |
| 1000 | 1747 | 1.747000 | 572409.845449 |
| 10000 | 17300 | 1.730000 | 578034.682081 |
| 100000 | 171122 | 1.711220 | 584378.396699 |

Latency and throughout expectedly stay consistent as orders are added

## Reamoving Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 7 | 0.700000 | 1428571.428571 |
| 100 | 54 | 0.540000 | 1851851.851852 |
| 1000 | 524 | 0.524000 | 1908396.946565 |
| 10000 | 5101 | 0.510100 | 1960399.921584 |
| 100000 | 84081 | 0.840810 | 1189329.337187 |

Latency and throughout also stays consistent here

## Order Match Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 19 | 1.900000 | 526315.789474 |
| 100 | 180 | 1.800000 | 555555.555556 |
| 1000 | 1772 | 1.772000 | 564334.085779 |
| 10000 | 17573 | 1.757300 | 569054.799977 |
| 100000 | 175306 | 1.753060 | 570431.131849 |

Latency and throughput are once again fairly consistent here
Although, it is worth noting that the best orders are reassigned every time here
