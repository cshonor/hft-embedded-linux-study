# 5.1 Start Simple（从简单开始）

> 所属：**Sane Concurrency** · [← 章索引](./README.md)

1. 先用 **`Mutex` / channel** 写对。
2. **Profiling** 证明锁是瓶颈后再上无锁。
3. 消息传递优先于共享可变状态 — ER Item 17。

**不要**为「可能快」先写 CAS 迷宫。
