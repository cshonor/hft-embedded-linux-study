# §15.2 PIPT vs VIPT

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

PIPT 用物理地址索引+匹配（安全但慢），VIPT 用虚拟地址索引+物理地址匹配（快但有别名问题）。ARM Cortex-A L1 D-cache 通常用 VIPT 但保证 Index 在页偏移内避免别名。理解索引方式有助于分析 cache 性能和别名问题。

## 核心要点

### 四种索引/标记方式

| 类型 | Index 用 | Tag 用 | 速度 | 别名问题 | 典型应用 |
|------|----------|--------|------|----------|----------|
| VIVT | VA | VA | 最快 | 严重（进程切换需 flush） | 几乎不用 |
| **PIPT** | PA | PA | 慢（需先 TLB 翻译） | 无 | L2/L3 |
| **VIPT** | VA | PA | 快（TLB 和 cache 并行查） | 有条件无别名 | L1 |
| PIVT | PA | VA | 慢 + 别名 | 有别名 | 几乎不用 |

### PIPT vs VIPT

| 类型 | 全称 | 说明 | 延迟 |
|------|------|------|------|
| **PIPT** | Physically Indexed, Physically Tagged | 用物理地址索引+匹配；最安全但需先 TLB 翻译 | 高 |
| **VIPT** | Virtually Indexed, Physically Tagged | 用 VA 低位索引，PA 匹配 Tag；可与 TLB 并行 | 低 |

```
PIPT 查找流程（串行）：
  VA → TLB 翻译 → PA → 用 PA 查 Cache → 命中/未命中
  （TLB 查找和 Cache 查找是串行的）

VIPT 查找流程（并行）：
  VA → 同时送 TLB 和 Cache
       ├→ TLB: VA → PA (得到 Tag)
       └→ Cache: 用 VA 的 Index 位查组 → 读出所有路的 Tag
  比较 TLB 返回的 PA Tag 和 Cache 读出的 Tag → 命中/未命中
  （TLB 翻译和 Cache 索引并行，延迟更低）
```

### 别名问题（Synonym）

- 两个不同 VA 映射到同一 PA（共享内存场景）
- 如果 VIPT 的 Index 使用 VA 的低位（在页偏移内），则两个 VA 的 Index 相同 → 无别名
- 如果 Index 超出页偏移 → 两个 VA 可能映射到不同 Cache 组 → 同一 PA 有两份缓存 → 数据不一致

> **ARM Cortex-A 系列：** L1 D-cache 通常 VIPT，但保证 Index 在页偏移内（颜色限制），避免别名。L2/L3 为 PIPT。

### VIPT 无别名条件

```
Cache Index 位数 + Block Offset 位数 ≤ 页偏移位数（12 位 for 4KB 页）

因为：页偏移内的 VA 低位 = PA 低位（4KB 页内偏移不变）
所以：不同 VA 映射同一 PA 时，页偏移内的 Index 相同 → 查到同一 cache 组 → 无别名

例1：64 字节 line × 64 组 → Index=6 + Offset=6 = 12 ≤ 12 → 无别名 ✓
例2：64 字节 line × 256 组 → Index=8 + Offset=6 = 14 > 12 → 有别名 ✗
```

### 大页对 VIPT 的影响

| 页大小 | 页偏移位数 | 允许的 Index+Offset | 最大无别名 Cache 大小 |
|--------|-----------|--------------------|-----------------------|
| 4KB | 12 位 | 12 位 | 4-way × 64B × 64组 = 16KB |
| 2MB | 21 位 | 21 位 | 4-way × 64B × 32768组 = 8MB |
| 1GB | 30 位 | 30 位 | 几乎无限制 |

> 使用 2MB 大页时，VIPT 的无别名条件放宽到 21 位，允许更大的 L1 cache 而无别名。

## HFT 关联

VIPT 的别名问题在 HFT 系统中可能导致数据不一致——两个 VA 映射同一 PA，CPU 通过一个 VA 写数据，通过另一个 VA 读到旧值。ARM Cortex-A 通过限制 cache 大小和路数避免别名（Index 在页偏移内），但 HFT 开发者仍需注意：不要手动创建同一 PA 的多个 VA 映射（除非使用 kernel API 正确处理）。使用大页（2MB）可以增加页偏移位数，允许更大的 VIPT cache 而无别名。

## 自测题

1. **PIPT 和 VIPT 的主要区别是什么？为什么 VIPT 更快？**

<details>
<summary>答案</summary>

- **PIPT**：用**物理地址**做 Index 和 Tag。需要先翻译 VA→PA（通过 TLB），然后才能查 cache。TLB 查找和 cache 查找是**串行**的。慢但安全——不会别名。
- **VIPT**：用**虚拟地址**做 Index，**物理地址**做 Tag。可以**同时**查 TLB 和 cache（并行），TLB 返回 PA Tag 与 cache 读出的 Tag 比较。快但有别名风险。

VIPT 快是因为 TLB 翻译和 cache 索引并行，不需要等 TLB 完成就能开始查 cache。
</details>

2. **VIPT 在什么条件下不会出现别名问题？**

<details>
<summary>答案</summary>

当 **Index 位数 + Block Offset 位数 ≤ 页偏移位数**时无别名。因为页偏移内的 VA 低位 = PA 低位（页内偏移不变），所以不同 VA 映射同一 PA 时，页偏移内的 Index 相同 → 查到同一 cache 组 → 无别名。4KB 页的页偏移 = 12 位，所以 Index + Offset ≤ 12 时安全。
</details>

3. **64 字节 cache line、4-way 组相联、64 组的 L1 D-cache，用 VIPT 是否有别名问题？（4KB 页）**

<details>
<summary>答案</summary>

Index = log2(64) = 6 位，Offset = log2(64) = 6 位。6 + 6 = 12 ≤ 12（4KB 页偏移）→ **无别名**。这个 cache 大小 = 4 × 64 × 64 = 16KB，是典型的 L1 D-cache 配置，ARM Cortex-A 保证无别名。
</details>

4. **为什么 L2/L3 用 PIPT 而不用 VIPT？**

<details>
<summary>答案</summary>

L2/L3 容量大（256KB-8MB），如果用 VIPT，Index 位数会超过页偏移位数（12 位），导致别名问题。PIPT 用物理地址索引，天然无别名，不受页偏移限制。

另外 L2/L3 的延迟本身较大（8-50 cycle），PIPT 的额外 TLB 查找延迟相对可忽略（L1 miss 后 TLB 大概率已命中）。
</details>

## 参考与延伸

- [§15.1 Cache 映射方式](01-cache-mapping.md) — Index/Tag/Offset 分解
- [§15.4 关键概念](04-key-concepts.md) — Cache line 大小和页偏移的关系
- [Ch14 §14.2 四级页表](../../chapter-14-memory-management/notes/02-four-level-page-table.md) — 页偏移和虚拟地址
