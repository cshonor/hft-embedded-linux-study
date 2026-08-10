# §16.6 易错点清单

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Cache 一致性的 4 个常见错误：伪共享、DMA 不做 Cache 操作、自修改代码不刷 I-Cache、MESI 状态记混。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | 伪共享 | 不同核的变量在同一 Cache Line，反复 invalidate | 对齐填充到 64 字节 |
| 2 | DMA 不做 Cache 操作 | 数据不一致 | DMA 前 invalidate/clean |
| 3 | 自修改代码不刷 I-Cache | 执行旧指令 | clean D-cache → invalidate I-cache |
| 4 | MESI 记混 | M=修改（内存过期）；E=独占（=内存）；S=共享（=内存，多核有副本） | 理解状态语义 |

### MESI 速记

| 状态 | 一句话记忆 |
|------|-----------|
| M | 改了，只有我有，内存过期 |
| E | 只有我有，内存是最新的 |
| S | 大家都有，都是最新的 |
| I | 我的副本无效 |

### 调试技巧

| 症状 | 检查方向 |
|------|----------|
| 多核性能远低于预期 | 伪共享（perf c2c） |
| DMA 数据偶尔错误 | Cache 一致性（invalidate/clean） |
| 代码修改不生效 | I-Cache（invalidate） |
| 共享变量写后其他核看不到 | 内存屏障（DMB，见 Ch18） |

## HFT 关联

这 4 个错误在 HFT 系统中都有致命后果。伪共享导致延迟抖动（不可预测的 cache line 传输）。DMA cache 错误导致网络数据丢失或损坏。自修改代码 I-Cache 问题导致"偶发"执行旧逻辑。MESI 理解不足导致错误使用共享变量。HFT 开发者应该把这 4 个检查点作为代码审查的必查项。

## 自测题

1. **MESI 四个状态中，哪个状态的 cache 行被 evict 时不需要写回内存？**

<details>
<summary>答案</summary>

**E**（Exclusive）和 **S**（Shared）。因为 E 和 S 状态的 cache 行与内存一致（Cache = 内存），evict 时直接丢弃即可。**M**（Modified）状态与内存不一致（内存过期），evict 时必须写回内存。**I**（Invalid）是无效的，没有数据需要处理。
</details>

2. **多核性能异常低，应该首先检查什么？**

<details>
<summary>答案</summary>

首先检查**伪共享**。用 `perf c2c record/report` 查看 HITM 指标。如果 HITM 高，说明 cache line 在核间频繁传输，很可能有伪共享。然后检查多核共享变量的布局，用 `aligned(64)` 修复。
</details>

3. **共享变量写后其他核看不到新值，可能的原因有哪些？**

<details>
<summary>答案</summary>

可能原因：
1. **缺少内存屏障**：ARM 弱序模型，Store-Store 可能重排，需要 `dmb ishst` 或 `smp_wmb()`
2. **编译器重排**：硬件屏障不阻止编译器重排，需要 `barrier()` 或 `volatile`
3. **Cache 一致性问题**（较少见）：MESI 应自动处理，但如果 DMA 涉及可能需要手动 flush
4. **`atomic_read` 不保证可见性**：需要 `smp_mb__after_atomic()` 或使用 acquire/release 语义
</details>

## 参考与延伸

- [§16.1 MESI 协议](01-mesi.md) — MESI 状态详解
- [§16.2 伪共享](02-false-sharing.md) — 伪共享修复
- [Ch18 §18.7 易错点](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — 内存屏障相关错误
