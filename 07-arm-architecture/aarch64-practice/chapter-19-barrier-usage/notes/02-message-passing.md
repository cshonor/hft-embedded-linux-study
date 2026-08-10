# §19.2 案例二：消息传递（邮箱）

> **来源：** [Ch19 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

消息传递场景只需要 wmb/rmb（Store-Store 和 Load-Load 屏障），不需要 mb（全屏障）。生产者用 smp_wmb() 保证 data 写在 ready 之前，消费者用 smp_rmb() 保证读 ready 后再读 data。

## 核心要点

### 消息传递屏障模式

```c
// 生产者
msg->data = payload;
msg->ready = true;
smp_wmb();  // ← 保证 data 写在 ready 之前

// 消费者
while (!msg->ready)
    smp_rmb();  // ← 保证读 ready 后再读 data
use(msg->data);
```

### 为什么只需要 wmb/rmb？

| 屏障 | 约束 | 为什么足够 |
|------|------|-----------|
| `smp_wmb()` = `dmb ishst` | Store-Store 有序 | 生产者只需 data 写在 ready 之前可见 |
| `smp_rmb()` = `dmb ishld` | Load-Load 有序 | 消费者只需读 ready 后再读 data |

- 不需要约束 Load-Store 或 Store-Load → 用更弱的屏障省性能
- `smp_mb()`（全屏障）也能工作但开销更大

### 与全屏障的性能对比

| 屏障 | 指令 | 估计延迟 |
|------|------|----------|
| `smp_wmb()` | `dmb ishst` | ~3-5ns |
| `smp_rmb()` | `dmb ishld` | ~3-5ns |
| `smp_mb()` | `dmb ish` | ~5-10ns |

> 每次省 2-5ns，在高频场景（如 HFT 无锁队列）累积效果显著。

## HFT 关联

消息传递模式是 HFT SPSC 无锁队列的原型。生产者写订单数据后写 index，消费者读 index 后读订单数据。用 `smp_wmb()` + `smp_rmb()` 配对比 `smp_mb()` 节省约 5ns/次。如果用 C++ `memory_order_release`/`memory_order_acquire`（编译为 STLR/LDAR），可以再省 2-3ns。HFT 中这个优化非常关键——无锁队列每秒可能调用百万次 push/pop，每次省 5ns 累计节省显著。

## 自测题

1. **消息传递场景为什么只需要 wmb/rmb 而不需要 mb？**

<details>
<summary>答案</summary>

生产者只需 **Store-Store 有序**（data 写在 ready 之前）→ `smp_wmb()` 约束 Store-Store。消费者只需 **Load-Load 有序**（读 ready 后再读 data）→ `smp_rmb()` 约束 Load-Load。不需要约束 Load-Store 或 Store-Load（这两种场景不存在）。用更弱的屏障省性能——`dmb ishst`/`dmb ishld` 比 `dmb ish` 更轻。
</details>

2. **如果误用 smp_mb() 替代 smp_wmb()/smp_rmb()，有什么影响？**

<details>
<summary>答案</summary>

**正确性不受影响**——smp_mb() 是更强的屏障，包含 wmb/rmb 的约束。但**性能下降**——`dmb ish`（全屏障）比 `dmb ishst`/`dmb ishld`（仅 Store/Load）重约 2-5ns。在高频场景（百万次/秒）累积效果显著。选最弱的足够屏障是性能优化的关键。
</details>

3. **如何用 C++ atomic 实现等价的消息传递？**

<details>
<summary>答案</summary>

```cpp
// 生产者
msg->data = payload;
msg->ready.store(true, std::memory_order_release);
// release = smp_wmb() + Store，编译为 STLR

// 消费者
while (!msg->ready.load(std::memory_order_acquire))
    ;
// acquire = Load + smp_rmb()，编译为 LDAR
use(msg->data);
```

`memory_order_release`/`memory_order_acquire` 比 `smp_wmb`/`smp_rmb` 更高效（用 STLR/LDAR 替代显式 DMB）。
</details>

## 参考与延伸

- [§19.6 HFT 中的屏障使用](06-hft-spsc.md) — SPSC 队列的完整实现
- [Ch18 §18.3 典型场景](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — 消息传递场景详解
- [Ch18 §18.4 Acquire/Release](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — LDAR/STLR 优势
