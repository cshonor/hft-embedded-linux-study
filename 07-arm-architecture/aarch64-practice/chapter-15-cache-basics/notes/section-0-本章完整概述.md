# Ch15 完整总结 · 高速缓存基础知识

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **选读**（对照 Hennessy）  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

Cache 基本概念：映射方式、替换策略、PIPT/VIPT、别名问题。选读——可对照计算机体系结构教材。

---

## 15.1 Cache 映射方式

| 方式 | 说明 | 优缺点 |
|------|------|--------|
| **直接映射** | 每个地址只能放一个固定位置 | 简单、快；冲突率高 |
| **全相联** | 地址可放任意位置 | 冲突最低；查找慢、面积大 |
| **组相联** | 折中：分 N 组，每组 K 路 | 实际常用（如 4-way/8-way） |

### 组相联地址分解

```
Cache 地址 = [Tag | Index | Block Offset]

  Index → 选哪一组
  Tag   → 组内哪一路命中
  Offset → 块内偏移
```

---

## 15.2 PIPT vs VIPT

| 类型 | 全称 | 说明 |
|------|------|------|
| **PIPT** | Physically Indexed, Physically Tagged | 用物理地址索引+匹配；最安全但慢 |
| **VIPT** | Virtually Indexed, Physically Tagged | 用虚拟地址索引，物理地址匹配；快但有别名问题 |

**别名问题（Synonym）：**
- 两个不同 VA 映射到同一 PA
- 如果 VIPT 的 Index 使用 VA 的低位（在页偏移内），则两个 VA 的 Index 相同 → 无别名
- 如果 Index 超出页偏移 → 两个 VA 可能映射到不同 Cache 行 → 同一 PA 有两份缓存 → 数据不一致

> **ARM Cortex-A 系列：** L1 D-cache 通常 VIPT，但保证 Index 在页偏移内（颜色限制），避免别名。L2/L3 为 PIPT。

---

## 15.3 ARMv8 Cache 层次

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

| 层 | 大小 | 延迟 |
|----|------|------|
| L1 | 32-64KB | 1-2 cycle |
| L2 | 256KB-1MB | 8-12 cycle |
| L3 | 2-8MB | 30-50 cycle |
| DRAM | — | 100-300 cycle |

---

## 15.4 关键概念

| 概念 | 含义 |
|------|------|
| **Cache Line** | 最小加载/替换单位（通常 64 字节） |
| **PoU** (Point of Unification) | I-Cache 和 D-Cache 汇聚点 |
| **PoC** (Point of Coherency) | 所有 CPU 核和 DMA 的汇聚点 |
| **Clean** | 写回脏数据到下一级内存 |
| **Invalidate** | 丢弃 Cache 内容（不写回） |
| **Flush** | Clean + Invalidate |

```c
// Linux 中常用 API
flush_dcache_page()      // clean + invalidate
invalidate_icache_range() // 使 I-cache 无效
```

---

## 15.5 DMA 与 Cache ⭐

```
DMA 直接到内存，不经过 CPU Cache → 数据不一致

场景1：DMA 写内存（设备→内存）
  CPU Cache 有旧数据 → CPU 读到旧值
  解决：DMA 前先 invalidate CPU Cache 对应区域

场景2：DMA 读内存（内存→设备）
  CPU 写了新数据在 Cache 中，还没写回 → DMA 读到旧值
  解决：DMA 前先 clean（flush）CPU Cache 对应区域
```

> 这是嵌入式驱动中必须处理的 Cache 一致性问题。

---

## 15.6 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 15-1 | 枚举高速缓存（读 CTR_EL0 / CSI） | Linux |
| 15-2 | 清理高速缓存（dc cvac / civac） | Linux |

---

## 15.7 易错点清单

1. **DMA 忘做 Cache 操作** → 设备读到旧数据，或 CPU 读到旧数据。
2. **混淆 Clean 和 Invalidate** → Clean 只写回不丢弃；Invalidate 只丢弃不写回。
3. **自修改代码忘清 I-Cache** → CPU 执行旧的指令缓存。
4. **VIPT 别名** → 多个 VA 映射同 PA 时 Cache 不一致（内核通常通过页着色避免）。

---

## 书中思考题（自测）

1. 直接映射、全相联、组相联各有什么优缺点？
2. PIPT 和 VIPT 的区别？VIPT 有什么问题？
3. PoU 和 PoC 分别是什么？
4. DMA 读写内存时分别需要做什么 Cache 操作？
5. Clean 和 Invalidate 的区别？

**参考答案：**

1. 直接映射简单但冲突高；全相联冲突低但面积大/查找慢；组相联折中。  
2. PIPT 用物理地址索引+匹配；VIPT 用 VA 索引、PA 匹配。VIPT 有**别名问题**。  
3. PoU=I-Cache 和 D-Cache 汇聚点；PoC=所有 CPU 和 DMA 的汇聚点。  
4. DMA 写内存→先 **invalidate**；DMA 读内存→先 **clean**。  
5. Clean=**写回**脏数据（不丢）；Invalidate=**丢弃**（不写回）。

---

上一章 [Ch14 内存管理](../../chapter-14-memory-management/) · 下一章 [Ch16 缓存一致性](../../chapter-16-cache-coherency/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
