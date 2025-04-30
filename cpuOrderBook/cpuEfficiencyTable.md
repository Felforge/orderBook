# Naive Linked List Efficiency Data

## Adding Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 31 | 3.100000 | 322580.645161 |
| 100 | 181 | 1.810000 | 552486.187845 |
| 1000 | 1348 | 1.348000 | 741839.762611 |
| 10000 | 12910 | 1.291000 | 774593.338497 |
| 100000 | 129583 | 1.295830 | 771706.165161 |

Latency and throughout expectedly stay consistent as orders are added

## Reamoving Order Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 6 | 0.600000 | 1666666.666667 |
| 100 | 46 | 0.460000 | 2173913.043478 |
| 1000 | 449 | 0.449000 | 2227171.492205 |
| 10000 | 4433 | 0.443300 | 2255808.707422 |
| 100000 | 71652 | 0.716520 | 1395634.455423 |

Latency and throughout also stays consistent here

## Order Match Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |  :-----------: |
| 10 | 18 | 1.800000 | 555555.555556 |
| 100 | 169 | 1.690000 | 591715.976331 |
| 1000 | 1679 | 1.679000 | 595592.614652 |
| 10000 | 16744 | 1.674400 | 597228.858098 |
| 100000 | 168209 | 1.682090 | 594498.510781 |

Latency and throughput are once again fairly consistent here
Although, it is worth noting that the best orders are reassigned every time here
