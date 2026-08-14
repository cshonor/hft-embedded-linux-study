# 1.1 Correctness（正确性）

> 所属：**The Trouble with Concurrency** · [← 章索引](./README.md)

## 编译期能挡什么

借用检查 + **`Send` / `Sync`** → 挡掉大量**内存级数据竞争**。

## 仍可能发生

- **死锁**
- **逻辑竞态**（业务顺序 bug，与内存安全无关）
- **unsafe** 破坏假设

Book → [16.4 Send 与 Sync](../../00-Book/16-fearless-concurrency/16.4-Send与Sync.md) · ER → [Item 17](../../01-ER/Chapter-03-Concepts/Item-17-shared-state-parallelism/README.md)
