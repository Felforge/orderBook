# Naive Linked List Efficiency Data

## Order Adding Efficiency Table

| **Num of Orders** | **Total Runtime (µs)** | **Latency Per Order (µs/Order)** | **Throughput (Orders/Second)** |
| :-----------: |  :-----------: |  :-----------: |
| 10 | 11 | 1.100000 | 909090 |
| 1000 | 23254 | 23.254000 | 43003 |
| 100000 | 84913107 | 849.131070 | 1177 |
Clearly the naive linked-list implementation becomes extremely inefficient as the order count increases

