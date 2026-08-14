# 4.3 Memory Ordering（内存序）

> 所属：**Lower-Level Concurrency** · [← 章索引](./README.md)

| Ordering | 直觉 |
|----------|------|
| **`Relaxed`** | 仅本原子变量的原子性；**几乎不**建立与其他内存的顺序；最快、最易误用 |
| **`Acquire` / `Release`** | 锁语义：**Release** 前写入对之后 **Acquire** 同锁线程可见 |
| **`AcqRel`** | RMW 常用 |
| **`SeqCst`** | 全局一致顺序；强、贵；调试或极少数协议 |

**Happens-before** 是正确推理工具 — 与 [07 内存操作](./07-memory-operations.md) 对照读。
