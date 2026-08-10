# §15.7 易错点清单

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Cache 的 4 个常见错误：DMA 忘做 Cache 操作、混淆 Clean 和 Invalidate、自修改代码忘清 I-Cache、VIPT 别名。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | DMA 忘做 Cache 操作 | 设备读到旧数据，或 CPU 读到旧数据 | DMA 前 invalidate/clean |
| 2 | 混淆 Clean 和 Invalidate | Clean 只写回不丢弃；Invalidate 只丢弃不写回 | 理解操作语义 |
| 3 | 自修改代码忘清 I-Cache | CPU 执行旧的指令缓存 | clean D-cache → invalidate I-cache |
| 4 | VIPT 别名 | 多个 VA 映射同 PA 时 Cache 不一致 | 内核通过页着色避免 |

### 调试技巧

| 症状 | 可能原因 |
|------|----------|
| DMA 数据偶尔错误 | 忘 invalidate/clean cache |
| 修改代码后仍执行旧指令 | I-Cache 缓存旧指令 |
| 多 VA 映射同 PA 数据不一致 | VIPT 别名 |
| 性能异常低 | 伪共享（见 Ch16） |

## HFT 关联

DMA cache 操作错误在 HFT 系统中是最常见的隐蔽 bug——数据"偶尔"不对，因为 cache 命中时读到旧值，miss 时读到新值，行为不确定。HFT 系统如果用 DMA 收发网络包，必须确保每次 DMA 操作都有正确的 cache 维护。自修改代码在 HFT 中不常见（通常不用 JIT），但如果用 eBPF 或动态补丁，必须处理 I-Cache 一致性。

## 自测题

1. **DMA 从设备读数据后 CPU 读到旧值，最可能的原因是什么？**

<details>
<summary>答案</summary>

**忘记 invalidate CPU cache**。DMA 写入新数据到内存，但 CPU cache 中还有旧数据。CPU 读时命中 cache → 读到旧值。修复：DMA 写之前 `dc ivac`（invalidate）对应区域，强制 CPU 下次从内存读。
</details>

2. **自修改代码修改后 CPU 仍执行旧指令，应该怎么修复？**

<details>
<summary>答案</summary>

1. **Clean D-cache**：新指令写在 D-cache 中，需写回内存（`dc cvac`）
2. **DSB**：等写回完成
3. **Invalidate I-cache**：丢弃 I-cache 中的旧指令缓存（`ic ivau`）
4. **DSB + ISB**：确保后续取指用新指令

顺序很重要：先写回 D-cache，再清 I-cache。因为 I-cache 和 D-cache 是分离的。
</details>

3. **Clean 和 Invalidate 误用会有什么后果？**

<details>
<summary>答案</summary>

- 该 **Invalidate** 时用了 **Clean**：cache 行仍有效，CPU 仍读旧值（DMA 场景）
- 该 **Clean** 时用了 **Invalidate**：脏数据丢失！CPU 写的新数据被丢弃，没有写回内存 → DMA 读到旧值或内存数据丢失

两种误用都会导致数据不一致，但症状不同。Clean 误用通常"数据不更新"，Invalidate 误用通常"数据丢失"。
</details>

## 参考与延伸

- [§15.5 DMA 与 Cache](05-dma-cache.md) — DMA cache 操作详解
- [§15.4 关键概念](04-key-concepts.md) — Clean/Invalidate/Flush 语义
- [Ch16 §16.4 自修改代码](../../chapter-16-cache-coherency/notes/section-0-本章完整概述.md) — I-Cache 一致性
