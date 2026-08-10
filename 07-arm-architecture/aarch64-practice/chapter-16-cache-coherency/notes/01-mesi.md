# §16.1 MESI 协议

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

MESI 是多核 Cache 一致性协议，四个状态：Modified（已修改，内存过期）、Exclusive（独占，=内存）、Shared（共享，=内存，多核有副本）、Invalid（无效）。

## 核心要点

### 四个状态

| 状态 | 含义 | Cache vs 内存 | 其他核有副本？ |
|------|------|--------------|---------------|
| **M** | 已修改 | Cache 有最新数据，内存过期 | 无 |
| **E** | 独占 | Cache = 内存 | 无 |
| **S** | 共享 | Cache = 内存 | 有（也是 S） |
| **I** | 无效 | Cache 行无效 | — |

### 状态转换

```
读 miss → 从内存加载 → E（如果无其他核）或 S（如果其他核有）

写 S → 广播 Invalidate → 其他核变 I → 本核变 M

写 E → 直接改 → M（无需广播）

M 被逐出 → 写回内存 → E（如果独占）或 S
```

### 关键转换

| 操作 | 原状态 | 新状态 | 说明 |
|------|--------|--------|------|
| 读命中 | M/E/S | 不变 | 直接读 cache |
| 写命中 | M | M | 直接写 |
| 写命中 | S | M | 广播 Invalidate，其他核变 I |
| 写命中 | E | M | 直接写，无需广播 |
| 读 miss | I | E/S | 从内存加载 |
| 写 miss | I | M | 从内存加载后写 |

## HFT 关联

理解 MESI 对 HFT 多核优化至关重要。E（独占）状态的写不需要广播——如果数据只有一个核访问，写操作零额外开销。S（共享）状态的写需要广播 Invalidate，增加延迟。HFT 系统应尽量让每个核有独占数据（如每核独立的订单队列），避免共享变量的写竞争。M 状态的 cache 行被 evict 时需要写回内存（~100ns），应避免频繁 evict 热数据。

## 自测题

1. **M 和 E 状态的区别是什么？**

<details>
<summary>答案</summary>

- **M**（Modified）：Cache 有最新数据，**内存已过期**（不一致）。被 evict 时必须写回内存。
- **E**（Exclusive）：Cache = 内存（**一致**）。被 evict 时不需要写回（直接丢弃）。

两者都是"只有一个核有副本"，但 M 的内存不是最新的，E 的内存是最新的。
</details>

2. **S 状态的 cache 行被写时发生什么？**

<details>
<summary>答案</summary>

广播 **Invalidate** 给所有拥有该 cache 行副本的核 → 其他核的该行变 **I** → 本核变 **M**。这个过程有总线开销（广播+等待确认），比 E 状态的写（直接变 M，无广播）慢。
</details>

3. **为什么 E 状态的写比 S 状态的写快？**

<details>
<summary>答案</summary>

E 状态只有一个核有副本，写操作直接变 M，**不需要广播 Invalidate**。S 状态有多个核有副本，写操作需要广播 Invalidate 给所有持有副本的核并等待确认，有总线延迟。这就是为什么 HFT 应避免共享变量的写——共享写触发 MESI 广播开销。
</details>

## 参考与延伸

- [§16.2 伪共享](02-false-sharing.md) — MESI 导致的伪共享问题
- [§16.3 DMA 一致性](03-dma-coherency.md) — MESI 与 DMA 的交互
- [Ch18 §18.1 弱序内存模型](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — MESI 与内存模型的关系
