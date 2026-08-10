# §15.4 关键概念

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Cache 的关键概念：Cache Line（最小加载/替换单位）、PoU（I/D-Cache 汇聚点）、PoC（所有核+DMA 汇聚点）、Clean/Invalidate/Flush 操作。

## 核心要点

### 关键概念

| 概念 | 含义 |
|------|------|
| **Cache Line** | 最小加载/替换单位（通常 64 字节） |
| **PoU** (Point of Unification) | I-Cache 和 D-Cache 汇聚点 |
| **PoC** (Point of Coherency) | 所有 CPU 核和 DMA 的汇聚点 |
| **Clean** | 写回脏数据到下一级内存 |
| **Invalidate** | 丢弃 Cache 内容（不写回） |
| **Flush** | Clean + Invalidate |

### Clean vs Invalidate vs Flush

| 操作 | 做什么 | 不做什么 | 典型用途 |
|------|--------|----------|----------|
| Clean | 写回脏数据 | 不丢弃 cache 行 | DMA 读内存前 |
| Invalidate | 丢弃 cache 行 | 不写回脏数据 | DMA 写内存后 |
| Flush | 写回 + 丢弃 | — | 自修改代码 |

### PoU vs PoC

| 概念 | 汇聚什么 | 典型层级 |
|------|----------|----------|
| PoU | I-Cache + D-Cache（同一核） | L2（I/D 统一） |
| PoC | 所有核 + DMA | L3 或内存 |

```c
// Linux 中常用 API
flush_dcache_page()      // clean + invalidate
invalidate_icache_range() // 使 I-cache 无效
```

## HFT 关联

理解 Cache Line 大小（64 字节）对 HFT 数据结构设计至关重要——频繁访问的字段应放在同一 cache line 内（减少 miss），不相关的字段应分到不同 cache line（避免伪共享）。PoU 和 PoC 的概念在 DMA 场景重要：网卡 DMA 读写的数据 buffer 需要 clean（DMA 读前）或 invalidate（DMA 写后），操作到 PoC 级别确保 DMA 和 CPU 看到一致的数据。

## 自测题

1. **Clean 和 Invalidate 的区别是什么？Flush 包含哪些操作？**

<details>
<summary>答案</summary>

- **Clean**：写回脏数据到下一级内存，**不丢弃** cache 行（后续访问仍可能命中）
- **Invalidate**：丢弃 cache 行，**不写回**脏数据（脏数据丢失！）
- **Flush** = Clean + Invalidate（先写回再丢弃）
</details>

2. **PoU 和 PoC 分别是什么？哪个范围更大？**

<details>
<summary>答案</summary>

- **PoU**（Point of Unification）：同一核的 **I-Cache 和 D-Cache 汇聚点**（通常是 L2）
- **PoC**（Point of Coherency）：**所有 CPU 核 + DMA** 的汇聚点（通常是 L3 或内存）

**PoC 范围更大**——PoU 只管同一核的 I/D cache，PoC 管所有核和外设。
</details>

3. **DMA 从设备读数据到内存后，CPU 应该对 cache 做什么操作？**

<details>
<summary>答案</summary>

DMA 写入数据后，CPU cache 可能缓存了旧数据 → 需要 **Invalidate** 对应区域的 D-cache。这样 CPU 下次读该地址时 cache miss → 从内存读到 DMA 写入的新数据。注意：是 Invalidate 不是 Clean（不需要写回，因为 DMA 写的是新数据，cache 里的旧数据应丢弃）。
</details>

## 参考与延伸

- [§15.5 DMA 与 Cache](05-dma-cache.md) — DMA 场景的 cache 操作
- [§15.2 PIPT vs VIPT](02-pipt-vipt.md) — Cache 索引方式
- [Ch16 §16.3 DMA 一致性](../../chapter-16-cache-coherency/notes/section-0-本章完整概述.md) — DMA cache 一致性详解
