# §15.1 Cache 映射方式

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Cache 的三种映射方式：直接映射（每个地址固定位置）、全相联（任意位置）、组相联（折中，实际最常用）。组相联用 Tag/Index/Offset 分解地址。理解映射方式有助于分析 cache 冲突 miss 和优化数据结构布局。

## 核心要点

### 三种映射方式

| 方式 | 说明 | 优缺点 | 典型应用 |
|------|------|--------|----------|
| **直接映射** | 每个地址只能放一个固定位置 | 简单、快、面积小；冲突率高 | 早期 L1 |
| **全相联** | 地址可放任意位置 | 冲突最低；查找慢、面积大 | TLB |
| **组相联** | 折中：分 N 组，每组 K 路 | 平衡冲突和速度 | 现代 L1/L2/L3 |

### 组相联地址分解

```
Cache 地址 = [Tag | Index | Block Offset]

  Index → 选哪一组（直接索引，不需要比较）
  Tag   → 组内哪一路命中（需要并行比较所有路）
  Offset → 块内偏移（选择 cache line 中的字节）

示例：4-way 组相联，64 组，64 字节 line
  VA = [Tag(剩余位) | Index(6位) | Offset(6位)]
  Index 6位 → 64 组
  Offset 6位 → 64 字节/line
  4 路 → 每组 4 个 cache line
  总大小 = 4 × 64 × 64 = 16KB
```

### 查找流程

```
1. 用 Index 选组（不需要比较，直接索引）
2. 用 Tag 并行比较该组所有路（4路并行比较）
3. 命中 → 用 Offset 选字节返回
4. 全部未命中 → Cache Miss → 从下一级加载
```

### 各级 Cache 典型配置

| 层级 | 路数 | 组数 | Line 大小 | 总大小 |
|------|------|------|----------|--------|
| L1 D-Cache | 4-way | 256 | 64B | 64KB |
| L1 I-Cache | 2-way/4-way | 128-256 | 64B | 32-64KB |
| L2 Cache | 8-way/16-way | 512-1024 | 64B | 256KB-1MB |
| L3 Cache | 16-way | 2048+ | 64-128B | 2-8MB |
| TLB | 全相联 | — | — | 64-128 条目 |

### Cache Miss 类型（3C 模型）

| 类型 | 全称 | 原因 | 映射方式影响 |
|------|------|------|-------------|
| Compulsory | 强制 miss | 第一次访问（冷启动） | 无影响 |
| Capacity | 容量 miss | Cache 容量不够 | 无影响 |
| **Conflict** | 冲突 miss | 多个地址映射到同一位置 | **直接映射最严重，全相联无冲突** |

> 组相联的路数越多 → 冲突 miss 越少，但查找延迟越大。
> 这就是为什么 L1 用 4-way（低延迟），L3 用 16-way（减少冲突）。

## HFT 关联

Cache 映射方式直接影响 HFT 的延迟确定性。组相联的冲突 miss 是不可预测的——如果两个热数据映射到同一组，会反复 evict。HFT 中应避免关键数据结构的大小是 cache 组大小的整数倍（否则所有元素映射到同一组）。例如 64-byte cache line × 8-way = 512-byte 组大小，如果数据结构是 512 字节的倍数，访问不同元素会冲突。用 `__attribute__((aligned(64)))` 对齐可以缓解。

```c
// 避免冲突 miss：数据结构不要是组大小（512B）的倍数
struct order_entry {
    uint64_t price;
    uint64_t quantity;
    uint32_t order_id;
    char pad[44];  // 填充到 64 字节（一个 cache line）
} __attribute__((aligned(64)));
```

## 自测题

1. **直接映射和全相联各自的优缺点是什么？**

<details>
<summary>答案</summary>

- **直接映射**：优点——简单、查找快（只查一个位置）、面积小功耗低；缺点——冲突率高（两个地址映射到同一位置必须 evict）
- **全相联**：优点——冲突率最低（任意位置都能放）；缺点——查找慢（要比较所有路）、面积大功耗高
</details>

2. **一个 4-way 组相联、64 组、64 字节 cache line 的 cache，总大小是多少？地址怎么分解？**

<details>
<summary>答案</summary>

总大小 = 4 路 × 64 组 × 64 字节 = **16384 字节 = 16KB**。地址分解：Index = 6 位（64 组），Offset = 6 位（64 字节），Tag = 剩余位。
</details>

3. **为什么 L1 通常用组相联而不是全相联？L3 用更多路？**

<details>
<summary>答案</summary>

L1 要求**低延迟**（1-2 cycle）。全相联需要比较所有路（如 32 路），延迟太大。组相联（4-way/8-way）只需比较 4-8 路，延迟可控。全相联通常用于 TLB（条目少，延迟要求不那么严格）。

L3 用更多路（16-way）是因为 L3 容量大（几 MB），如果路数少则组数过多，冲突 miss 增加。更多路可以减少冲突 miss，但 L3 延迟本身较大（30-50 cycle），多比较几路的开销可忽略。
</details>

4. **什么是冲突 miss？如何减少？**

<details>
<summary>答案</summary>

冲突 miss：多个地址映射到**同一 cache 组**，导致该组路数不够用，反复 evict。即使 cache 总容量未满也会发生。

减少方法：1) 增加路数（硬件设计时决定）；2) 避免数据结构大小是组大小（路数 × line 大小）的整数倍；3) 用 `aligned` 对齐控制数据布局；4) 对关键数据使用 `__builtin_prefetch` 预取。
</details>

## 参考与延伸

- [§15.2 PIPT vs VIPT](02-pipt-vipt.md) — 组相联下的索引方式
- [§15.3 Cache 层次](03-cache-hierarchy.md) — L1/L2/L3 的映射方式选择
- [Ch16 §16.2 伪共享](../../chapter-16-cache-coherency/notes/02-false-sharing.md) — Cache line 冲突导致的性能问题
