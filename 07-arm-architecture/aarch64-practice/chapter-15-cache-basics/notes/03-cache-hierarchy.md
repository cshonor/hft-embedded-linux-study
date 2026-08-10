# §15.3 ARMv8 Cache 层次

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARMv8 的多级 cache 层次：L1（每核私有，I/D 分离）、L2（每核私有或共享）、L3（全核共享）。延迟从 L1 的 1-2 cycle 到 DRAM 的 100-300 cycle。理解 cache 层次结构对 HFT 延迟优化至关重要。

## 核心要点

### Cache 层次结构

```
CPU Core 0              CPU Core 1
├── L1 I-Cache (64KB)   ├── L1 I-Cache (64KB)    ← 每核私有，分离
├── L1 D-Cache (64KB)   ├── L1 D-Cache (64KB)    ← 每核私有，分离
└── L2 Unified (512KB)  └── L2 Unified (512KB)   ← 每核私有，统一
        |                       |
      ┌─────── L3 Shared (2-8MB) ──────┐         ← 全核共享
      │                                │
      └──────── Main Memory ──────────┘          ← DRAM
```

### 各级延迟对比

| 层 | 典型大小 | 延迟(cycle) | 延迟(ns @2.4GHz) | 说明 |
|----|---------|------------|------------------|------|
| L1 | 32-64KB | 1-4 | 0.4-1.7 | 每核私有，I/D 分离 |
| L2 | 256KB-1MB | 8-12 | 3.3-5.0 | 每核私有，统一 |
| L3 | 2-8MB | 30-50 | 12.5-20.8 | 全核共享 |
| DRAM | — | 100-300 | 41.7-125 | 主存 |

### Pi4B (Cortex-A72) vs Pi5 (Cortex-A76) 对比

| 层 | Pi4B (A72) | Pi5 (A76) | 差异 |
|----|-----------|-----------|------|
| L1 I-Cache | 48KB 3-way | 64KB 4-way | A76 更大 |
| L1 D-Cache | 32KB 2-way | 64KB 4-way | A76 翻倍 |
| L2 | 1MB (共享2核) | 512KB (私有) | A76 改为私有 |
| L3 | 无 | 2-4MB 共享 | A76 新增 L3 |
| L1 延迟 | 4 cycle | 4 cycle | 相同 |
| L2 延迟 | ~12 cycle | ~12 cycle | 相同 |
| L3 延迟 | N/A | ~40 cycle | A76 新增 |

> A76 的 L2 从共享改为私有（每核 512KB），但新增了全核共享的 L3。
> 这改变了多核通信的 cache 层次——核间数据共享通过 L3 而非 L2。

### L1 为什么分 I-Cache 和 D-Cache？

| 理由 | 说明 |
|------|------|
| 并行访问 | 取指和访存可以同时进行（分别查 I-cache 和 D-cache） |
| 不同优化 | I-cache 只读（不需要写端口），D-cache 需要读写端口 |
| 替换策略不同 | I-cache 通常是顺序访问（空间局部性强），D-cache 访问模式复杂 |
| 哈佛架构 | L1 用哈佛架构（I/D 分离），L2 及以下用普林斯顿架构（统一） |

### Cache 包含关系

| 类型 | 说明 | 典型应用 |
|------|------|----------|
| Inclusive | L3 包含 L2 和 L1 的所有数据 | Intel CPU |
| Exclusive | L1/L2/L3 互不包含（数据只在一层） | AMD CPU |
| Non-inclusive/Non-exclusive | 不强制包含也不互斥 | ARM Cortex-A |

> ARM Cortex-A76 的 L2 不包含在 L3 中（non-inclusive），节省 L3 容量。

## HFT 关联

HFT 的延迟敏感数据必须放在 L1 中。Pi5 A76 的 L1 延迟 4 cycle ≈ 1.7ns，L2 12 cycle ≈ 5ns，DRAM 200 cycle ≈ 83ns。差距 50 倍。HFT 系统应将热数据结构控制在 L1 大小内（64KB），超出后每 miss 一次增加 3-4ns（L2）或 80ns（DRAM）。使用 `__attribute__((hot))` 提示编译器将关键函数放在一起，减少 I-cache miss。Cache 行预取（`__builtin_prefetch`）可以隐藏 L2/L3 延迟。

```c
// HFT 热数据结构应控制在 L1 D-cache 大小内
// Pi5 A76 L1 D-cache = 64KB
struct order_book {
    // 热路径数据（< 64KB）
    uint64_t best_bid;      // 最优买价
    uint64_t best_ask;      // 最优卖价
    uint32_t bid_levels[64]; // 买盘层级
    uint32_t ask_levels[64]; // 卖盘层级
    // 冷数据移到另一个结构
} __attribute__((aligned(64)));
```

## 自测题

1. **L1 和 L2 的延迟差距大约多少倍？HFT 数据应放在哪一级？**

<details>
<summary>答案</summary>

L1 约 1-4 cycle，L2 约 8-12 cycle，差距约 **3-10 倍**。HFT 热数据应放在 **L1**（64KB 以内），超出后每次 L2 miss 增加 ~5ns。如果数据结构大于 L1，考虑压缩或分块（tiling）。
</details>

2. **L1 为什么要分 I-Cache 和 D-Cache？L2 为什么不分？**

<details>
<summary>答案</summary>

L1 分 I/D 是因为取指和访存可以**并行**（同时查 I-cache 和 D-cache），提高流水线吞吐。L2 不分是因为 L2 是 L1 miss 后的备份，统一存储更节省空间（代码和数据共享 L2 容量），且 L2 延迟远大于 L1，分离的收益不大。
</details>

3. **Pi5 Cortex-A76 的 L1 D-cache 有多大？如果 HFT 订单簿 80KB，会发生什么？**

<details>
<summary>答案</summary>

L1 D-cache = **64KB**。订单簿 80KB > 64KB → 超出 L1 → 部分 miss 到 L2（+5ns/miss）。解决方案：1) 压缩订单簿数据结构到 64KB 以内；2) 分块处理（只活跃部分放 L1）；3) 用 prefetch 预取即将访问的部分。
</details>

4. **A76 的 L2 从共享改为私有，对多核 HFT 有什么影响？**

<details>
<summary>答案</summary>

A72 的 L2 是 2 核共享（1MB），A76 改为每核私有（512KB）+ 全核共享 L3（2-4MB）。

影响：1) 每核 L2 独占 512KB，不被其他核干扰 → 延迟更确定；2) 核间数据共享从 L2（快）变为 L3（慢 ~40 cycle），核间通信延迟增加；3) 但 L2 私有避免了 MESI 在 L2 层的竞争。

对 HFT：每核独立交易数据放 L2（私有，无干扰）更好；核间共享数据（如行情广播）需要走 L3，延迟增加。总体上 A76 的设计对 HFT 更有利（每核隔离更好）。
</details>

## 参考与延伸

- [§15.1 Cache 映射方式](01-cache-mapping.md) — 每级 cache 的映射方式
- [§15.4 关键概念](04-key-concepts.md) — Cache line 和 PoU/PoC
- [Ch16 §16.2 伪共享](../../chapter-16-cache-coherency/notes/02-false-sharing.md) — 多核 cache 层次导致的问题
