# §14.2 四级页表

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

48 位 VA = 4 级页表 × 9 位索引 + 12 位页内偏移。每级 512 表项 × 8 字节 = 4KB（恰好一页）。L1/L2 可用 Block descriptor 直接映射大块，省一层查找。

## 核心要点

### VA 分解

```
VA [47:39] → L0 索引 (512 entries)
VA [38:30] → L1 索引 (512 entries)
VA [29:21] → L2 索引 (512 entries)
VA [20:12] → L3 索引 (512 entries)
VA [11:0]  → 页内偏移 (4KB)
```

### 各级映射大小

| 级别 | 表项大小 | 每表项映射 | 表大小 |
|------|----------|-----------|--------|
| L0 | 8 字节 | 512GB | 4KB |
| L1 | 8 字节 | 1GB | 4KB |
| L2 | 8 字节 | 2MB | 4KB |
| L3 | 8 字节 | 4KB | 4KB |

> 每级 512 表项 × 8 字节 = 4KB，恰好一页。
> 可用 **Block descriptor** 在 L1(1GB) 或 L2(2MB) 层直接映射大块，省一层查找。

### Block vs Table descriptor

| 类型 | bit[1:0] | 可用层级 | 映射大小 |
|------|----------|----------|----------|
| Table | 0b11 | L0/L1/L2 | 指向下一级页表 |
| Block | 0b01 | L1/L2 | 1GB(L1) / 2MB(L2) |
| Page | 0b11 | L3 | 4KB |

## HFT 关联

HFT 系统应尽量使用大页（2MB Block descriptor）减少 TLB 压力。一个 2MB 大页只占 1 个 TLB 条目，而同样大小的 4KB 小页需要 512 个 TLB 条目。在 HFT 交易引擎中，将订单簿数据用 2MB 大页映射可以显著减少 TLB miss。另外，页表 walk 的 4 级查找在最坏情况下需要 4 次内存访问（~400ns），大页将 walk 减少到 2-3 级。

## 自测题

1. **48 位 VA 的 4 级页表，每级用多少位做索引？页内偏移多少位？**

<details>
<summary>答案</summary>

每级用 **9 位**做索引（9×4=36 位），页内偏移 **12 位**（4KB）。总计 36+12=48 位。每级 2^9 = 512 个表项。
</details>

2. **L1 的 Block descriptor 映射多大？L2 的 Block descriptor 映射多大？**

<details>
<summary>答案</summary>

- L1 Block：**1GB**（VA[38:30] 共 30 位，2^30 = 1GB）
- L2 Block：**2MB**（VA[29:21] 共 21 位，2^21 = 2MB）

L3 不能用 Block，只能用 Page（4KB）。
</details>

3. **为什么每级页表恰好是 4KB？**

<details>
<summary>答案</summary>

每级 512 表项 × 8 字节/表项 = 4096 字节 = **4KB**。这恰好是一个页的大小，方便内存分配和管理——分配一页就是一张页表。这是 ARMv8 架构的精心设计。
</details>

## 参考与延伸

- [§14.1 虚拟地址空间](01-va-space.md) — 48 位 VA 的空间布局
- [§14.3 页表项格式](03-descriptor-format.md) — Table/Block/Page descriptor 的位域
- [Ch17 §17.1 TLB 基本概念](../../chapter-17-tlb-management/notes/section-0-本章完整概述.md) — 大页减少 TLB miss
