# §15.4 关键概念

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Cache 的关键概念：Cache Line（最小加载/替换单位）、PoU（I/D-Cache 汇聚点）、PoC（所有核+DMA 汇聚点）、Clean/Invalidate/Flush 操作。这些概念是理解 DMA cache 一致性和自修改代码的基础。

## 核心要点

### 关键概念表

| 概念 | 全称 | 含义 | 典型层级 |
|------|------|------|----------|
| **Cache Line** | — | 最小加载/替换单位（通常 64 字节） | 所有层 |
| **PoU** | Point of Unification | I-Cache 和 D-Cache 汇聚点 | L2（I/D 统一） |
| **PoC** | Point of Coherency | 所有 CPU 核和 DMA 的汇聚点 | L3 或内存 |
| **PoP** | Point of Persistence | 数据持久化点（非易失内存） | NVM |

### Clean vs Invalidate vs Flush

| 操作 | 做什么 | 不做什么 | Cache 行状态 | 典型用途 |
|------|--------|----------|-------------|----------|
| Clean | 写回脏数据到下一级 | 不丢弃 cache 行 | 仍有效（后续可命中） | DMA 读内存前 |
| Invalidate | 丢弃 cache 行 | 不写回脏数据 | 无效（下次必定 miss） | DMA 写内存后 |
| Flush | Clean + Invalidate | — | 无效（写回+丢弃） | 自修改代码 |
| Prefetch | 预加载到 cache | — | 有效（如果命中） | 隐藏延迟 |

```
Clean 操作：
  D-Cache: [脏数据A] → 写回内存 → D-Cache: [干净数据A]（仍在 cache）

Invalidate 操作：
  D-Cache: [数据B] → 丢弃 → D-Cache: [空]（数据B丢失，如果脏则不写回！）

Flush 操作：
  D-Cache: [脏数据C] → 写回内存 → 丢弃 → D-Cache: [空]
```

### PoU vs PoC

| 概念 | 汇聚什么 | 典型层级 | 操作范围 |
|------|----------|----------|----------|
| PoU | I-Cache + D-Cache（同一核） | L2（I/D 统一） | 单核 |
| PoC | 所有核 + DMA | L3 或内存 | 全系统 |

```
PoU 场景（自修改代码）：
  CPU 写新指令 → D-Cache → clean 到 PoU(L2) → invalidate I-Cache
  确保 I-Cache 和 D-Cache 在 L2 层面看到一致的数据

PoC 场景（DMA）：
  CPU 写数据 → D-Cache → clean 到 PoC(L3/内存) → DMA 从内存读
  确保所有核和 DMA 设备看到一致的数据
```

### ARMv8 Cache 维护指令

| 指令 | 作用 | 操作到 |
|------|------|--------|
| `dc cvac, x0` | Clean by VA to PoC | 写回脏数据到 PoC |
| `dc cvau, x0` | Clean by VA to PoU | 写回脏数据到 PoU |
| `dc ivac, x0` | Invalidate by VA to PoC | 丢弃 cache 行到 PoC |
| `dc civac, x0` | Clean+Invalidate by VA to PoC | Flush（写回+丢弃） |
| `dc csw, x0` | Clean by Set/Way | 全 cache clean |
| `dc isw, x0` | Invalidate by Set/Way | 全 cache invalidate |
| `ic ivau, x0` | Invalidate I-Cache by VA to PoU | 丢弃 I-Cache 行 |
| `ic iallu` | Invalidate All I-Cache (local) | 全 I-Cache 无效 |
| `ic ialluis` | Invalidate All I-Cache (Inner Shareable) | 跨核 I-Cache 无效 |

### Linux Cache API 对应

```c
// Linux 内核中常用 API
void flush_dcache_page(struct page *page);      // clean + invalidate
void clean_dcache_page(struct page *page);       // clean only
void invalidate_dcache_page(struct page *page);  // invalidate only
void flush_icache_range(unsigned long start, unsigned long end);  // I-Cache flush
void flush_cache_range(struct vm_area_struct *vma,
                       unsigned long start, unsigned long end);   // 全 cache flush
```

## HFT 关联

理解 Cache Line 大小（64 字节）对 HFT 数据结构设计至关重要——频繁访问的字段应放在同一 cache line 内（减少 miss），不相关的字段应分到不同 cache line（避免伪共享）。PoU 和 PoC 的概念在 DMA 场景重要：网卡 DMA 读写的数据 buffer 需要 clean（DMA 读前）或 invalidate（DMA 写后），操作到 PoC 级别确保 DMA 和 CPU 看到一致的数据。

```c
// HFT 数据结构按 cache line 对齐
struct hot_data {
    uint64_t price;        // offset 0
    uint64_t quantity;     // offset 8
    uint32_t flags;        // offset 16
    // 填充到 64 字节，避免和其他数据伪共享
    char pad[44];
} __attribute__((aligned(64)));
```

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

4. **`dc cvac` 和 `dc cvau` 有什么区别？分别用于什么场景？**

<details>
<summary>答案</summary>

- `dc cvac`：Clean 到 **PoC**（所有核+DMA 可见）→ 用于 DMA 场景（确保 DMA 能读到最新数据）
- `dc cvau`：Clean 到 **PoU**（仅 I-Cache 和 D-Cache 汇聚）→ 用于自修改代码（确保 I-Cache 能看到 D-Cache 写的新指令）

PoC 范围更大（到 L3/内存），PoU 范围更小（到 L2）。DMA 需要操作到 PoC，自修改代码只需操作到 PoU。
</details>

## 参考与延伸

- [§15.5 DMA 与 Cache](05-dma-cache.md) — DMA 场景的 cache 操作
- [§15.2 PIPT vs VIPT](02-pipt-vipt.md) — Cache 索引方式
- [Ch16 §16.3 DMA 一致性](../../chapter-16-cache-coherency/notes/03-dma-coherency.md) — DMA cache 一致性详解
