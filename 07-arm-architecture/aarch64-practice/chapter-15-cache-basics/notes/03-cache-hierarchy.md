# §15.3 ARMv8 Cache 层次

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARMv8 的多级 cache 层次：L1（每核私有，I/D 分离）、L2（每核私有或共享）、L3（全核共享）。延迟从 L1 的 1-2 cycle 到 DRAM 的 100-300 cycle。

## 核心要点

### Cache 层次结构

```
CPU Core 0          CPU Core 1
├── L1 I-Cache      ├── L1 I-Cache
├── L1 D-Cache      ├── L1 D-Cache
└── L2 Unified      └── L2 Unified
        ↓                  ↓
      ┌──────── L3 Shared ────────┐
      │                           │
      └──── Main Memory ─────────┘
```

### 各级延迟

| 层 | 大小 | 延迟 |
|----|------|------|
| L1 | 32-64KB | 1-2 cycle |
| L2 | 256KB-1MB | 8-12 cycle |
| L3 | 2-8MB | 30-50 cycle |
| DRAM | — | 100-300 cycle |

### Pi5 (Cortex-A76) 典型配置

| 层 | 大小 | 延迟（cycle @ 2.4GHz） |
|----|------|----------------------|
| L1 I/D | 64KB/64KB | 4 cycle |
| L2 | 512KB | 12 cycle |
| L3 | 共享 | ~40 cycle |
| DRAM | — | ~200 cycle |

## HFT 关联

HFT 的延迟敏感数据必须放在 L1 中。Pi5 A76 的 L1 延迟 4 cycle ≈ 1.7ns，L2 12 cycle ≈ 5ns，DRAM 200 cycle ≈ 83ns。差距 50 倍。HFT 系统应将热数据结构控制在 L1 大小内（64KB），超出后每 miss 一次增加 3-4ns（L2）或 80ns（DRAM）。使用 `__attribute__((hot))` 提示编译器将关键函数放在一起，减少 I-cache miss。Cache 行预取（`__builtin_prefetch`）可以隐藏 L2/L3 延迟。

## 自测题

1. **L1 和 L2 的延迟差距大约多少倍？HFT 数据应放在哪一级？**

<details>
<summary>答案</summary>

L1 约 1-2 cycle，L2 约 8-12 cycle，差距约 **5-10 倍**。HFT 热数据应放在 **L1**（64KB 以内），超出后每次 L2 miss 增加 ~5ns。如果数据结构大于 L1，考虑压缩或分块（tiling）。
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

## 参考与延伸

- [§15.1 Cache 映射方式](01-cache-mapping.md) — 每级 cache 的映射方式
- [§15.4 关键概念](04-key-concepts.md) — Cache line 和 PoU/PoC
- [Ch16 §16.2 伪共享](../../chapter-16-cache-coherency/notes/section-0-本章完整概述.md) — 多核 cache 层次导致的问题
