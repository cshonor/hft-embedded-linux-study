# 1.2 Performance（性能）

> 所属：**The Trouble with Concurrency** · [← 章索引](./README.md)

## 锁的串行化

互斥抹平并行收益 — 「加线程反而变慢」并不少见。

## 伪共享 (False Sharing)

两核修改**同一缓存行**上不同变量 → 缓存一致性流量。

**缓解**：对齐 / **padding** 把热点计数器拆到独立缓存行、数据结构**分片**。

ER → [Item 20 避免过度优化](../../01-ER/Chapter-03-Concepts/Item-20-avoid-over-optimize/README.md)
