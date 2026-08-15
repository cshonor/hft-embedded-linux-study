# 2.1 Shared Memory（共享内存）

> 所属：**Concurrency Models** · [← 章索引](./README.md)

多线程通过 **`Mutex` / `RwLock`** 等共享可变状态。

| 适合 | 代价 |
|------|------|
| 复杂、非交换性共享状态 | 热点锁竞争 |

**缓解**：读写比例 → `RwLock`；**分片锁 (sharding)** 降低争用。

Book → [16.3 共享状态并发](../../00-Book/16-fearless-concurrency/16.3-共享状态并发.md)
